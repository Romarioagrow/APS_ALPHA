// Rio 03.10 (galaxy phase 3): registry, render-thread hand-off and the public API.
#include "APSStarRegistry.h"

#include "APSStarRendererPrivate.h"
#include "Async/Async.h"
#include "Engine/World.h"
#include "Misc/ScopeLock.h"
#include "RHIGPUReadback.h"
#include "RenderingThread.h"
#include "SceneInterface.h"

namespace APSStarRenderer::Private
{
	namespace
	{
		constexpr double PaletteMinKelvin = 1500.0;
		constexpr double PaletteMaxKelvin = 40000.0;

		double PiecewiseGaussian(const double Lambda, const double Mu, const double SigmaLow, const double SigmaHigh)
		{
			const double Sigma = Lambda < Mu ? SigmaLow : SigmaHigh;
			const double T = (Lambda - Mu) / Sigma;
			return FMath::Exp(-0.5 * T * T);
		}

		/** Black body colour: Planck spectrum, CIE 1931 fit (Wyman et al. 2013), linear sRGB at unit luminance. */
		FLinearColor BlackBodyColor(const double Kelvin)
		{
			double X = 0.0;
			double Y = 0.0;
			double Z = 0.0;
			for (double Lambda = 380.0; Lambda <= 780.0; Lambda += 5.0)
			{
				const double LambdaM = Lambda * 1.0e-9;
				const double Planck = 1.0 / (FMath::Pow(LambdaM, 5.0) * (FMath::Exp(1.4387769e-2 / (LambdaM * Kelvin)) - 1.0));
				const double XBar = 1.056 * PiecewiseGaussian(Lambda, 599.8, 37.9, 31.0)
					+ 0.362 * PiecewiseGaussian(Lambda, 442.0, 16.0, 26.7)
					- 0.065 * PiecewiseGaussian(Lambda, 501.1, 20.4, 26.2);
				const double YBar = 0.821 * PiecewiseGaussian(Lambda, 568.8, 46.9, 40.5)
					+ 0.286 * PiecewiseGaussian(Lambda, 530.9, 16.3, 31.1);
				const double ZBar = 1.217 * PiecewiseGaussian(Lambda, 437.0, 11.8, 36.0)
					+ 0.681 * PiecewiseGaussian(Lambda, 459.0, 26.0, 13.8);
				X += Planck * XBar;
				Y += Planck * YBar;
				Z += Planck * ZBar;
			}
			if (!(Y > 0.0))
			{
				return FLinearColor::White;
			}
			X /= Y;
			Z /= Y;
			Y = 1.0;
			const double R = FMath::Max(3.2406 * X - 1.5372 * Y - 0.4986 * Z, 0.0);
			const double G = FMath::Max(-0.9689 * X + 1.8758 * Y + 0.0415 * Z, 0.0);
			const double B = FMath::Max(0.0557 * X - 0.2040 * Y + 1.0570 * Z, 0.0);
			const double Luminance = FMath::Max(0.2126 * R + 0.7152 * G + 0.0722 * B, 1.0e-6);
			return FLinearColor(static_cast<float>(R / Luminance), static_cast<float>(G / Luminance),
				static_cast<float>(B / Luminance), 1.0f);
		}

		double PaletteKelvin(const int32 Index)
		{
			return PaletteMinKelvin * FMath::Pow(PaletteMaxKelvin / PaletteMinKelvin, Index / 255.0);
		}

		FVector4f NormalizePaletteColor(const FLinearColor& Color)
		{
			const float R = FMath::Max(Color.R, 0.0f);
			const float G = FMath::Max(Color.G, 0.0f);
			const float B = FMath::Max(Color.B, 0.0f);
			const float Luminance = 0.2126f * R + 0.7152f * G + 0.0722f * B;
			if (!(Luminance > 1.0e-6f) || !FMath::IsFinite(Luminance))
			{
				return FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
			}
			return FVector4f(R / Luminance, G / Luminance, B / Luminance, 1.0f);
		}
	}

	FRenderState::FRenderState()
	{
		for (int32 Index = 0; Index < 256; ++Index)
		{
			Palette[Index] = NormalizePaletteColor(BlackBodyColor(PaletteKelvin(Index)));
		}
	}

	FRenderState::~FRenderState() = default;

	FPointSetRT* FRenderState::FindPointSet(const FHandle Handle)
	{
		return PointSets.FindByPredicate([Handle](const FPointSetRT& Set) { return Set.Handle == Handle; });
	}

