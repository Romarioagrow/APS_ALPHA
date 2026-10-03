// Rio 03.10 (galaxy phase 3): per-view passes of the GPU star renderer.
//   upload (progressive) -> clear -> raster (per set, budgeted) -> glow ray march (low res) -> composite CS.
// Compute only (03.10 08:17): the plugin creates no graphics PSO, and every compute PSO is created once through the
// RHI before its first dispatch, so a pipeline that cannot be built switches the feature off instead of crashing.
#include "APSStarViewExtension.h"

#include "APSStarRegistry.h"
#include "APSStarRendererPrivate.h"
#include "APSStarShaders.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "DynamicRHI.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "PixelFormat.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RHIGPUReadback.h"
#include "RHIStaticStates.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderUtils.h"
#include "SceneInterface.h"
#include "SceneTexturesConfig.h"
#include "SceneView.h"
#include "ScreenPass.h"
#include "SystemTextures.h"

DECLARE_GPU_STAT_NAMED(APSGpuStars, TEXT("APS Stars"));

namespace APSStarRenderer::Private
{
	namespace
	{
		template <typename ShaderType>
		TShaderRef<ShaderType> FindGlobalShader(const FGlobalShaderMap* ShaderMap, const int32 PermutationId)
		{
			TShaderRef<ShaderType> Result;
			if (ShaderMap != nullptr)
			{
				const TShaderRef<FShader> Shader = ShaderMap->GetShader(&ShaderType::GetStaticType(), PermutationId);
				if (Shader.IsValid())
				{
					Result = TShaderRef<ShaderType>::Cast(Shader);
				}
			}
			return Result;
		}

		/** View constants shared by every set and the glow. */
		struct FViewFrame
		{
			FMatrix TranslatedWorldToClip;
			FMatrix InvProjection;
			FMatrix ViewToTranslatedWorld;
			FVector ViewOrigin;
			FVector PreViewTranslation;
			FIntPoint TargetSize;
			double PixelSolidAngle = 1.0;
		};

		/** Local (packing) space -> render space under the optional observer far envelope. */
		struct FPresentation
		{
			FVector3f CameraLocal = FVector3f::ZeroVector;
			FMatrix44f LocalRotation = FMatrix44f::Identity;
			FVector3f ObserverTranslated = FVector3f::ZeroVector;
			/** Render cm per local unit at the observer, and that divided by the envelope. */
			double CompressA = 1.0;
			double CompressB = 0.0;
			double LocalScale = 1.0;
			bool bValid = false;
		};

		FPresentation MakePresentation(const FTransform& LocalToWorld, const FFarEnvelope& Envelope, const FViewFrame& Frame)
		{
			FPresentation Result;
			const double LocalScale = LocalToWorld.GetMaximumAxisScale();
			if (!FMath::IsFinite(LocalScale) || !(LocalScale > 0.0))
			{
				return Result;
			}
			FVector Observer = Frame.ViewOrigin;
			FVector ObserverRender = Frame.ViewOrigin;
			double CompressA = LocalScale;
			double CompressB = 0.0;
			if (Envelope.bEnabled && FMath::IsFinite(Envelope.RenderPerPhysical) && Envelope.RenderPerPhysical > 0.0)
			{
				Observer = Envelope.ObserverPhysical;
				ObserverRender = Envelope.ObserverRender;
				CompressA = LocalScale * Envelope.RenderPerPhysical;
				CompressB = Envelope.FarEnvelope > 0.0 ? CompressA / Envelope.FarEnvelope : 0.0;
			}
			const FVector CameraLocal = LocalToWorld.InverseTransformPosition(Observer);
			const FVector ObserverTranslated = ObserverRender + Frame.PreViewTranslation;
			Result.CameraLocal = FVector3f(CameraLocal);
			Result.LocalRotation = FMatrix44f(LocalToWorld.GetRotation().ToMatrix());
			Result.ObserverTranslated = FVector3f(ObserverTranslated);
			Result.CompressA = CompressA;
			Result.CompressB = CompressB;
			Result.LocalScale = LocalScale;
			Result.bValid = !CameraLocal.ContainsNaN() && !ObserverTranslated.ContainsNaN()
				&& FMath::IsFinite(CompressA) && CompressA > 0.0 && FMath::IsFinite(CompressB);
			return Result;
		}

		FMatrix WithoutTranslation(FMatrix Matrix)
		{
			Matrix.M[3][0] = 0.0;
			Matrix.M[3][1] = 0.0;
			Matrix.M[3][2] = 0.0;
			return Matrix;
		}

		FScreenPassTexture PassThrough(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SceneColor,
			const FPostProcessMaterialInputs& Inputs)
		{
			if (Inputs.OverrideOutput.IsValid() && SceneColor.IsValid())
			{
				AddDrawTexturePass(GraphBuilder, View, SceneColor, Inputs.OverrideOutput);
				return FScreenPassTexture(Inputs.OverrideOutput.Texture, Inputs.OverrideOutput.ViewRect);
			}
			return SceneColor;
		}

		void LogOnce(bool& bLogged, const TCHAR* Message)
		{
			if (!bLogged)
			{
				bLogged = true;
				UE_LOG(LogAPSStarRenderer, Warning, TEXT("[APS.GpuStars] %s"), Message);
			}
		}

