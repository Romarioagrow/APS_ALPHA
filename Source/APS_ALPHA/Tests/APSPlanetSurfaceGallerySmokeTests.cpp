#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "AssetCompilingManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "UnrealClient.h"

namespace APSPlanetSurfaceGallerySmokeTests
{
	constexpr double WholeTestTimeoutSeconds = 1200.0;
	constexpr double SurfaceTimeoutSeconds = 150.0;
	constexpr double ScreenshotTimeoutSeconds = 10.0;
	constexpr double CleanupTimeoutSeconds = 20.0;
	constexpr double WarmSampleSeconds = 0.85;
	constexpr double MinimumWarmFps = 75.0;
	constexpr double MaximumFrameMilliseconds = 250.0;
	constexpr double MinimumPlanetMeanBrightness = 4.0;
	constexpr double MinimumPlanetBrightnessVariance = 8.0;
	constexpr double MinimumPlanetNonBlackRatio = 0.10;
	constexpr double MaximumPlanetClippedRatio = 0.40;
	constexpr double MaximumBackgroundMeanBrightness = 64.0;
	constexpr double MaximumBackgroundClippedRatio = 0.10;
	bool InheritsExpectedWorldScapeTerrainTemplate(UMaterialInterface* Material,
		const EAPSPlanetSurfaceArchetype /*Archetype*/)
	{
		// Every solid family now resolves through one project-owned WorldScape master.
		// The old Terra/Selenae/Magma ancestry check made a ready 10/10 live payload
		// wait for the full timeout after the canonical material migration.
		if (!IsValid(Material) || Material->GetBlendMode() != BLEND_Opaque)
		{
			return false;
		}
		const UMaterial* Master = Material->GetMaterial();
		return IsValid(Master)
			&& Master->GetPathName().Contains(TEXT("M_APS_WorldScapeTerrain"));
	}

	bool HasCompleteCenteredWorldScapeSurface(AWorldScapeRoot* Root)
	{
		if (!IsValid(Root) || Root->WorldScapeLod.Num() != Root->MaxLod
			|| Root->WorldScapeLodInGeneration.Num() != 0)
		{
			return false;
		}
		const FVector DesiredNormal = Root->WorldToECEF(
			Root->OverridedPlayerPosition).ToFVector().GetSafeNormal();
		if (DesiredNormal.IsNearlyZero())
		{
			return false;
		}
		for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
		{
			if (!APSWorldScapePayloadValidation::HasCompleteCenteredPayload(
				Lod, DesiredNormal, true))
			{
				return false;
			}
		}
		if (!Root->bOcean)
		{
			return true;
		}
		if (Root->WorldScapeLodOcean.Num() != Root->OceanMaxLod)
		{
			return false;
		}
		for (const UWorldScapeLod* Lod : Root->WorldScapeLodOcean)
		{
			if (!APSWorldScapePayloadValidation::HasCompleteCenteredPayload(
				Lod, DesiredNormal, false))
			{
				return false;
			}
		}
		return true;
	}

	struct FGalleryCase
	{
		EPlanetType PlanetType;
		const TCHAR* FileToken;
		const TCHAR* DisplayName;
	};

	struct FSavedConsoleVariableState
	{
		IConsoleVariable* Variable{nullptr};
		FString Value;
		EConsoleVariableFlags Flags{ECVF_Default};
	};

	// These are deliberately concrete EPlanetType presets, not archetype-only
	// probes. They cover every surface family called out in the visual regression
	// report plus both large-continent temperate variants.
	const FGalleryCase GalleryCases[] =
	{
		{EPlanetType::Ice, TEXT("Ice"), TEXT("ICE / FROZEN")},
		{EPlanetType::Desert, TEXT("Desert"), TEXT("DESERT")},
		{EPlanetType::Water, TEXT("Water"), TEXT("OCEAN / WATER")},
		{EPlanetType::Forest, TEXT("Forest"), TEXT("FOREST / BIOSPHERE")},
		{EPlanetType::Metallic, TEXT("Metallic"), TEXT("METALLIC")},
		{EPlanetType::Lava, TEXT("Lava"), TEXT("LAVA / VOLCANIC")},
		{EPlanetType::Pangea, TEXT("Pangea"), TEXT("PANGEA / TEMPERATE")},
		{EPlanetType::HighMountain, TEXT("HighMountain"), TEXT("HIGH MOUNTAIN")},
	};