	FGlowVolumeRT* FRenderState::FindGlowVolume(const FHandle Handle)
	{
		return GlowVolumes.FindByPredicate([Handle](const FGlowVolumeRT& Volume) { return Volume.Handle == Handle; });
	}

	float FRenderState::GetSceneVisibility(const FSceneInterface* Scene) const
	{
		const float* Found = SceneVisibility.Find(Scene);
		return Found ? *Found : 1.0f;
	}

	void FRenderState::Reset()
	{
		PointSets.Empty();
		GlowVolumes.Empty();
		SceneVisibility.Empty();
		Readbacks.Empty();
	}

	FRegistry::FRegistry()
		: RenderState(MakeShared<FRenderState, ESPMode::ThreadSafe>())
	{
	}

	FRegistry& FRegistry::Get()
	{
		static FRegistry Instance;
		return Instance;
	}

	FHandle FRegistry::AllocateHandle()
	{
		FHandle Handle = NextHandle.fetch_add(1);
		if (Handle == 0)
		{
			Handle = NextHandle.fetch_add(1);
		}
		return Handle;
	}

	void FRegistry::AddEntry(const FHandle Handle, const bool bGlow, const UWorld* World, const FSceneInterface* Scene,
		const bool bEnabled)
	{
		FScopeLock Lock(&EntryMutex);
		FEntry& Entry = Entries.Add(Handle);
		Entry.bGlow = bGlow;
		Entry.bEnabled = bEnabled;
		Entry.World = World;
		Entry.Scene = Scene;
	}

	void FRegistry::SetEntryEnabled(const FHandle Handle, const bool bEnabled)
	{
		FScopeLock Lock(&EntryMutex);
		if (FEntry* Entry = Entries.Find(Handle))
		{
			Entry->bEnabled = bEnabled;
		}
	}

	void FRegistry::RemoveEntry(const FHandle Handle)
	{
		FScopeLock Lock(&EntryMutex);
		Entries.Remove(Handle);
	}

	void FRegistry::CollectWorldEntries(const UWorld* World, TArray<FHandle>& OutHandles) const
	{
		FScopeLock Lock(&EntryMutex);
		for (const TPair<FHandle, FEntry>& Pair : Entries)
		{
			if (Pair.Value.World == World)
			{
				OutHandles.Add(Pair.Key);
			}
		}
	}

	bool FRegistry::HasEnabledEntries(const FSceneInterface* Scene, const bool bPoints, const bool bGlow) const
	{
		FScopeLock Lock(&EntryMutex);
		for (const TPair<FHandle, FEntry>& Pair : Entries)
		{
			const FEntry& Entry = Pair.Value;
			if (!Entry.bEnabled || (Entry.bGlow ? !bGlow : !bPoints))
			{
				continue;
			}
			if (Scene == nullptr || Entry.Scene == Scene)
			{
				return true;
			}
		}
		return false;
	}

	int32 FRegistry::CountEntries(const bool bGlow) const
	{
		FScopeLock Lock(&EntryMutex);
		int32 Count = 0;
		for (const TPair<FHandle, FEntry>& Pair : Entries)
		{
			Count += Pair.Value.bGlow == bGlow ? 1 : 0;
		}
		return Count;
	}

	void FRegistry::EnqueueRenderUpdate(TUniqueFunction<void(FRenderState&)>&& Update)
	{
		if (bShutdown.load())
		{
			return;
		}
		TSharedPtr<FRenderState, ESPMode::ThreadSafe> State = RenderState;
		ENQUEUE_RENDER_COMMAND(APSStarRendererUpdate)(
			[State, Update = MoveTemp(Update)](FRHICommandListImmediate&) mutable
			{
				if (State.IsValid())
				{
					Update(*State);
				}
			});
	}

	void FRegistry::PublishStats(const FStats& InStats)
	{
		FScopeLock Lock(&StatsMutex);
		const int64 Tested = Stats.GpuTested;
		const int64 InFrustum = Stats.GpuInFrustum;
		const int64 Visible = Stats.GpuVisible;
		const int64 Written = Stats.GpuWritten;
		Stats = InStats;
		Stats.GpuTested = Tested;
		Stats.GpuInFrustum = InFrustum;
		Stats.GpuVisible = Visible;
		Stats.GpuWritten = Written;
	}