		/**
		 * Rio 03.10 08:17: a pipeline that fails inside PipelineStateCache when a pass needs it is fatal; the RHI call
		 * underneath only returns null, and the RHI keeps the result, so the pass's own creation later is a cache hit.
		 * Every compute PSO goes through here once before its first dispatch. Render thread only.
		 */
		bool EnsureComputePipeline(FRHIComputeShader* Shader, const TCHAR* Name)
		{
			static TMap<FSHAHash, bool> Results;
			if (Shader == nullptr)
			{
				return false;
			}
			const FSHAHash Hash = Shader->GetHash();
			if (const bool* Known = Results.Find(Hash))
			{
				return *Known;
			}
			const FComputePipelineStateRHIRef Pipeline = RHICreateComputePipelineState(Shader);
			const bool bValid = Pipeline.IsValid();
			Results.Add(Hash, bValid);
			if (!bValid)
			{
				UE_LOG(LogAPSStarRenderer, Error,
					TEXT("[APS.GpuStars] %s: the RHI could not create its compute pipeline (shader %s); it is never dispatched"),
					Name, *Hash.ToString());
			}
			return bValid;
		}

		template <typename ShaderType>
		bool EnsureComputePipeline(const TShaderRef<ShaderType>& Shader, const TCHAR* Name)
		{
			return Shader.IsValid() && EnsureComputePipeline(Shader.GetComputeShader(), Name);
		}

		/** Creates the set's GPU buffer once and uploads one chunk per frame (catalogue order, so it fills in uniformly). */
		FRDGBufferRef PrepareSetBuffer(FRDGBuilder& GraphBuilder, FPointSetRT& Set, const uint32 FrameNumber)
		{
			FRDGBufferRef Buffer = nullptr;
			if (!Set.Buffer.IsValid())
			{
				if (Set.NumPoints <= 0 || Set.PendingPoints.Num() != Set.NumPoints)
				{
					return nullptr;
				}
				Buffer = GraphBuilder.CreateBuffer(
					FRDGBufferDesc::CreateStructuredDesc(sizeof(FPackedStar), static_cast<uint32>(Set.NumPoints)),
					TEXT("APS.Stars.Points"));
				Set.Buffer = GraphBuilder.ConvertToExternalBuffer(Buffer);
			}
			else
			{
				Buffer = GraphBuilder.RegisterExternalBuffer(Set.Buffer);
			}

			if (Set.NumUploaded < Set.NumPoints && Set.LastUploadFrame != FrameNumber
				&& Set.PendingPoints.Num() == Set.NumPoints)
			{
				Set.LastUploadFrame = FrameNumber;
				const int32 ChunkLimit = FMath::Max(CVarGpuUploadPointsPerFrame.GetValueOnRenderThread(), 4096);
				const int32 Chunk = FMath::Min(Set.NumPoints - Set.NumUploaded, ChunkLimit);
				const uint64 ChunkBytes = static_cast<uint64>(Chunk) * sizeof(FPackedStar);
				const FRDGBufferRef Upload = CreateStructuredBuffer(GraphBuilder, TEXT("APS.Stars.PointUpload"),
					sizeof(FPackedStar), static_cast<uint32>(Chunk), Set.PendingPoints.GetData() + Set.NumUploaded, ChunkBytes,
					ERDGInitialDataFlags::None);
				AddCopyBufferPass(GraphBuilder, Buffer, static_cast<uint64>(Set.NumUploaded) * sizeof(FPackedStar), Upload, 0, ChunkBytes);
				Set.NumUploaded += Chunk;
				if (Set.NumUploaded >= Set.NumPoints)
				{
					Set.PendingPoints.Empty();
					UE_LOG(LogAPSStarRenderer, Log, TEXT("[APS.GpuStars] point set %u '%s' resident: %d points, %.1f MB"),
						Set.Handle, *Set.Desc.DebugName, Set.NumPoints, Set.NumPoints * sizeof(FPackedStar) / (1024.0 * 1024.0));
				}
			}
			return Buffer;
		}

		struct FGlowMapTextures
		{
			FRDGTextureRef Emission = nullptr;
			FRDGTextureRef Profile = nullptr;
			FRDGTextureRef Shape = nullptr;
		};