	AAstroGenerator* FindPreviewGenerator(UWorld* World)
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsValid(*It) && It->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return *It;
			}
		}
		return nullptr;
	}

	APlanetarySurfaceGenerator* FindPreviewSurfaceGenerator(
		UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		for (TActorIterator<APlanetarySurfaceGenerator> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetOwner() == PreviewGenerator)
			{
				return *It;
			}
		}
		return nullptr;
	}

	enum class EGalleryStep : uint8
	{
		Startup,
		WaitForSurface,
		WarmSurface,
		CaptureSurface,
		Cleanup,
	};

	class FRenderedPlanetSurfaceGalleryCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FRenderedPlanetSurfaceGalleryCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (TestStartSeconds <= 0.0)
			{
				TestStartSeconds = Now;
				StepStartSeconds = Now;
			}
			if (Step != EGalleryStep::Cleanup
				&& Now - TestStartSeconds > WholeTestTimeoutSeconds)
			{
				return Fail(TEXT("Rendered planet surface gallery timed out"));
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;

			switch (Step)
			{
			case EGalleryStep::Startup:
				return UpdateStartup(World, Controller, ViewModel, Now);
			case EGalleryStep::WaitForSurface:
				return UpdateWaitForSurface(World, ViewModel, Now);
			case EGalleryStep::WarmSurface:
				return UpdateWarmSurface(World, ViewModel, Now);
			case EGalleryStep::CaptureSurface:
				return UpdateCaptureSurface(World, Controller, ViewModel, Now);
			case EGalleryStep::Cleanup:
				return UpdateCleanup(Now);
			default:
				return true;
			}
		}

	private:
		void PrepareCleanRenderWarmup()
		{
			if (bWarmupPrepared)
			{
				return;
			}
			bWarmupPrepared = true;

			for (const TCHAR* CVarName : {TEXT("r.VSync"), TEXT("rhi.SyncInterval"),
				TEXT("t.MaxFPS"), TEXT("t.IdleWhenNotForeground"),
				TEXT("r.DontLimitOnBattery")})
			{
				if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(CVarName))
				{
					FSavedConsoleVariableState& Saved = SavedConsoleVariables.AddDefaulted_GetRef();
					Saved.Variable = CVar;
					Saved.Value = CVar->GetString();
					Saved.Flags = CVar->GetFlags();
					CVar->Set(FCString::Strcmp(CVarName, TEXT("r.DontLimitOnBattery")) == 0 ? 1 : 0,
						ECVF_SetByCode);
				}
			}

			UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
				TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
			const auto KeepResident = [this](UObject* Asset)
			{
				if (IsValid(Asset) && !Asset->IsRooted())
				{
					Asset->AddToRoot();
					WarmedAssets.Add(Asset);
				}
			};
			KeepResident(Catalog);
			if (Catalog)
			{
				for (const TPair<EAPSPlanetSurfaceArchetype,
					FAPSPlanetSurfaceArchetypeDefinition>& Entry : Catalog->Archetypes)
				{
					KeepResident(Entry.Value.TerrainMaterial.LoadSynchronous());
					KeepResident(Entry.Value.OceanMaterial.LoadSynchronous());
				}
			}
		}

		bool WaitForCompilationIdle(double Now, const FString& Context)
		{
			if (CompilationContext != Context)
			{
				CompilationContext = Context;
				CompilationIdleSince = 0.0;
			}
			if (FAssetCompilingManager::Get().GetNumRemainingAssets() > 0)
			{
				CompilationIdleSince = 0.0;
				return false;
			}
			if (CompilationIdleSince <= 0.0)
			{
				CompilationIdleSince = Now;
				return false;
			}
			return Now - CompilationIdleSince >= 0.75;
		}

		bool UpdateStartup(UWorld* World, AMainMenuController* Controller,
			UWorldGenerationViewModel* ViewModel, double Now)
		{
			if (!World || !Controller || !ViewModel || !ViewModel->GeneratedWorld)
			{
				return false;
			}
			PrepareCleanRenderWarmup();
			if (!WaitForCompilationIdle(Now, TEXT("GALLERY_MENU_ASSETS")))
			{
				return false;
			}

			UGeneratedWorld* Model = ViewModel->GeneratedWorld;
			Model->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
			Model->bGenerateFullScaledWorld = true;
			Model->bGenerateHomeSystem = true;
			Model->bStartWithHomePlanet = true;
			Model->PlanetsAmount = 3;
			Model->MoonsAmount = 1;
			Model->StartPlanetIndex = 1;
			Model->PlanetRadius = 6750.0;
			Model->PlanetType = GalleryCases[0].PlanetType;
			Model->PlanetSurfaceSeed = 73991;
			Model->SurfaceFeatureScale = 1.0;
			Model->SurfaceReliefScale = 1.0;
			Model->SurfaceLandCoverageScale = 1.0;
			Model->SurfaceMountainScale = 1.0;
			Model->SurfaceCraterScale = 1.0;
			Model->SurfaceRoughnessScale = 1.0;
			Model->AtmosphereHeight = 120.0;
			Model->AtmosphereOpacity = 5.0;

			BeginOperationTracking();
			if (!Controller->OpenAstronomicalGenerationForAutomation(
				EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Space))
			{
				return false;
			}

			CurrentCaseIndex = 0;
			RevisionBeforeCase = ViewModel->PreviewRevision;
			CompilationContext.Reset();
			Step = EGalleryStep::WaitForSurface;
			StepStartSeconds = Now;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Gallery] Opened real PLANET page; cases=%d"), UE_ARRAY_COUNT(GalleryCases));
			return false;
		}

		bool UpdateWaitForSurface(UWorld* World,
			UWorldGenerationViewModel* ViewModel, double Now)
		{
			TrackOperationFrame();
			if (Now - StepStartSeconds > SurfaceTimeoutSeconds)
			{
				RecordCaseFailure(FString::Printf(TEXT("surface did not become ready in %.0fs"),
					SurfaceTimeoutSeconds));
				Step = EGalleryStep::CaptureSurface;
				StepStartSeconds = Now;
				return false;
			}
			if (!World || !ViewModel)
			{
				return false;
			}

			if (!PreviewGenerator.IsValid())
			{
				PreviewGenerator = FindPreviewGenerator(World);
			}
			AAstroGenerator* Generator = PreviewGenerator.Get();
			if (!ViewModel->bPreviewReady || !IsValid(Generator)
				|| ViewModel->GetPreviewFocus() != EAstroPreviewFocus::HomePlanet)
			{
				return false;
			}

			APlanet* Planet = Cast<APlanet>(Generator->GetActivePreviewWorldScapeBody());
			APlanetarySurfaceGenerator* Surface = FindPreviewSurfaceGenerator(World, Generator);
			AWorldScapeRoot* Root = Surface ? Surface->WorldScapeRootInstance : nullptr;
			const FGalleryCase& GalleryCase = GalleryCases[CurrentCaseIndex];
			const bool bRevisionReady = CurrentCaseIndex == 0
				|| ViewModel->PreviewRevision > RevisionBeforeCase;
			const bool bProfileReady = IsValid(Planet) && Planet->PlanetType == GalleryCase.PlanetType
				&& Planet->bWorldScapeSurfaceReady && Surface && Root
				&& Surface->IsSurfaceProfileCurrent(Planet)
				&& Surface->ResolvedSurfaceProfile.PlanetType == GalleryCase.PlanetType
				&& IsValid(Surface->ResolvedNoiseInstance)
				&& IsValid(Surface->ResolvedTerrainMaterialInstance)
				&& !Root->IsHidden()
				&& Root->TerrainMaterial.DefaultMaterial
					== Surface->ResolvedTerrainMaterialInstance
				&& InheritsExpectedWorldScapeTerrainTemplate(
					Surface->ResolvedTerrainMaterialInstance,
					Surface->ResolvedSurfaceProfile.Archetype)
				&& HasCompleteCenteredWorldScapeSurface(Root);
			if (!bRevisionReady || !bProfileReady)
			{
				return false;
			}

			const FString CompilationLabel = FString::Printf(TEXT("GALLERY_%s"),
				GalleryCase.FileToken);
			if (!WaitForCompilationIdle(Now, CompilationLabel))
			{
				return false;
			}

			if (!ValidateCommittedSurface(Planet, Surface, Root, GalleryCase))
			{
				RecordCaseFailure(TEXT("committed an incomplete canonical WorldScape surface"));
			}
			AssertOperationFrameBudget(GalleryCase);
			BeginWarmSample();
			Step = EGalleryStep::WarmSurface;
			StepStartSeconds = Now;
			return false;
		}

		bool ValidateCommittedSurface(APlanet* Planet,
			APlanetarySurfaceGenerator* Surface, AWorldScapeRoot* Root,
			const FGalleryCase& GalleryCase)
		{
			bool bValid = true;
			bValid &= Test->TestTrue(FString::Printf(TEXT("%s uses the WorldScape resolver pipeline"),
				GalleryCase.DisplayName),
				UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(GalleryCase.PlanetType));
			bValid &= Test->TestFalse(FString::Printf(TEXT("%s presents the real WorldScape root"),
				GalleryCase.DisplayName), Root->IsHidden());
			bValid &= Test->TestFalse(FString::Printf(TEXT("%s ready WorldScape root no longer ticks"),
				GalleryCase.DisplayName), Root->IsActorTickEnabled());
			bValid &= Test->TestFalse(FString::Printf(TEXT("%s ready WorldScape root is frozen"),
				GalleryCase.DisplayName), Root->bGenerateWorldScape);
			bValid &= Test->TestEqual(FString::Printf(TEXT("%s has no WorldScape workers"),
				GalleryCase.DisplayName), Root->WorldScapeLodInGeneration.Num(), 0);
			bValid &= Test->TestTrue(FString::Printf(
				TEXT("%s owns one complete centered WorldScape payload"),
				GalleryCase.DisplayName), HasCompleteCenteredWorldScapeSurface(Root));
			bValid &= Test->TestEqual(FString::Printf(
				TEXT("%s real WorldScape root uses the exact resolver terrain MID"),
				GalleryCase.DisplayName), Root->TerrainMaterial.DefaultMaterial,
				static_cast<UMaterialInterface*>(Surface->ResolvedTerrainMaterialInstance));
			bValid &= Test->TestTrue(FString::Printf(
				TEXT("%s resolved MID inherits its archetype WorldScape terrain template"),
				GalleryCase.DisplayName), InheritsExpectedWorldScapeTerrainTemplate(
					Surface->ResolvedTerrainMaterialInstance,
					Surface->ResolvedSurfaceProfile.Archetype));

			int32 CompleteTerrainLods = 0;
			float MinimumHeightChannel = TNumericLimits<float>::Max();
			float MaximumHeightChannel = -TNumericLimits<float>::Max();
			for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
			{
				if (!APSWorldScapePayloadValidation::HasCompletePayload(Lod, true))
				{
					continue;
				}
				++CompleteTerrainLods;
				for (const FLinearColor& VertexColor : Lod->VertexColors)
				{
					MinimumHeightChannel = FMath::Min(MinimumHeightChannel, VertexColor.R);
					MaximumHeightChannel = FMath::Max(MaximumHeightChannel, VertexColor.R);
				}
			}
			bValid &= Test->TestEqual(FString::Printf(
				TEXT("%s commits every real WorldScape LOD payload"), GalleryCase.DisplayName),
				CompleteTerrainLods, Root->WorldScapeLod.Num());
			bValid &= Test->TestTrue(FString::Printf(
				TEXT("%s real WorldScape height channel is not flat"), GalleryCase.DisplayName),
				MaximumHeightChannel > MinimumHeightChannel);

			UProceduralMeshComponent* TerrainProxy =
				PreviewGenerator->GetActivePreviewTerrainProxy();
			const bool bCustomProxyVisible = IsValid(TerrainProxy)
				&& TerrainProxy->IsVisible() && !TerrainProxy->bHiddenInGame;
			bValid &= Test->TestFalse(FString::Printf(
				TEXT("%s never treats the selected-body custom proxy as the ready surface"),
				GalleryCase.DisplayName), bCustomProxyVisible);

			if (Root->bOcean)
			{
				bValid &= Test->TestEqual(FString::Printf(
					TEXT("%s WorldScape root retains the resolver liquid MID"),
					GalleryCase.DisplayName), Root->OceanMaterial.DefaultMaterial,
					static_cast<UMaterialInterface*>(Surface->ResolvedOceanMaterialInstance));
				bValid &= Test->TestTrue(FString::Printf(
					TEXT("%s commits real WorldScape ocean LODs"), GalleryCase.DisplayName),
					Root->WorldScapeLodOcean.Num() > 0);
			}
			else
			{
				bValid &= Test->TestNull(FString::Printf(
					TEXT("%s has no stale WorldScape liquid material"), GalleryCase.DisplayName),
					Root->OceanMaterial.DefaultMaterial);
			}

			if (PreviousSurfaceSignature != 0)
			{
				bValid &= Test->TestTrue(FString::Printf(TEXT("%s changes the resolver signature"),
					GalleryCase.DisplayName),
					Surface->AppliedSurfaceProfileSignature != PreviousSurfaceSignature);
			}
			bValid &= Test->TestTrue(FString::Printf(TEXT("%s keeps the same selected planet actor"),
				GalleryCase.DisplayName), !InitialPlanet.IsValid() || Planet == InitialPlanet.Get());
			if (!InitialPlanet.IsValid())
			{
				InitialPlanet = Planet;
			}
			return bValid;
		}

		bool UpdateWarmSurface(UWorld* World,
			UWorldGenerationViewModel* ViewModel, double Now)
		{
			if (!World || !ViewModel || !PreviewGenerator.IsValid())
			{
				RecordCaseFailure(TEXT("preview disappeared during warm sample"));
				Step = EGalleryStep::CaptureSurface;
				StepStartSeconds = Now;
				return false;
			}
			TrackWarmFrame();
			if (Now - WarmStartSeconds < WarmSampleSeconds)
			{
				return false;
			}

			const double Elapsed = FMath::Max(Now - WarmStartSeconds, UE_DOUBLE_SMALL_NUMBER);
			const double AverageFps = WarmFrameCount / Elapsed;
			const double MaxFrameMs = WarmMaxFrameSeconds * 1000.0;
			const FGalleryCase& GalleryCase = GalleryCases[CurrentCaseIndex];
			const bool bFpsPass = Test->TestTrue(FString::Printf(
				TEXT("%s avoids the sustained 30-FPS regression"),
				GalleryCase.DisplayName), AverageFps >= MinimumWarmFps);
			const bool bFramePass = Test->TestTrue(FString::Printf(
				TEXT("%s has no quarter-second warm stall"),
				GalleryCase.DisplayName), MaxFrameMs < MaximumFrameMilliseconds);
			if (!bFpsPass || !bFramePass)
			{
				RecordCaseFailure(FString::Printf(
					TEXT("warm frame budget failed (avgFps=%.1f maxFrameMs=%.2f)"),
					AverageFps, MaxFrameMs));
			}
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Gallery.Perf] type=%s frames=%d seconds=%.2f avgFps=%.1f maxFrameMs=%.2f targetFps=120"),
				GalleryCase.DisplayName, WarmFrameCount, Elapsed, AverageFps, MaxFrameMs);

			Step = EGalleryStep::CaptureSurface;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateCaptureSurface(UWorld* World, AMainMenuController* Controller,
			UWorldGenerationViewModel* ViewModel, double Now)
		{
			if (!World || !Controller || !ViewModel)
			{
				return Fail(TEXT("Preview disappeared before gallery screenshot"));
			}
			const FGalleryCase& GalleryCase = GalleryCases[CurrentCaseIndex];
			APlanetarySurfaceGenerator* Surface = PreviewGenerator.IsValid()
				? FindPreviewSurfaceGenerator(World, PreviewGenerator.Get()) : nullptr;
			AWorldScapeRoot* Root = Surface ? Surface->WorldScapeRootInstance : nullptr;
			FString CaptureFailure;
			bool bScreenshotWritten = false;
			const bool bCapturePassed = CaptureAndValidatePlanetViewport(World, Controller,
				Root, GalleryCase, CaptureFailure, bScreenshotWritten);
			if (!bCapturePassed && !bScreenshotWritten && CaptureFailure.IsEmpty())
			{
				if (Now - StepStartSeconds < ScreenshotTimeoutSeconds)
				{
					return false;
				}
				CaptureFailure = TEXT("viewport pixels remained unavailable for ten seconds");
			}
			if (bScreenshotWritten)
			{
				++CapturedCaseCount;
			}
			if (!bCapturePassed)
			{
				RecordCaseFailure(CaptureFailure.IsEmpty()
					? TEXT("rendered frame did not satisfy the pixel contract")
					: CaptureFailure);
			}

			PreviousSurfaceSignature = Surface
				? Surface->AppliedSurfaceProfileSignature : 0;
			if (++CurrentCaseIndex >= UE_ARRAY_COUNT(GalleryCases))
			{
				Step = EGalleryStep::Cleanup;
				StepStartSeconds = Now;
				return false;
			}

			RevisionBeforeCase = ViewModel->PreviewRevision;
			CompilationContext.Reset();
			CompilationIdleSince = 0.0;
			BeginOperationTracking();
			ViewModel->SetEnumValue(StaticEnum<EPlanetType>(),
				static_cast<int32>(GalleryCases[CurrentCaseIndex].PlanetType));
			Step = EGalleryStep::WaitForSurface;
			StepStartSeconds = FPlatformTime::Seconds();
			return false;
		}

		bool CaptureAndValidatePlanetViewport(UWorld* World,
			AMainMenuController* Controller, AWorldScapeRoot* Root,
			const FGalleryCase& GalleryCase, FString& OutFailure,
			bool& bOutScreenshotWritten)
		{
			OutFailure.Reset();
			bOutScreenshotWritten = false;
			UGameViewportClient* GameViewportClient = AutomationCommon::GetAnyGameViewportClient();
			FViewport* Viewport = GameViewportClient ? GameViewportClient->Viewport : nullptr;
			if (!Viewport || !GameViewportClient || GameViewportClient->GetWorld() != World)
			{
				return false;
			}

			const FIntPoint ViewportSize = Viewport->GetSizeXY();
			TArray<FColor> Pixels;
			if (ViewportSize.X <= 0 || ViewportSize.Y <= 0 || !Viewport->ReadPixels(Pixels)
				|| Pixels.Num() != static_cast<int64>(ViewportSize.X) * ViewportSize.Y)
			{
				return false;
			}

			const FString ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
				FString::Printf(TEXT("Screenshots/Windows/APS_PlanetGallery_%s.png"),
					GalleryCase.FileToken));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
			TArray64<uint8> PngData;
			FImageUtils::PNGCompressImageArray(ViewportSize.X, ViewportSize.Y,
				TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PngData);
			if (PngData.IsEmpty() || !FFileHelper::SaveArrayToFile(PngData, *ScreenshotPath))
			{
				OutFailure = FString::Printf(TEXT("Could not encode/write %s"), *ScreenshotPath);
				return false;
			}
			bOutScreenshotWritten = true;
			if (!IsValid(Root) || Root->IsHidden())
			{
				OutFailure = FString::Printf(TEXT("%s frame has no committed WorldScape root"),
					GalleryCase.DisplayName);
				return false;
			}

			FVector2D PlanetScreenCenter;
			FVector2D PlanetScreenEdge;
			const FVector CameraRight = Controller->PlayerCameraManager
				? Controller->PlayerCameraManager->GetCameraRotation().RotateVector(FVector::RightVector)
				: FVector::RightVector;
			const FVector PlanetCenter = Root->GetActorLocation();
			const double PlanetRadius = FMath::Max(Root->PlanetScale, 1.0);
			if (!Controller->ProjectWorldLocationToScreen(PlanetCenter,
				PlanetScreenCenter, false)
				|| !Controller->ProjectWorldLocationToScreen(PlanetCenter
					+ CameraRight * PlanetRadius, PlanetScreenEdge, false))
			{
				OutFailure = FString::Printf(TEXT("%s planet globe could not be projected into the viewport"),
					GalleryCase.DisplayName);
				return false;
			}

			const double ProjectedRadius = FVector2D::Distance(PlanetScreenCenter, PlanetScreenEdge);
			const double SampleRadius = ProjectedRadius * 0.72;
			if (!FMath::IsFinite(SampleRadius) || SampleRadius < 24.0)
			{
				OutFailure = FString::Printf(TEXT("%s projected globe is too small (%.1f px)"),
					GalleryCase.DisplayName, ProjectedRadius);
				return false;
			}

			double BrightnessSum = 0.0;
			double BrightnessSquaredSum = 0.0;
			int64 SamplePixelCount = 0;
			int64 NonBlackPixelCount = 0;
			int64 ClippedPlanetPixelCount = 0;
			const int32 MinX = FMath::Max(0,
				FMath::FloorToInt(PlanetScreenCenter.X - SampleRadius));
			const int32 MaxX = FMath::Min(ViewportSize.X - 1,
				FMath::CeilToInt(PlanetScreenCenter.X + SampleRadius));
			const int32 MinY = FMath::Max(0,
				FMath::FloorToInt(PlanetScreenCenter.Y - SampleRadius));
			const int32 MaxY = FMath::Min(ViewportSize.Y - 1,
				FMath::CeilToInt(PlanetScreenCenter.Y + SampleRadius));
			const double SampleRadiusSquared = FMath::Square(SampleRadius);
			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				for (int32 X = MinX; X <= MaxX; ++X)
				{
					const double DeltaX = X - PlanetScreenCenter.X;
					const double DeltaY = Y - PlanetScreenCenter.Y;
					if (DeltaX * DeltaX + DeltaY * DeltaY > SampleRadiusSquared)
					{
						continue;
					}
					// Exclude the compact body label/crosshair so UI glyphs cannot make an
					// otherwise black sphere pass the rendered-surface contract.
					if (FMath::Abs(DeltaX) < SampleRadius * 0.38
						&& FMath::Abs(DeltaY) < SampleRadius * 0.12)
					{
						continue;
					}
					const FColor& Pixel = Pixels[Y * ViewportSize.X + X];
					const double Brightness = static_cast<double>(
						FMath::Max3(Pixel.R, Pixel.G, Pixel.B));
					BrightnessSum += Brightness;
					BrightnessSquaredSum += Brightness * Brightness;
					NonBlackPixelCount += Brightness > 12.0 ? 1 : 0;
					ClippedPlanetPixelCount += Brightness >= 250.0 ? 1 : 0;
					++SamplePixelCount;
				}
			}

			if (SamplePixelCount <= 0)
			{
				OutFailure = FString::Printf(TEXT("%s planet disk contained no readable pixels"),
					GalleryCase.DisplayName);
				return false;
			}
			const double PixelCount = static_cast<double>(SamplePixelCount);
			const double MeanBrightness = BrightnessSum / PixelCount;
			const double BrightnessVariance = FMath::Max(0.0,
				BrightnessSquaredSum / PixelCount - MeanBrightness * MeanBrightness);
			const double NonBlackRatio = static_cast<double>(NonBlackPixelCount) / PixelCount;
			const double PlanetClippedRatio =
				static_cast<double>(ClippedPlanetPixelCount) / PixelCount;

			// A pathological emissive material can make the planet disk non-black while
			// blooming over the entire camera (including labels and the starfield). Sample
			// outside a generous planet envelope so that local lava highlights remain
			// valid but global auto-exposure/bloom clipping becomes a hard regression.
			double BackgroundBrightnessSum = 0.0;
			double BackgroundBrightnessSquaredSum = 0.0;
			int64 BackgroundPixelCount = 0;
			int64 ClippedBackgroundPixelCount = 0;
			const double BackgroundExclusionRadius = ProjectedRadius * 1.25;
			const double BackgroundExclusionRadiusSquared =
				FMath::Square(BackgroundExclusionRadius);
			for (int32 Y = 0; Y < ViewportSize.Y; ++Y)
			{
				for (int32 X = 0; X < ViewportSize.X; ++X)
				{
					const double DeltaX = X - PlanetScreenCenter.X;
					const double DeltaY = Y - PlanetScreenCenter.Y;
					if (DeltaX * DeltaX + DeltaY * DeltaY
						<= BackgroundExclusionRadiusSquared)
					{
						continue;
					}
					const FColor& Pixel = Pixels[Y * ViewportSize.X + X];
					const double Brightness = static_cast<double>(
						FMath::Max3(Pixel.R, Pixel.G, Pixel.B));
					BackgroundBrightnessSum += Brightness;
					BackgroundBrightnessSquaredSum += Brightness * Brightness;
					ClippedBackgroundPixelCount += Brightness >= 250.0 ? 1 : 0;
					++BackgroundPixelCount;
				}
			}
			if (BackgroundPixelCount <= 0)
			{
				OutFailure = FString::Printf(
					TEXT("%s frame has no pixels outside the projected planet envelope"),
					GalleryCase.DisplayName);
				return false;
			}
			const double BackgroundCount = static_cast<double>(BackgroundPixelCount);
			const double BackgroundMeanBrightness = BackgroundBrightnessSum / BackgroundCount;
			const double BackgroundBrightnessVariance = FMath::Max(0.0,
				BackgroundBrightnessSquaredSum / BackgroundCount
					- FMath::Square(BackgroundMeanBrightness));
			const double BackgroundClippedRatio =
				static_cast<double>(ClippedBackgroundPixelCount) / BackgroundCount;
			const double BackgroundToPlanetExposureRatio = BackgroundMeanBrightness
				/ FMath::Max(MeanBrightness, 1.0);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Gallery.Pixel] type=%s viewport=%dx%d center=(%.1f,%.1f) radius=%.1f samples=%lld mean=%.3f variance=%.3f nonBlackRatio=%.5f clippedRatio=%.5f backgroundSamples=%lld backgroundMean=%.3f backgroundVariance=%.3f backgroundClippedRatio=%.5f backgroundToPlanet=%.5f screenshot=%s"),
				GalleryCase.DisplayName, ViewportSize.X, ViewportSize.Y,
				PlanetScreenCenter.X, PlanetScreenCenter.Y, ProjectedRadius,
				SamplePixelCount, MeanBrightness, BrightnessVariance, NonBlackRatio,
				PlanetClippedRatio, BackgroundPixelCount, BackgroundMeanBrightness,
				BackgroundBrightnessVariance, BackgroundClippedRatio,
				BackgroundToPlanetExposureRatio,
				*ScreenshotPath);

			TArray<FString> PixelFailures;
			if (MeanBrightness < MinimumPlanetMeanBrightness
				|| BrightnessVariance < MinimumPlanetBrightnessVariance
				|| NonBlackRatio < MinimumPlanetNonBlackRatio
				|| PlanetClippedRatio > MaximumPlanetClippedRatio)
			{
				PixelFailures.Add(FString::Printf(
					TEXT("planet disk is black/flat/clipped (mean=%.3f variance=%.3f nonBlack=%.5f clipped=%.5f; contracts mean>=%.3f variance>=%.3f nonBlack>=%.3f clipped<=%.3f)"),
					MeanBrightness, BrightnessVariance, NonBlackRatio,
					PlanetClippedRatio, MinimumPlanetMeanBrightness,
					MinimumPlanetBrightnessVariance, MinimumPlanetNonBlackRatio,
					MaximumPlanetClippedRatio));
			}
			if (BackgroundMeanBrightness > MaximumBackgroundMeanBrightness
				|| BackgroundClippedRatio > MaximumBackgroundClippedRatio)
			{
				PixelFailures.Add(FString::Printf(
					TEXT("global bloom/overexposure escaped the planet (backgroundMean=%.3f backgroundClipped=%.5f backgroundToPlanet=%.5f; maximum %.3f/%.3f)"),
					BackgroundMeanBrightness, BackgroundClippedRatio,
					BackgroundToPlanetExposureRatio, MaximumBackgroundMeanBrightness,
					MaximumBackgroundClippedRatio));
			}
			if (!PixelFailures.IsEmpty())
			{
				OutFailure = FString::Printf(TEXT("%s: %s"), GalleryCase.DisplayName,
					*FString::Join(PixelFailures, TEXT("; ")));
				return false;
			}
			return true;
		}

		void BeginOperationTracking()
		{
			OperationLastFrameSeconds = FPlatformTime::Seconds();
			OperationMaxFrameSeconds = 0.0;
		}

		void TrackOperationFrame()
		{
			const double Now = FPlatformTime::Seconds();
			const double FrameSeconds = Now - OperationLastFrameSeconds;
			OperationLastFrameSeconds = Now;
			if (FrameSeconds > 0.0 && FMath::IsFinite(FrameSeconds))
			{
				OperationMaxFrameSeconds = FMath::Max(OperationMaxFrameSeconds, FrameSeconds);
			}
		}

		void AssertOperationFrameBudget(const FGalleryCase& GalleryCase)
		{
			const double MaxFrameMs = OperationMaxFrameSeconds * 1000.0;
			const bool bPass = Test->TestTrue(FString::Printf(
				TEXT("%s switch has no quarter-second stall"),
				GalleryCase.DisplayName), MaxFrameMs < MaximumFrameMilliseconds);
			if (!bPass)
			{
				RecordCaseFailure(FString::Printf(TEXT("switch frame budget failed (%.2f ms)"),
					MaxFrameMs));
			}
			UE_LOG(LogTemp, Display, TEXT("[APS.Gallery.Perf] type=%s switchMaxFrameMs=%.2f"),
				GalleryCase.DisplayName, MaxFrameMs);
		}

		void BeginWarmSample()
		{
			WarmStartSeconds = FPlatformTime::Seconds();
			WarmLastFrameSeconds = WarmStartSeconds;
			WarmMaxFrameSeconds = 0.0;
			WarmFrameCount = 0;
		}

		void TrackWarmFrame()
		{
			const double Now = FPlatformTime::Seconds();
			const double FrameSeconds = Now - WarmLastFrameSeconds;
			WarmLastFrameSeconds = Now;
			if (FrameSeconds > 0.0 && FMath::IsFinite(FrameSeconds))
			{
				WarmMaxFrameSeconds = FMath::Max(WarmMaxFrameSeconds, FrameSeconds);
				++WarmFrameCount;
			}
		}

		void RecordCaseFailure(const FString& Failure)
		{
			const FString TaggedFailure = FString::Printf(TEXT("[%s] %s"),
				GalleryCases[CurrentCaseIndex].DisplayName, *Failure);
			CaseFailures.AddUnique(TaggedFailure);
			UE_LOG(LogTemp, Warning, TEXT("[APS.Gallery.Aggregate] %s"), *TaggedFailure);
		}

		bool Fail(const FString& Message)
		{
			if (PendingFailure.IsEmpty())
			{
				PendingFailure = Message;
				UE_LOG(LogTemp, Error, TEXT("[APS.Gallery] %s; draining preview before exit"),
					*Message);
			}
			Step = EGalleryStep::Cleanup;
			StepStartSeconds = FPlatformTime::Seconds();
			return false;
		}

		bool UpdateCleanup(double Now)
		{
			if (PreviewGenerator.IsValid() && !PreviewGenerator->PreparePreviewForTravel())
			{
				if (Now - StepStartSeconds <= CleanupTimeoutSeconds)
				{
					return false;
				}
				if (PendingFailure.IsEmpty())
				{
					PendingFailure = TEXT("Gallery preview did not drain during cleanup");
				}
			}

			for (const TWeakObjectPtr<UObject>& WarmedAsset : WarmedAssets)
			{
				if (UObject* Asset = WarmedAsset.Get(); IsValid(Asset) && Asset->IsRooted())
				{
					Asset->RemoveFromRoot();
				}
			}
			WarmedAssets.Reset();
			for (const FSavedConsoleVariableState& Saved : SavedConsoleVariables)
			{
				if (!Saved.Variable) continue;
				// Restore through the same high-priority channel used by the test, then
				// restore the exact previous flags so later tests retain their original
				// scalability/project/command-line precedence.
				Saved.Variable->Set(*Saved.Value, ECVF_SetByCode);
				Saved.Variable->SetFlags(Saved.Flags);
			}
			SavedConsoleVariables.Reset();
			if (CapturedCaseCount != UE_ARRAY_COUNT(GalleryCases))
			{
				CaseFailures.AddUnique(FString::Printf(
					TEXT("[GALLERY] wrote %d/%d required screenshots"),
					CapturedCaseCount, UE_ARRAY_COUNT(GalleryCases)));
			}
			if (!PendingFailure.IsEmpty())
			{
				Test->AddError(PendingFailure);
			}
			if (!CaseFailures.IsEmpty())
			{
				Test->AddError(FString::Printf(
					TEXT("Planet surface gallery completed all cases with %d aggregated failure(s):\n - %s"),
					CaseFailures.Num(), *FString::Join(CaseFailures, TEXT("\n - "))));
			}
			if (PendingFailure.IsEmpty() && CaseFailures.IsEmpty())
			{
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Gallery] PASS wrote %d resolved WorldScape screenshots"),
					UE_ARRAY_COUNT(GalleryCases));
			}
			return true;
		}

		FAutomationTestBase* Test{nullptr};
		EGalleryStep Step{EGalleryStep::Startup};
		int32 CurrentCaseIndex{0};
		int32 RevisionBeforeCase{0};
		double TestStartSeconds{0.0};
		double StepStartSeconds{0.0};
		double CompilationIdleSince{0.0};
		double OperationLastFrameSeconds{0.0};
		double OperationMaxFrameSeconds{0.0};
		double WarmStartSeconds{0.0};
		double WarmLastFrameSeconds{0.0};
		double WarmMaxFrameSeconds{0.0};
		int32 WarmFrameCount{0};
		int32 CapturedCaseCount{0};
		uint32 PreviousSurfaceSignature{0};
		bool bWarmupPrepared{false};
		FString CompilationContext;
		FString PendingFailure;
		TArray<FString> CaseFailures;
		TWeakObjectPtr<AAstroGenerator> PreviewGenerator;
		TWeakObjectPtr<APlanet> InitialPlanet;
		TArray<TWeakObjectPtr<UObject>> WarmedAssets;
		TArray<FSavedConsoleVariableState> SavedConsoleVariables;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSRenderedPlanetSurfaceGallerySmokeTest,
	"APS.Rendered.MainMenu.PlanetSurfaceGallery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSRenderedPlanetSurfaceGallerySmokeTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("Could not open the MainMenu map for rendered planet surface gallery"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSPlanetSurfaceGallerySmokeTests::FRenderedPlanetSurfaceGalleryCommand(this));
	return true;
}

#endif