	void FRegistry::PublishGpuCounters(const uint32 FrameNumber, const uint32 Counters[4])
	{
		FScopeLock Lock(&StatsMutex);
		Stats.GpuTested = Counters[0];
		Stats.GpuInFrustum = Counters[1];
		Stats.GpuVisible = Counters[2];
		Stats.GpuWritten = Counters[3];
		Stats.FrameNumber = FrameNumber;
	}

	FStats FRegistry::GetStats() const
	{
		FScopeLock Lock(&StatsMutex);
		return Stats;
	}

	void FRegistry::Shutdown()
	{
		if (bShutdown.exchange(true))
		{
			return;
		}
		{
			FScopeLock Lock(&EntryMutex);
			Entries.Empty();
		}
		TSharedPtr<FRenderState, ESPMode::ThreadSafe> State = RenderState;
		ENQUEUE_RENDER_COMMAND(APSStarRendererShutdown)(
			[State](FRHICommandListImmediate&)
			{
				if (State.IsValid())
				{
					State->Reset();
				}
			});
		FlushRenderingCommands();
	}
}

namespace APSStarRenderer
{
	using namespace APSStarRenderer::Private;

	namespace
	{
		/** API calls arrive from any thread; the registry and the render queue are fed in game-thread order. */
		void RunOnGameThread(TUniqueFunction<void()>&& Function)
		{
			if (IsInGameThread())
			{
				Function();
			}
			else
			{
				AsyncTask(ENamedThreads::GameThread, MoveTemp(Function));
			}
		}

		TWeakObjectPtr<UWorld> MakeWeakWorld(const UWorld* World)
		{
			return TWeakObjectPtr<UWorld>(const_cast<UWorld*>(World));
		}

		uint32 QuantizeAxis(const float Value, const float Min, const float Size)
		{
			if (!(Size > 0.0f) || !FMath::IsFinite(Value))
			{
				return 0u;
			}
			const float T = FMath::Clamp((Value - Min) / Size, 0.0f, 1.0f);
			return static_cast<uint32>(FMath::RoundToInt(T * 65535.0f)) & 0xFFFFu;
		}
	}

	uint8 EncodeIntensity(const float Intensity)
	{
		if (!(Intensity > 0.0f) || !FMath::IsFinite(Intensity))
		{
			return 0;
		}
		const float Code = FMath::RoundToFloat(FMath::Log2(Intensity) * 8.0f) + 128.0f;
		return static_cast<uint8>(FMath::Clamp(Code, 1.0f, 255.0f));
	}

	float DecodeIntensity(const uint8 Code)
	{
		return Code == 0 ? 0.0f : FMath::Exp2((static_cast<float>(Code) - 128.0f) * 0.125f);
	}

	FPackedStar PackStar(const FVector3f& LocalPosition, const FBox3f& Bounds, const uint8 ColorIndex, const float Intensity)
	{
		const FVector3f Size = Bounds.Max - Bounds.Min;
		FPackedStar Star;
		Star.XY = QuantizeAxis(LocalPosition.X, Bounds.Min.X, Size.X)
			| (QuantizeAxis(LocalPosition.Y, Bounds.Min.Y, Size.Y) << 16);
		Star.ZColorIntensity = QuantizeAxis(LocalPosition.Z, Bounds.Min.Z, Size.Z)
			| (static_cast<uint32>(ColorIndex) << 16)
			| (static_cast<uint32>(EncodeIntensity(Intensity)) << 24);
		return Star;
	}

	FVector3f UnpackStarPosition(const FPackedStar& Star, const FBox3f& Bounds)
	{
		const FVector3f Step = (Bounds.Max - Bounds.Min) / 65535.0f;
		return Bounds.Min + FVector3f(
			static_cast<float>(Star.XY & 0xFFFFu) * Step.X,
			static_cast<float>(Star.XY >> 16) * Step.Y,
			static_cast<float>(Star.ZColorIntensity & 0xFFFFu) * Step.Z);
	}