		/** Uploads the glow maps once (map arrays are freed afterwards) and registers them for this graph. */
		bool PrepareGlowMaps(FRDGBuilder& GraphBuilder, FGlowVolumeRT& Volume, const FGlobalShaderMap* ShaderMap,
			FGlowMapTextures& OutTextures)
		{
			if (Volume.EmissionTexture.IsValid() && Volume.ProfileTexture.IsValid() && Volume.ShapeTexture.IsValid())
			{
				OutTextures.Emission = GraphBuilder.RegisterExternalTexture(Volume.EmissionTexture);
				OutTextures.Profile = GraphBuilder.RegisterExternalTexture(Volume.ProfileTexture);
				OutTextures.Shape = GraphBuilder.RegisterExternalTexture(Volume.ShapeTexture);
				return true;
			}
			const int32 Resolution = Volume.Map.Resolution;
			const int32 TexelCount = Resolution * Resolution;
			if (Resolution < 2 || Volume.Map.Emission.Num() != TexelCount || Volume.Map.Profile.Num() != TexelCount
				|| Volume.Map.Shape.Num() != TexelCount)
			{
				return false;
			}
			const TShaderRef<FAPSGlowUploadCS> UploadShader = FindGlobalShader<FAPSGlowUploadCS>(ShaderMap, 0);
			if (!EnsureComputePipeline(UploadShader, TEXT("glow upload")))
			{
				return false;
			}
			const FIntPoint Extent(Resolution, Resolution);
			const ETextureCreateFlags Flags = TexCreate_ShaderResource | TexCreate_UAV;
			const FRDGTextureRef Emission = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Extent, PF_FloatRGBA,
				FClearValueBinding::Black, Flags), TEXT("APS.Glow.EmissionMap"));
			const FRDGTextureRef Profile = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Extent, PF_A32B32G32R32F,
				FClearValueBinding::Black, Flags), TEXT("APS.Glow.ProfileMap"));
			const FRDGTextureRef Shape = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Extent, PF_R32_FLOAT,
				FClearValueBinding::Black, Flags), TEXT("APS.Glow.ShapeMap"));
			const FRDGBufferRef SourceEmission = CreateStructuredBuffer(GraphBuilder, TEXT("APS.Glow.EmissionUpload"),
				sizeof(FVector4f), static_cast<uint32>(TexelCount), Volume.Map.Emission.GetData(),
				static_cast<uint64>(TexelCount) * sizeof(FVector4f), ERDGInitialDataFlags::None);
			const FRDGBufferRef SourceProfile = CreateStructuredBuffer(GraphBuilder, TEXT("APS.Glow.ProfileUpload"),
				sizeof(FVector4f), static_cast<uint32>(TexelCount), Volume.Map.Profile.GetData(),
				static_cast<uint64>(TexelCount) * sizeof(FVector4f), ERDGInitialDataFlags::None);
			const FRDGBufferRef SourceShape = CreateStructuredBuffer(GraphBuilder, TEXT("APS.Glow.ShapeUpload"),
				sizeof(float), static_cast<uint32>(TexelCount), Volume.Map.Shape.GetData(),
				static_cast<uint64>(TexelCount) * sizeof(float), ERDGInitialDataFlags::None);

			FAPSGlowUploadCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAPSGlowUploadCS::FParameters>();
			Parameters->MapResolution = static_cast<uint32>(Resolution);
			Parameters->SrcEmission = GraphBuilder.CreateSRV(SourceEmission);
			Parameters->SrcProfile = GraphBuilder.CreateSRV(SourceProfile);
			Parameters->SrcShape = GraphBuilder.CreateSRV(SourceShape);
			Parameters->DstEmission = GraphBuilder.CreateUAV(Emission);
			Parameters->DstProfile = GraphBuilder.CreateUAV(Profile);
			Parameters->DstShape = GraphBuilder.CreateUAV(Shape);
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("APS.Glow.Upload %dx%d", Resolution, Resolution),
				UploadShader, Parameters, FComputeShaderUtils::GetGroupCount(Extent, 8));

			Volume.EmissionTexture = GraphBuilder.ConvertToExternalTexture(Emission);
			Volume.ProfileTexture = GraphBuilder.ConvertToExternalTexture(Profile);
			Volume.ShapeTexture = GraphBuilder.ConvertToExternalTexture(Shape);
			Volume.Map.Emission.Empty();
			Volume.Map.Profile.Empty();
			Volume.Map.Shape.Empty();
			OutTextures.Emission = Emission;
			OutTextures.Profile = Profile;
			OutTextures.Shape = Shape;
			return true;
		}

		/** Polls finished GPU counter read-backs (render thread). */
		void PollReadbacks(FRenderState& State)
		{
			for (FStatsReadback& Entry : State.Readbacks)
			{
				if (Entry.bInFlight && Entry.Readback.IsValid() && Entry.Readback->IsReady())
				{
					const uint32* Counters = static_cast<const uint32*>(Entry.Readback->Lock(4 * sizeof(uint32)));
					if (Counters != nullptr)
					{
						FRegistry::Get().PublishGpuCounters(Entry.FrameNumber, Counters);
					}
					Entry.Readback->Unlock();
					Entry.bInFlight = false;
				}
			}
		}

		FRHIGPUBufferReadback* AcquireReadback(FRenderState& State, const uint32 FrameNumber)
		{
			for (FStatsReadback& Entry : State.Readbacks)
			{
				if (!Entry.bInFlight)
				{
					Entry.bInFlight = true;
					Entry.FrameNumber = FrameNumber;
					return Entry.Readback.Get();
				}
			}
			if (State.Readbacks.Num() >= 4)
			{
				return nullptr;
			}
			FStatsReadback& Added = State.Readbacks.AddDefaulted_GetRef();
			Added.Readback = MakeUnique<FRHIGPUBufferReadback>(TEXT("APS.Stars.StatsReadback"));
			Added.bInFlight = true;
			Added.FrameNumber = FrameNumber;
			return Added.Readback.Get();
		}

		void PublishFrameCounters(FRenderState& State, const uint32 FrameNumber, const int64 Submitted)
		{
			if (State.CounterFrame != FrameNumber)
			{
				if (State.CounterFrame != MAX_uint32)
				{
					FStats Snapshot;
					for (const FPointSetRT& Set : State.PointSets)
					{
						Snapshot.PointsResident += Set.NumUploaded;
						Snapshot.PointsPending += Set.NumPoints - Set.NumUploaded;
					}
					Snapshot.PointsSubmitted = State.FrameSubmitted;
					Snapshot.FrameNumber = State.CounterFrame;
					FRegistry::Get().PublishStats(Snapshot);
				}
				State.CounterFrame = FrameNumber;
				State.FrameSubmitted = 0;
			}
			State.FrameSubmitted += Submitted;
		}

		FScreenPassTexture AddStarPasses(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
		{
			static bool bLoggedMissingShaders = false;
			static bool bLoggedAtomics = false;

			const FScreenPassTexture SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder,
				Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
			if (!SceneColor.IsValid())
			{
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}

			FRegistry& Registry = FRegistry::Get();
			if (Registry.IsShutdown() || View.Family == nullptr || View.Family->Scene == nullptr
				|| View.bIsReflectionCapture || View.bIsPlanarReflection || !View.IsPerspectiveProjection()
				|| View.GetFeatureLevel() < ERHIFeatureLevel::SM5 || Inputs.SceneTextures.SceneTextures.GetUniformBuffer() == nullptr)
			{
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}

			FRenderState& State = Registry.GetRenderState();
			const FSceneInterface* Scene = View.Family->Scene;
			const uint32 FrameNumber = View.Family->FrameNumber;
			const bool bSceneCapture = View.bIsSceneCapture;
			const bool bPointsOn = CVarGpuPoints.GetValueOnRenderThread() != 0;
			const bool bGlowOn = CVarGalaxyGlow.GetValueOnRenderThread() != 0;
			const float SceneVisibility = State.GetSceneVisibility(Scene);
			PollReadbacks(State);

			// Sets and the glow volume of this scene.
			TArray<FPointSetRT*, TInlineAllocator<16>> Sets;
			if (bPointsOn && SceneVisibility > 0.0f)
			{
				for (FPointSetRT& Set : State.PointSets)
				{
					if (Set.Scene == Scene && Set.Desc.bEnabled && Set.Desc.Visibility > 0.0f && Set.NumPoints > 0
						&& (!bSceneCapture || Set.Desc.bDrawInSceneCaptures))
					{
						Sets.Add(&Set);
					}
				}
				Sets.Sort([](const FPointSetRT& A, const FPointSetRT& B) { return A.Desc.Priority > B.Desc.Priority; });
			}
			FGlowVolumeRT* Volume = nullptr;
			if (bGlowOn && SceneVisibility > 0.0f)
			{
				for (FGlowVolumeRT& Candidate : State.GlowVolumes)
				{
					if (Candidate.Scene == Scene && Candidate.Desc.bEnabled && Candidate.Desc.Visibility > 0.0f
						&& Candidate.Desc.TotalIntensity > 0.0 && (!bSceneCapture || Candidate.Desc.bDrawInSceneCaptures))
					{
						Volume = &Candidate;
						break;
					}
				}
			}
			if (Sets.Num() == 0 && Volume == nullptr)
			{
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}

			// Shaders: anything missing (aps.Stars.CompileShaders=0 or a compile error with non-fatal errors) -> no change.
			const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
			const bool bWant64 = CVarGpuPointAtomic64.GetValueOnRenderThread() != 0 && NaniteAtomicsSupported()
				&& FDataDrivenShaderPlatformInfo::GetSupportsUInt64ImageAtomics(View.GetShaderPlatform());
			FAPSStarRasterCS::FPermutationDomain Permutation64;
			Permutation64.Set<FAPSStarRasterCS::FAtomic64Dim>(true);
			FAPSStarRasterCS::FPermutationDomain Permutation32;
			Permutation32.Set<FAPSStarRasterCS::FAtomic64Dim>(false);
			// Rio 03.10 08:17: every compute pipeline is created once through the RHI before its first dispatch
			// (EnsureComputePipeline). A failed 64-bit one falls back to the 32-bit path; a failed required one turns the
			// feature off with the frame unchanged.
			bool bUse64 = bWant64
				&& EnsureComputePipeline(FindGlobalShader<FAPSStarClearCS>(ShaderMap, Permutation64.ToDimensionValueId()),
					TEXT("clear (64-bit)"))
				&& EnsureComputePipeline(FindGlobalShader<FAPSStarRasterCS>(ShaderMap, Permutation64.ToDimensionValueId()),
					TEXT("raster (64-bit)"))
				&& EnsureComputePipeline(FindGlobalShader<FAPSStarCompositeCS>(ShaderMap, Permutation64.ToDimensionValueId()),
					TEXT("composite (64-bit)"));
			if (bWant64 != bUse64)
			{
				LogOnce(bLoggedAtomics, TEXT("64-bit atomic shaders or pipelines unavailable, using the 32-bit point path"));
			}
			const int32 PermutationId = (bUse64 ? Permutation64 : Permutation32).ToDimensionValueId();
			const TShaderRef<FAPSStarClearCS> ClearShader = FindGlobalShader<FAPSStarClearCS>(ShaderMap, PermutationId);
			const TShaderRef<FAPSStarRasterCS> RasterShader = FindGlobalShader<FAPSStarRasterCS>(ShaderMap, PermutationId);
			const TShaderRef<FAPSStarCompositeCS> CompositeShader = FindGlobalShader<FAPSStarCompositeCS>(ShaderMap, PermutationId);
			const TShaderRef<FAPSStarClearCS> Clear32Shader = FindGlobalShader<FAPSStarClearCS>(ShaderMap, Permutation32.ToDimensionValueId());
			const TShaderRef<FAPSStarCompositeCS> Composite32Shader =
				FindGlobalShader<FAPSStarCompositeCS>(ShaderMap, Permutation32.ToDimensionValueId());
			const TShaderRef<FAPSGlowRaymarchCS> GlowShader = FindGlobalShader<FAPSGlowRaymarchCS>(ShaderMap, 0);
			if (!EnsureComputePipeline(ClearShader, TEXT("clear")) || !EnsureComputePipeline(RasterShader, TEXT("raster"))
				|| !EnsureComputePipeline(CompositeShader, TEXT("composite"))
				|| !EnsureComputePipeline(Clear32Shader, TEXT("clear (32-bit)"))
				|| !EnsureComputePipeline(Composite32Shader, TEXT("composite (32-bit)")))
			{
				GShadersMissingAtRuntime.store(true);
				LogOnce(bLoggedMissingShaders, TEXT("plugin shaders or their pipelines are unavailable (aps.Stars.CompileShaders=0 at ")
					TEXT("start-up, a compile error, or a pipeline the RHI rejected): GPU points and glow stay off, the frame is unchanged"));
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}
			GUsingAtomic64.store(bUse64);

			RDG_EVENT_SCOPE(GraphBuilder, "APS.Stars");
			RDG_GPU_STAT_SCOPE(GraphBuilder, APSGpuStars);

			// View frame. Post-TSR scene colour is unjittered: project with the NoAA matrices.
			const FViewMatrices& Matrices = View.ViewMatrices;
			const FMatrix ProjectionNoAA = Matrices.GetProjectionNoAAMatrix();
			FViewFrame Frame;
			Frame.TranslatedWorldToClip = Matrices.GetTranslatedViewMatrix() * ProjectionNoAA;
			Frame.InvProjection = ProjectionNoAA.Inverse();
			Frame.ViewToTranslatedWorld = WithoutTranslation(Matrices.GetInvTranslatedViewMatrix());
			Frame.ViewOrigin = Matrices.GetViewOrigin();
			Frame.PreViewTranslation = Matrices.GetPreViewTranslation();
			Frame.TargetSize = SceneColor.ViewRect.Size();
			const double P00 = FMath::Abs(ProjectionNoAA.M[0][0]);
			const double P11 = FMath::Abs(ProjectionNoAA.M[1][1]);
			Frame.PixelSolidAngle = 4.0 / FMath::Max(P00 * P11 * Frame.TargetSize.X * Frame.TargetSize.Y, 1.0e-30);
			if (Frame.TargetSize.X <= 0 || Frame.TargetSize.Y <= 0)
			{
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}
			const FRDGTextureRef SceneDepth = Inputs.SceneTextures.SceneTextures->GetParameters()->SceneDepthTexture;
			if (SceneDepth == nullptr)
			{
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}

			// ---- Points ------------------------------------------------------------------------------------------
			FRDGTextureRef StarBuffer = nullptr;
			bool bHasStars = false;
			int64 Submitted = 0;
			if (Sets.Num() > 0)
			{
				const EPixelFormat Format64 = GPixelFormats[PF_R64_UINT].Supported ? PF_R64_UINT : PF_R32G32_UINT;
				StarBuffer = bUse64
					? GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Frame.TargetSize, Format64, FClearValueBinding::None,
						TexCreate_ShaderResource | TexCreate_UAV | ETextureCreateFlags::Atomic64Compatible), TEXT("APS.Stars.Buffer64"))
					: GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Frame.TargetSize, PF_R32_UINT, FClearValueBinding::None,
						TexCreate_ShaderResource | TexCreate_UAV | TexCreate_AtomicCompatible), TEXT("APS.Stars.Buffer32"));
				const FRDGTextureUAVRef StarUAV = GraphBuilder.CreateUAV(StarBuffer);
				{
					FAPSStarClearCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAPSStarClearCS::FParameters>();
					Parameters->TargetSize = FUintVector2(Frame.TargetSize.X, Frame.TargetSize.Y);
					if (bUse64)
					{
						Parameters->OutStarBuffer64 = StarUAV;
					}
					else
					{
						Parameters->OutStarBuffer32 = StarUAV;
					}
					FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("APS.Stars.Clear"), ClearShader, Parameters,
						FComputeShaderUtils::GetGroupCount(Frame.TargetSize, 8));
				}

				const bool bStats = CVarGpuStats.GetValueOnRenderThread() != 0;
				const FRDGBufferRef StatsBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 4),
					TEXT("APS.Stars.Stats"));
				const FRDGBufferUAVRef StatsUAV = GraphBuilder.CreateUAV(StatsBuffer);
				AddClearUAVPass(GraphBuilder, StatsUAV, 0u);

				const float GlobalIntensity = FMath::Max(CVarGpuPointIntensity.GetValueOnRenderThread(), 0.0f);
				const float MinPixel = FMath::Max(CVarGpuPointMinPixel.GetValueOnRenderThread(), 1.0e-7f);
				const float MaxPixel = FMath::Max(CVarGpuPointMaxPixel.GetValueOnRenderThread(), MinPixel);
				int64 Budget = FMath::Max<int64>(CVarGpuPointBudget.GetValueOnRenderThread(), 0);

				for (FPointSetRT* Set : Sets)
				{
					const FRDGBufferRef PointBuffer = PrepareSetBuffer(GraphBuilder, *Set, FrameNumber);
					const int32 Count = static_cast<int32>(FMath::Min<int64>(Set->NumUploaded, Budget));
					if (PointBuffer == nullptr || Count <= 0)
					{
						continue;
					}
					const FPresentation Presentation = MakePresentation(Set->Desc.LocalToWorld, Set->Desc.FarEnvelope, Frame);
					const FVector3f BoundsSize = Set->Desc.LocalBounds.Max - Set->Desc.LocalBounds.Min;
					if (!Presentation.bValid || !Set->Desc.LocalBounds.IsValid || BoundsSize.ContainsNaN())
					{
						continue;
					}
					const double Brightness = static_cast<double>(Set->Desc.IntensityScale) * Set->Desc.Visibility * SceneVisibility
						* GlobalIntensity / Frame.PixelSolidAngle;
					if (!FMath::IsFinite(Brightness) || !(Brightness > 0.0))
					{
						continue;
					}
					Budget -= Count;
					Submitted += Count;

					const int32 Groups = FMath::Clamp(FMath::DivideAndRoundUp(Count, FAPSStarRasterCS::GroupSize), 1,
						FAPSStarRasterCS::MaxGroups);
					FAPSStarRasterCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAPSStarRasterCS::FParameters>();
					Parameters->View = View.ViewUniformBuffer;
					Parameters->LocalRotation = Presentation.LocalRotation;
					Parameters->TranslatedWorldToClip = FMatrix44f(Frame.TranslatedWorldToClip);
					Parameters->BoundsMin = Set->Desc.LocalBounds.Min;
					Parameters->SetBrightness = static_cast<float>(Brightness);
					Parameters->BoundsStep = BoundsSize / 65535.0f;
					Parameters->MinPixelValue = MinPixel;
					Parameters->CameraLocal = Presentation.CameraLocal;
					Parameters->MaxPixelValue = MaxPixel;
					Parameters->ObserverTranslated = Presentation.ObserverTranslated;
					// Distances below one quantisation step are not resolvable: clamp the inverse square there, or at the
					// set's brightness floor (gameplay sky, Rio 03.10) when that is farther.
					const float MinDistance = FMath::Max3(BoundsSize.GetMax() / 65535.0f, 1.0e-20f,
						FMath::IsFinite(Set->Desc.BrightnessFloorDistanceLocal) ? Set->Desc.BrightnessFloorDistanceLocal : 0.0f);
					Parameters->MinDistanceSq = MinDistance * MinDistance;
					uint32 NumSpheres = 0;
					const auto AddExclusion = [Parameters, &NumSpheres](const FVector3f& Center, const float Radius)
					{
						if (NumSpheres < static_cast<uint32>(MaxExclusionSpheres) && Radius > 0.0f && FMath::IsFinite(Radius)
							&& !Center.ContainsNaN())
						{
							Parameters->ExclusionSpheres[NumSpheres++] = FVector4f(Center, Radius * Radius);
						}
					};
					AddExclusion(Set->Desc.ExclusionCenterLocal, Set->Desc.ExclusionRadiusLocal);
					for (const FVector4f& Sphere : Set->Desc.ExtraExclusionSpheresLocal)
					{
						AddExclusion(FVector3f(Sphere.X, Sphere.Y, Sphere.Z), Sphere.W);
					}
					for (uint32 Unused = NumSpheres; Unused < static_cast<uint32>(MaxExclusionSpheres); ++Unused)
					{
						Parameters->ExclusionSpheres[Unused] = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
					}
					Parameters->NumExclusionSpheres = NumSpheres;
					Parameters->TargetSizeF = FVector2f(static_cast<float>(Frame.TargetSize.X), static_cast<float>(Frame.TargetSize.Y));
					Parameters->CompressA = static_cast<float>(Presentation.CompressA);
					Parameters->CompressB = static_cast<float>(Presentation.CompressB);
					Parameters->TargetSize = FUintVector2(Frame.TargetSize.X, Frame.TargetSize.Y);
					Parameters->PointOffset = 0u;
					Parameters->PointCount = static_cast<uint32>(Count);
					Parameters->ThreadCount = static_cast<uint32>(Groups * FAPSStarRasterCS::GroupSize);
					Parameters->bWriteStats = bStats ? 1u : 0u;
					Parameters->Points = GraphBuilder.CreateSRV(PointBuffer);
					Parameters->SceneDepthTexture = SceneDepth;
					Parameters->OutStats = StatsUAV;
					if (bUse64)
					{
						Parameters->OutStarBuffer64 = StarUAV;
					}
					else
					{
						Parameters->OutStarBuffer32 = StarUAV;
					}
					FComputeShaderUtils::AddPass(GraphBuilder,
						RDG_EVENT_NAME("APS.Stars.Raster %s (%d points)", *Set->Desc.DebugName, Count),
						RasterShader, Parameters, FIntVector(Groups, 1, 1));
					bHasStars = true;
					if (Budget <= 0)
					{
						break;
					}
				}

				if (bStats && bHasStars)
				{
					if (FRHIGPUBufferReadback* Readback = AcquireReadback(State, FrameNumber))
					{
						AddEnqueueCopyPass(GraphBuilder, Readback, StatsBuffer, 4 * sizeof(uint32));
					}
				}
			}
			PublishFrameCounters(State, FrameNumber, Submitted);

			// ---- Glow --------------------------------------------------------------------------------------------
			FRDGTextureRef GlowTexture = nullptr;
			if (Volume != nullptr && EnsureComputePipeline(GlowShader, TEXT("glow ray march")))
			{
				FGlowMapTextures Maps;
				const FPresentation Presentation = MakePresentation(Volume->Desc.LocalToWorld, Volume->Desc.FarEnvelope, Frame);
				const double ExtentXY = Volume->Map.ExtentXY;
				if (Presentation.bValid && ExtentXY > 0.0 && PrepareGlowMaps(GraphBuilder, *Volume, ShaderMap, Maps))
				{
					const int32 Divisor = FMath::Clamp(CVarGalaxyGlowResolution.GetValueOnRenderThread(), 1, 8);
					const FIntPoint GlowSize(FMath::DivideAndRoundUp(Frame.TargetSize.X, Divisor),
						FMath::DivideAndRoundUp(Frame.TargetSize.Y, Divisor));
					GlowTexture = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(GlowSize, PF_FloatRGBA, FClearValueBinding::Black,
						TexCreate_ShaderResource | TexCreate_UAV), TEXT("APS.Glow"));

					// Map space = local / ExtentXY; rotations only, the march normalises its direction.
					const FMatrix WorldToLocalRotation = Volume->Desc.LocalToWorld.GetRotation().Inverse().ToMatrix();
					const FMatrix ViewToMap = Frame.ViewToTranslatedWorld * WorldToLocalRotation;
					// Photometry in local units: L = TotalIntensity * EmissionNorm / ExtentXY^2 * (map integral).
					const double EmissionScale = Volume->Desc.TotalIntensity * Volume->Map.EmissionNorm / (ExtentXY * ExtentXY)
						* Volume->Desc.Visibility * SceneVisibility * FMath::Max(CVarGalaxyGlowIntensity.GetValueOnRenderThread(), 0.0f);

					FAPSGlowRaymarchCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAPSGlowRaymarchCS::FParameters>();
					Parameters->View = View.ViewUniformBuffer;
					Parameters->InvProjection = FMatrix44f(Frame.InvProjection);
					Parameters->ViewToMap = FMatrix44f(ViewToMap);
					Parameters->CameraMap = Presentation.CameraLocal / static_cast<float>(ExtentXY);
					Parameters->EmissionScale = static_cast<float>(FMath::IsFinite(EmissionScale) ? EmissionScale : 0.0);
					Parameters->VolumeParams = FVector4f(Volume->Map.ExtentZ, FMath::Max(Volume->Desc.DustOpacity, 0.0f),
						FMath::Max(Volume->Desc.DustHeightScale, 0.0f), FMath::Max(Volume->Desc.DustHeightMax, 1.0e-5f));
					Parameters->GlowSizeF = FVector2f(static_cast<float>(GlowSize.X), static_cast<float>(GlowSize.Y));
					Parameters->CompressA = static_cast<float>(Presentation.CompressA * ExtentXY);
					Parameters->CompressB = static_cast<float>(Presentation.CompressB * ExtentXY);
					Parameters->GlowSize = FUintVector2(GlowSize.X, GlowSize.Y);
					Parameters->StepCount = static_cast<uint32>(FMath::Clamp(CVarGalaxyGlowSteps.GetValueOnRenderThread(), 8, 256));
					Parameters->MinScaleHeight = 1.0e-4f;
					// The point sets' brightness floor (gameplay sky), in map units.
					const double FloorMap = FMath::IsFinite(Volume->Desc.BrightnessFloorDistanceLocal)
						? FMath::Max(static_cast<double>(Volume->Desc.BrightnessFloorDistanceLocal), 0.0) / ExtentXY : 0.0;
					Parameters->FloorDistanceSq = static_cast<float>(FloorMap * FloorMap);
					Parameters->SceneDepthTexture = SceneDepth;
					Parameters->EmissionMap = Maps.Emission;
					Parameters->ProfileMap = Maps.Profile;
					Parameters->ShapeMap = Maps.Shape;
					Parameters->MapSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
					Parameters->OutGlow = GraphBuilder.CreateUAV(GlowTexture);
					FComputeShaderUtils::AddPass(GraphBuilder,
						RDG_EVENT_NAME("APS.Glow.RayMarch %dx%d", GlowSize.X, GlowSize.Y),
						GlowShader, Parameters, FComputeShaderUtils::GetGroupCount(GlowSize, 8));
				}
			}

			if (!bHasStars && GlowTexture == nullptr)
			{
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}

			// ---- Composite ---------------------------------------------------------------------------------------
			// A compute pass into a new texture (typed UAV store, as TSR writes this format). Without points the 32-bit
			// composite reads a 1x1 dummy star buffer.
			const bool bComposite64 = bHasStars && bUse64;
			const TShaderRef<FAPSStarCompositeCS> Composite = bComposite64 ? CompositeShader : Composite32Shader;
			const FRDGTextureDesc& InputDesc = SceneColor.Texture->Desc;
			if (!Composite.IsValid() || !RHIIsTypedUAVStoreSupported(InputDesc.Format))
			{
				static bool bLoggedFormat = false;
				LogOnce(bLoggedFormat, TEXT("the scene colour format has no typed UAV store: GPU points and glow are skipped"));
				return PassThrough(GraphBuilder, View, SceneColor, Inputs);
			}
			if (!bHasStars)
			{
				StarBuffer = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(FIntPoint(1, 1), PF_R32_UINT, FClearValueBinding::None,
					TexCreate_ShaderResource | TexCreate_UAV | TexCreate_AtomicCompatible), TEXT("APS.Stars.Empty"));
				FAPSStarClearCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAPSStarClearCS::FParameters>();
				Parameters->TargetSize = FUintVector2(1, 1);
				Parameters->OutStarBuffer32 = GraphBuilder.CreateUAV(StarBuffer);
				FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("APS.Stars.ClearEmpty"), Clear32Shader, Parameters,
					FIntVector(1, 1, 1));
			}

			// A new texture: the TSR output is also its history and must never be modified.
			const FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(InputDesc.Extent,
				InputDesc.Format, FClearValueBinding::Black, TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable),
				TEXT("APS.Stars.SceneColor"));
			const FIntRect OutputRect = SceneColor.ViewRect;

			const float CoreSigma = FMath::Clamp(CVarGpuPointCoreSigma.GetValueOnRenderThread(), 0.3f, 2.0f);
			const float HaloShare = FMath::Clamp(CVarGpuPointHalo.GetValueOnRenderThread(), 0.0f, 0.9f);
			const float MinPixel = FMath::Max(CVarGpuPointMinPixel.GetValueOnRenderThread(), 1.0e-7f);

			FAPSStarCompositeCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAPSStarCompositeCS::FParameters>();
			for (int32 Index = 0; Index < 256; ++Index)
			{
				Parameters->StarPalette[Index] = State.Palette[Index];
			}
			Parameters->InputRect = FIntVector4(SceneColor.ViewRect.Min.X, SceneColor.ViewRect.Min.Y,
				SceneColor.ViewRect.Width(), SceneColor.ViewRect.Height());
			Parameters->OutputRect = FIntVector4(OutputRect.Min.X, OutputRect.Min.Y,
				FMath::Max(OutputRect.Width(), 1), FMath::Max(OutputRect.Height(), 1));
			Parameters->PsfParams = FVector4f(CoreSigma, 0.9f, 1.6f, HaloShare);
			// Halo starts growing 16x above the visibility cut and is full 8 stops later.
			Parameters->CompositeParams = FVector4f(1.0f / (16.0f * MinPixel),
				FMath::Clamp(CVarGalaxyGlowDustOnScene.GetValueOnRenderThread(), 0.0f, 1.0f),
				FMath::Clamp(CVarGalaxyGlowDustOnPoints.GetValueOnRenderThread(), 0.0f, 1.0f),
				FMath::Max(CVarGalaxyGlowMax.GetValueOnRenderThread(), 0.0f));
			Parameters->bHasStars = bHasStars ? 1u : 0u;
			Parameters->bHasGlow = GlowTexture != nullptr ? 1u : 0u;
			Parameters->DebugMode = static_cast<uint32>(FMath::Clamp(CVarGpuDebugView.GetValueOnRenderThread(), 0, 3));
			Parameters->PsfRadius = FMath::Clamp(CVarGpuPointPsfRadius.GetValueOnRenderThread(), 1, 3);
			Parameters->SceneColorTexture = SceneColor.Texture;
			if (bComposite64)
			{
				Parameters->StarBuffer64 = StarBuffer;
			}
			else
			{
				Parameters->StarBuffer32 = StarBuffer;
			}
			Parameters->GlowTexture = GlowTexture != nullptr ? GlowTexture : GSystemTextures.GetBlackDummy(GraphBuilder);
			Parameters->GlowSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			Parameters->OutSceneColor = GraphBuilder.CreateUAV(OutputTexture);

			FComputeShaderUtils::AddPass(GraphBuilder,
				RDG_EVENT_NAME("APS.Stars.Composite %dx%d", OutputRect.Width(), OutputRect.Height()), Composite, Parameters,
				FComputeShaderUtils::GetGroupCount(OutputRect.Size(), FAPSStarCompositeCS::GroupSize));
			// When this is the chain's last pass the engine names the output: copied with the engine's own draw pass.
			return PassThrough(GraphBuilder, View, FScreenPassTexture(OutputTexture, OutputRect), Inputs);
		}
	}
}