	FHandle RegisterPointSet(const UWorld* World, const FPointSetDesc& Desc, TArray<FPackedStar>&& Points)
	{
		FRegistry& Registry = FRegistry::Get();
		if (World == nullptr || Points.Num() == 0 || Registry.IsShutdown())
		{
			return 0;
		}
		if (Points.Num() > MaxPointsPerSet)
		{
			UE_LOG(LogAPSStarRenderer, Warning, TEXT("[APS.GpuStars] point set '%s': %d points, kept the first %d (split it into sets)"),
				*Desc.DebugName, Points.Num(), MaxPointsPerSet);
			Points.SetNum(MaxPointsPerSet);
		}
		const FHandle Handle = Registry.AllocateHandle();
		RunOnGameThread([Handle, WeakWorld = MakeWeakWorld(World), Desc, Points = MoveTemp(Points)]() mutable
		{
			const UWorld* GameWorld = WeakWorld.Get();
			const FSceneInterface* Scene = GameWorld ? GameWorld->Scene : nullptr;
			if (Scene == nullptr)
			{
				UE_LOG(LogAPSStarRenderer, Warning, TEXT("[APS.GpuStars] point set '%s' dropped: the world has no scene"), *Desc.DebugName);
				return;
			}
			FRegistry& GameRegistry = FRegistry::Get();
			GameRegistry.AddEntry(Handle, false, GameWorld, Scene, Desc.bEnabled);
			UE_LOG(LogAPSStarRenderer, Log, TEXT("[APS.GpuStars] point set %u '%s': %d points (population %lld)"),
				Handle, *Desc.DebugName, Points.Num(), Desc.Population);
			GameRegistry.EnqueueRenderUpdate([Handle, Scene, Desc, Points = MoveTemp(Points)](FRenderState& State) mutable
			{
				FPointSetRT& Set = State.PointSets.AddDefaulted_GetRef();
				Set.Handle = Handle;
				Set.Scene = Scene;
				Set.Desc = Desc;
				Set.NumPoints = Points.Num();
				Set.PendingPoints = MoveTemp(Points);
			});
		});
		return Handle;
	}

	void UpdatePointSet(const FHandle Handle, const FPointSetDesc& Desc)
	{
		if (Handle == 0)
		{
			return;
		}
		RunOnGameThread([Handle, Desc]()
		{
			FRegistry& Registry = FRegistry::Get();
			Registry.SetEntryEnabled(Handle, Desc.bEnabled);
			Registry.EnqueueRenderUpdate([Handle, Desc](FRenderState& State)
			{
				if (FPointSetRT* Set = State.FindPointSet(Handle))
				{
					const FBox3f Bounds = Set->Desc.LocalBounds;
					Set->Desc = Desc;
					Set->Desc.LocalBounds = Bounds;
				}
			});
		});
	}

	FHandle RegisterGlowVolume(const UWorld* World, const FGlowVolumeDesc& Desc, FGlowMap&& Map)
	{
		FRegistry& Registry = FRegistry::Get();
		if (World == nullptr || !Map.IsValid() || Registry.IsShutdown())
		{
			UE_LOG(LogAPSStarRenderer, Warning, TEXT("[APS.GpuStars] glow volume '%s' rejected (no world or invalid map)"), *Desc.DebugName);
			return 0;
		}
		const FHandle Handle = Registry.AllocateHandle();
		RunOnGameThread([Handle, WeakWorld = MakeWeakWorld(World), Desc, Map = MoveTemp(Map)]() mutable
		{
			const UWorld* GameWorld = WeakWorld.Get();
			const FSceneInterface* Scene = GameWorld ? GameWorld->Scene : nullptr;
			if (Scene == nullptr)
			{
				return;
			}
			FRegistry& GameRegistry = FRegistry::Get();
			GameRegistry.AddEntry(Handle, true, GameWorld, Scene, Desc.bEnabled);
			UE_LOG(LogAPSStarRenderer, Log, TEXT("[APS.GpuStars] glow volume %u '%s': %dx%d map, %lld samples, extent %.4g x %.4g"),
				Handle, *Desc.DebugName, Map.Resolution, Map.Resolution, Map.SampleCount, Map.ExtentXY, Map.ExtentZ);
			GameRegistry.EnqueueRenderUpdate([Handle, Scene, Desc, Map = MoveTemp(Map)](FRenderState& State) mutable
			{
				FGlowVolumeRT& Volume = State.GlowVolumes.AddDefaulted_GetRef();
				Volume.Handle = Handle;
				Volume.Scene = Scene;
				Volume.Desc = Desc;
				Volume.Map = MoveTemp(Map);
			});
		});
		return Handle;
	}

	void UpdateGlowVolume(const FHandle Handle, const FGlowVolumeDesc& Desc)
	{
		if (Handle == 0)
		{
			return;
		}
		RunOnGameThread([Handle, Desc]()
		{
			FRegistry& Registry = FRegistry::Get();
			Registry.SetEntryEnabled(Handle, Desc.bEnabled);
			Registry.EnqueueRenderUpdate([Handle, Desc](FRenderState& State)
			{
				if (FGlowVolumeRT* Volume = State.FindGlowVolume(Handle))
				{
					Volume->Desc = Desc;
				}
			});
		});
	}

	void SetTransform(const FHandle Handle, const FTransform& LocalToWorld)
	{
		if (Handle == 0)
		{
			return;
		}
		RunOnGameThread([Handle, LocalToWorld]()
		{
			FRegistry::Get().EnqueueRenderUpdate([Handle, LocalToWorld](FRenderState& State)
			{
				if (FPointSetRT* Set = State.FindPointSet(Handle))
				{
					Set->Desc.LocalToWorld = LocalToWorld;
				}
				if (FGlowVolumeRT* Volume = State.FindGlowVolume(Handle))
				{
					Volume->Desc.LocalToWorld = LocalToWorld;
				}
			});
		});
	}

	void SetFarEnvelope(const FHandle Handle, const FFarEnvelope& Envelope)
	{
		if (Handle == 0)
		{
			return;
		}
		RunOnGameThread([Handle, Envelope]()
		{
			FRegistry::Get().EnqueueRenderUpdate([Handle, Envelope](FRenderState& State)
			{
				if (FPointSetRT* Set = State.FindPointSet(Handle))
				{
					Set->Desc.FarEnvelope = Envelope;
				}
				if (FGlowVolumeRT* Volume = State.FindGlowVolume(Handle))
				{
					Volume->Desc.FarEnvelope = Envelope;
				}
			});
		});
	}

	void SetVisibility(const FHandle Handle, const float Visibility)
	{
		if (Handle == 0)
		{
			return;
		}
		const float Clamped = FMath::Clamp(Visibility, 0.0f, 1.0f);
		RunOnGameThread([Handle, Clamped]()
		{
			FRegistry::Get().EnqueueRenderUpdate([Handle, Clamped](FRenderState& State)
			{
				if (FPointSetRT* Set = State.FindPointSet(Handle))
				{
					Set->Desc.Visibility = Clamped;
				}
				if (FGlowVolumeRT* Volume = State.FindGlowVolume(Handle))
				{
					Volume->Desc.Visibility = Clamped;
				}
			});
		});
	}

	void SetEnabled(const FHandle Handle, const bool bEnabled)
	{
		if (Handle == 0)
		{
			return;
		}
		RunOnGameThread([Handle, bEnabled]()
		{
			FRegistry& Registry = FRegistry::Get();
			Registry.SetEntryEnabled(Handle, bEnabled);
			Registry.EnqueueRenderUpdate([Handle, bEnabled](FRenderState& State)
			{
				if (FPointSetRT* Set = State.FindPointSet(Handle))
				{
					Set->Desc.bEnabled = bEnabled;
				}
				if (FGlowVolumeRT* Volume = State.FindGlowVolume(Handle))
				{
					Volume->Desc.bEnabled = bEnabled;
				}
			});
		});
	}

	void Remove(const FHandle Handle)
	{
		if (Handle == 0)
		{
			return;
		}
		RunOnGameThread([Handle]()
		{
			FRegistry& Registry = FRegistry::Get();
			Registry.RemoveEntry(Handle);
			Registry.EnqueueRenderUpdate([Handle](FRenderState& State)
			{
				State.PointSets.RemoveAll([Handle](const FPointSetRT& Set) { return Set.Handle == Handle; });
				State.GlowVolumes.RemoveAll([Handle](const FGlowVolumeRT& Volume) { return Volume.Handle == Handle; });
			});
		});
	}

	void RemoveAll(const UWorld* World)
	{
		if (World == nullptr)
		{
			return;
		}
		RunOnGameThread([WeakWorld = MakeWeakWorld(World), WorldKey = World]()
		{
			FRegistry& Registry = FRegistry::Get();
			TArray<FHandle> Handles;
			Registry.CollectWorldEntries(WorldKey, Handles);
			for (const FHandle Handle : Handles)
			{
				Registry.RemoveEntry(Handle);
			}
			const UWorld* GameWorld = WeakWorld.Get();
			const FSceneInterface* Scene = GameWorld ? GameWorld->Scene : nullptr;
			if (Handles.Num() > 0)
			{
				UE_LOG(LogAPSStarRenderer, Log, TEXT("[APS.GpuStars] removed %d sets/volumes of world %s"), Handles.Num(),
					GameWorld ? *GameWorld->GetName() : TEXT("<gone>"));
			}
			Registry.EnqueueRenderUpdate([Handles = MoveTemp(Handles), Scene](FRenderState& State)
			{
				State.PointSets.RemoveAll([&Handles](const FPointSetRT& Set) { return Handles.Contains(Set.Handle); });
				State.GlowVolumes.RemoveAll([&Handles](const FGlowVolumeRT& Volume) { return Handles.Contains(Volume.Handle); });
				if (Scene != nullptr)
				{
					State.SceneVisibility.Remove(Scene);
				}
			});
		});
	}