FAPSStarViewExtension::FAPSStarViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

bool FAPSStarViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	using namespace APSStarRenderer::Private;
	const bool bPoints = CVarGpuPoints.GetValueOnAnyThread() != 0;
	const bool bGlow = CVarGalaxyGlow.GetValueOnAnyThread() != 0;
	if (!bPoints && !bGlow)
	{
		return false;
	}
	if (!ShouldCompileShaders() || GShadersMissingAtRuntime.load())
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			UE_LOG(LogAPSStarRenderer, Warning,
				TEXT("[APS.GpuStars] aps.Stars.GpuPoints/GalaxyGlow are on, but the plugin shaders are not available. ")
				TEXT("Set aps.Stars.CompileShaders=1 in [SystemSettings] and restart (see Docs/Design/GALAXY_GPU_STARS.md)."));
		}
		return false;
	}
	const FSceneInterface* Scene = Context.Scene;
	if (Scene == nullptr)
	{
		const UWorld* World = Context.GetWorld();
		Scene = World != nullptr ? World->Scene : nullptr;
	}
	return FRegistry::Get().HasEnabledEntries(Scene, bPoints, bGlow);
}

void FAPSStarViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks,
	bool /*bIsPassEnabled*/)
{
	if (Pass == EPostProcessingPass::MotionBlur)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FAPSStarViewExtension::PostMotionBlur_RenderThread));
	}
}

FScreenPassTexture FAPSStarViewExtension::PostMotionBlur_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
	const FPostProcessMaterialInputs& Inputs)
{
	return APSStarRenderer::Private::AddStarPasses(GraphBuilder, View, Inputs);
}