	void SetWorldFarEnvelope(const UWorld* World, const FFarEnvelope& Envelope)
	{
		if (World == nullptr)
		{
			return;
		}
		RunOnGameThread([WeakWorld = MakeWeakWorld(World), Envelope]()
		{
			const UWorld* GameWorld = WeakWorld.Get();
			const FSceneInterface* Scene = GameWorld ? GameWorld->Scene : nullptr;
			if (Scene == nullptr)
			{
				return;
			}
			FRegistry::Get().EnqueueRenderUpdate([Scene, Envelope](FRenderState& State)
			{
				for (FPointSetRT& Set : State.PointSets)
				{
					if (Set.Scene == Scene)
					{
						Set.Desc.FarEnvelope = Envelope;
					}
				}
				for (FGlowVolumeRT& Volume : State.GlowVolumes)
				{
					if (Volume.Scene == Scene)
					{
						Volume.Desc.FarEnvelope = Envelope;
					}
				}
			});
		});
	}

	void SetWorldVisibility(const UWorld* World, const float Visibility)
	{
		if (World == nullptr)
		{
			return;
		}
		const float Clamped = FMath::Clamp(Visibility, 0.0f, 1.0f);
		RunOnGameThread([WeakWorld = MakeWeakWorld(World), Clamped]()
		{
			const UWorld* GameWorld = WeakWorld.Get();
			const FSceneInterface* Scene = GameWorld ? GameWorld->Scene : nullptr;
			if (Scene == nullptr)
			{
				return;
			}
			FRegistry::Get().EnqueueRenderUpdate([Scene, Clamped](FRenderState& State)
			{
				State.SceneVisibility.Add(Scene, Clamped);
			});
		});
	}

	void SetColorPalette(TConstArrayView<FLinearColor> Colors)
	{
		TArray<FVector4f> Normalized;
		Normalized.Reserve(FMath::Min(Colors.Num(), 256));
		for (int32 Index = 0; Index < Colors.Num() && Index < 256; ++Index)
		{
			Normalized.Add(NormalizePaletteColor(Colors[Index]));
		}
		RunOnGameThread([Normalized = MoveTemp(Normalized)]() mutable
		{
			FRegistry::Get().EnqueueRenderUpdate([Normalized = MoveTemp(Normalized)](FRenderState& State)
			{
				for (int32 Index = 0; Index < Normalized.Num(); ++Index)
				{
					State.Palette[Index] = Normalized[Index];
				}
			});
		});
	}

	uint8 GetDefaultPaletteIndex(const float TemperatureKelvin)
	{
		const double Kelvin = FMath::Clamp(static_cast<double>(TemperatureKelvin), PaletteMinKelvin, PaletteMaxKelvin);
		const double T = FMath::Loge(Kelvin / PaletteMinKelvin) / FMath::Loge(PaletteMaxKelvin / PaletteMinKelvin);
		return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(T * 255.0), 0, 255));
	}

	FLinearColor GetDefaultPaletteColor(const uint8 Index)
	{
		const FVector4f Color = NormalizePaletteColor(BlackBodyColor(PaletteKelvin(Index)));
		return FLinearColor(Color.X, Color.Y, Color.Z, 1.0f);
	}

	FStats GetStats()
	{
		FRegistry& Registry = FRegistry::Get();
		FStats Result = Registry.GetStats();
		Result.PointSets = Registry.CountEntries(false);
		Result.GlowVolumes = Registry.CountEntries(true);
		Result.bShadersCompiled = AreShadersEnabled();
		Result.bAtomic64 = GUsingAtomic64.load();
		return Result;
	}

	bool AreShadersEnabled()
	{
		return ShouldCompileShaders() && !GShadersMissingAtRuntime.load();
	}
}
