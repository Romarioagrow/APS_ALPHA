// Rio 03.10 (galaxy phase 3): test commands. aps.Stars.GpuDebug registers a synthetic galaxy (points + glow)
// shaped like the historic two-arm spiral, so the renderer can be judged before the catalogue integration.
#include "APSGalaxyGlowBuilder.h"
#include "APSStarRendererAPI.h"
#include "APSStarRendererPrivate.h"

#include "Async/ParallelFor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

namespace APSStarRenderer::Private
{
	namespace
	{
		FHandle GDebugPointSet = 0;
		FHandle GDebugGlowVolume = 0;

		uint64 SplitMix64(uint64 Value)
		{
			Value += 0x9e3779b97f4a7c15ull;
			Value = (Value ^ (Value >> 30)) * 0xbf58476d1ce4e5b9ull;
			Value = (Value ^ (Value >> 27)) * 0x94d049bb133111ebull;
			return Value ^ (Value >> 31);
		}

		struct FDebugStream
		{
			uint64 State;
			float Next()
			{
				State = SplitMix64(State);
				return static_cast<float>(State >> 40) * (1.0f / 16777216.0f);
			}
		};

		struct FDebugStar
		{
			FVector3f Position;
			uint8 ColorIndex = 0;
			float Intensity = 0.0f;
			float Dust = 0.0f;
		};

		/** Historic-spiral-like sample: 18% halo, 16% bulge (0.22 R), two Archimedean arms of 2.35 turns. Radius 1. */
		FDebugStar MakeDebugStar(const int64 Index)
		{
			FDebugStream Stream{SplitMix64(0x41505331ull ^ static_cast<uint64>(Index))};
			const float Selector = Stream.Next();
			FDebugStar Star;
			bool bArm = false;
			if (Selector < 0.34f)
			{
				const bool bHalo = Selector < 0.18f;
				FVector3f Direction;
				for (int32 Attempt = 0; Attempt < 8; ++Attempt)
				{
					Direction = FVector3f(Stream.Next() * 2.0f - 1.0f, Stream.Next() * 2.0f - 1.0f, Stream.Next() * 2.0f - 1.0f);
					if (Direction.SizeSquared() <= 1.0f && Direction.SizeSquared() > 1.0e-6f)
					{
						break;
					}
				}
				Direction = Direction.GetSafeNormal();
				const float Radius = bHalo ? FMath::Pow(Stream.Next(), 0.55f) : FMath::Pow(Stream.Next(), 1.0f / 3.0f) * 0.22f;
				Star.Position = Direction * Radius;
			}
			else
			{
				bArm = true;
				const float Alpha = FMath::Sqrt(Stream.Next());
				const float ArmAngle = (Stream.Next() < 0.5f ? 0.0f : UE_PI);
				const float Jitter = (Stream.Next() * 2.0f - 1.0f) * 0.22f * (0.35f + Alpha);
				const float Angle = ArmAngle + Alpha * UE_TWO_PI * 2.35f + Jitter;
				const float Height = (Stream.Next() * 2.0f - 1.0f) * 0.025f * (1.15f - Alpha);
				Star.Position = FVector3f(FMath::Cos(Angle) * Alpha, FMath::Sin(Angle) * Alpha, Height);
			}

			// Historic spectral shares; intensity = sqrt(luminosity) keeps the debug view readable.
			const float Class = Stream.Next();
			float Luminosity = 0.045f;
			float Kelvin = 3200.0f;
			if (Class >= 0.975f) { Luminosity = 60000.0f; Kelvin = 35000.0f; }
			else if (Class >= 0.96f) { Luminosity = 1200.0f; Kelvin = 15000.0f; }
			else if (Class >= 0.93f) { Luminosity = 55.0f; Kelvin = 9000.0f; }
			else if (Class >= 0.88f) { Luminosity = 6.0f; Kelvin = 6800.0f; }
			else if (Class >= 0.79f) { Luminosity = 1.0f; Kelvin = 5800.0f; }
			else if (Class >= 0.65f) { Luminosity = 0.35f; Kelvin = 4400.0f; }
			Star.Intensity = FMath::Sqrt(Luminosity);
			Star.ColorIndex = GetDefaultPaletteIndex(Kelvin);
			Star.Dust = bArm ? (Kelvin >= 9000.0f ? 1.0f : 0.35f) : 0.0f;
			return Star;
		}

		bool IsGalaxyActor(const AActor* Actor)
		{
			for (const UClass* Class = Actor ? Actor->GetClass() : nullptr; Class != nullptr; Class = Class->GetSuperClass())
			{
				if (Class->GetFName() == FName(TEXT("Galaxy")))
				{
					return true;
				}
			}
			return false;
		}

		void RemoveDebugGalaxy()
		{
			Remove(GDebugPointSet);
			Remove(GDebugGlowVolume);
			GDebugPointSet = 0;
			GDebugGlowVolume = 0;
		}

		void RunDebugCommand(const TArray<FString>& Args, UWorld* World)
		{
			const int32 Count = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 2000000;
			RemoveDebugGalaxy();
			if (World == nullptr || Count <= 0)
			{
				UE_LOG(LogAPSStarRenderer, Display, TEXT("[APS.GpuStars] debug galaxy removed"));
				return;
			}
			const int32 PointCount = FMath::Clamp(Count, 1000, MaxPointsPerSet);
			const FString Mode = Args.Num() > 1 ? Args[1].ToLower() : FString(TEXT("galaxy"));
			const float GlowShare = Args.Num() > 2 ? FMath::Max(FCString::Atof(*Args[2]), 0.0f) : 1.0f;

			// Camera.
			FVector CameraLocation = FVector::ZeroVector;
			FRotator CameraRotation = FRotator::ZeroRotator;
			float FovDegrees = 90.0f;
			APlayerController* Controller = World->GetFirstPlayerController();
			if (Controller != nullptr && Controller->PlayerCameraManager != nullptr)
			{
				CameraLocation = Controller->PlayerCameraManager->GetCameraLocation();
				CameraRotation = Controller->PlayerCameraManager->GetCameraRotation();
				FovDegrees = Controller->PlayerCameraManager->GetFOVAngle();
			}

			// Placement: over the first Galaxy actor, in front of the camera, or around it (inside view).
			FTransform LocalToWorld = FTransform::Identity;
			bool bPlaced = false;
			if (Mode == TEXT("galaxy"))
			{
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (!IsGalaxyActor(*It))
					{
						continue;
					}
					const FBox LocalBox = It->CalculateComponentsBoundingBoxInLocalSpace(true);
					const double Radius = LocalBox.IsValid ? FMath::Max(LocalBox.GetExtent().X, LocalBox.GetExtent().Y) : 0.0;
					if (Radius > 1.0)
					{
						LocalToWorld = FTransform(FQuat::Identity, LocalBox.GetCenter(), FVector(Radius)) * It->GetActorTransform();
						bPlaced = true;
					}
					break;
				}
			}
			if (!bPlaced)
			{
				const double Radius = 2.0e6;
				if (Mode == TEXT("inside"))
				{
					// The camera sits in the disk at 0.55 R from the centre: the band runs across the sky.
					const FVector Centre = CameraLocation + FRotator(0.0, CameraRotation.Yaw, 0.0).Vector() * (0.55 * Radius);
					LocalToWorld = FTransform(FRotator(0.0, CameraRotation.Yaw, 0.0).Quaternion(), Centre, FVector(Radius));
				}
				else
				{
					const FVector Centre = CameraLocation + CameraRotation.Vector() * (2.5 * Radius);
					LocalToWorld = FTransform((CameraRotation + FRotator(-55.0, 0.0, 0.0)).Quaternion(), Centre, FVector(Radius));
				}
			}

			// Points and glow samples, in parallel; local units: galaxy radius 1.
			constexpr int32 TaskCount = 16;
			constexpr int32 GlowResolution = 256;
			const FBox3f Bounds(FVector3f(-1.05f), FVector3f(1.05f));
			TArray<FPackedStar> Points;
			Points.SetNumUninitialized(PointCount);
			TArray<FGlowMapBuilder> Builders;
			for (int32 Task = 0; Task < TaskCount; ++Task)
			{
				Builders.Emplace(GlowResolution, 1.05f);
			}
			TArray<FLinearColor> Palette;
			Palette.SetNum(256);
			for (int32 Index = 0; Index < 256; ++Index)
			{
				Palette[Index] = GetDefaultPaletteColor(static_cast<uint8>(Index));
			}
			ParallelFor(TaskCount, [&Points, &Builders, &Palette, &Bounds, PointCount](const int32 Task)
			{
				const int32 Begin = static_cast<int32>(static_cast<int64>(PointCount) * Task / TaskCount);
				const int32 End = static_cast<int32>(static_cast<int64>(PointCount) * (Task + 1) / TaskCount);
				for (int32 Index = Begin; Index < End; ++Index)
				{
					const FDebugStar Star = MakeDebugStar(Index);
					Points[Index] = PackStar(Star.Position, Bounds, Star.ColorIndex, Star.Intensity);
					Builders[Task].AddStar(Star.Position, Palette[Star.ColorIndex], Star.Intensity, Star.Dust);
				}
			});
			for (int32 Task = 1; Task < TaskCount; ++Task)
			{
				Builders[0].Merge(Builders[Task]);
			}

			// Brightness: a G star (intensity 1) at the camera distance gives ~0.02 of white per pixel.
			const double CameraDistanceLocal = FMath::Max(LocalToWorld.InverseTransformPosition(CameraLocation).Size(), 0.05);
			const double PixelAngle = 2.0 * FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FovDegrees, 5.0f, 170.0f) * 0.5)) / 1920.0;
			const float IntensityScale = static_cast<float>(0.02 * CameraDistanceLocal * CameraDistanceLocal * PixelAngle * PixelAngle);

			FPointSetDesc PointDesc;
			PointDesc.LocalToWorld = LocalToWorld;
			PointDesc.LocalBounds = Bounds;
			PointDesc.IntensityScale = IntensityScale;
			PointDesc.Population = PointCount;
			PointDesc.DebugName = TEXT("GpuDebug");
			GDebugPointSet = RegisterPointSet(World, PointDesc, MoveTemp(Points));

			FGlowMap Map;
			if (Builders[0].Build(Map))
			{
				FGlowVolumeDesc GlowDesc;
				GlowDesc.LocalToWorld = LocalToWorld;
				GlowDesc.TotalIntensity = Builders[0].GetIntensitySum() * IntensityScale * GlowShare;
				GlowDesc.DustOpacity = 1.5f;
				GlowDesc.DebugName = TEXT("GpuDebug");
				GDebugGlowVolume = RegisterGlowVolume(World, GlowDesc, MoveTemp(Map));
			}

			UE_LOG(LogAPSStarRenderer, Display,
				TEXT("[APS.GpuStars] debug galaxy: %d points, mode %s, radius %.3g cm, camera at %.2f R, intensity scale %.3g, glow share %.2f. ")
				TEXT("Turn on: aps.Stars.GpuPoints 1, aps.Stars.GalaxyGlow 1 (shaders: %s)."),
				PointCount, *Mode, LocalToWorld.GetMaximumAxisScale(), CameraDistanceLocal, IntensityScale, GlowShare,
				ShouldCompileShaders() ? TEXT("compiled") : TEXT("NOT compiled, set aps.Stars.CompileShaders=1 and restart"));
		}

		void RunReportCommand()
		{
			const FStats Stats = GetStats();
			UE_LOG(LogAPSStarRenderer, Display,
				TEXT("[APS.GpuStars] sets %d, glow %d, resident %lld, pending %lld, submitted %lld/frame; GPU (aps.Stars.GpuStats 1, frame %u): ")
				TEXT("tested %lld, in frustum %lld, visible %lld, written %lld; shaders %d, atomic64 %d"),
				Stats.PointSets, Stats.GlowVolumes, Stats.PointsResident, Stats.PointsPending, Stats.PointsSubmitted, Stats.FrameNumber,
				Stats.GpuTested, Stats.GpuInFrustum, Stats.GpuVisible, Stats.GpuWritten, Stats.bShadersCompiled ? 1 : 0,
				Stats.bAtomic64 ? 1 : 0);
		}

		FAutoConsoleCommandWithWorldAndArgs GDebugCommand(
			TEXT("aps.Stars.GpuDebug"),
			TEXT("Test: aps.Stars.GpuDebug [count=2000000] [galaxy|front|inside] [glowShare=1] registers a synthetic two-arm spiral ")
			TEXT("(GPU points + glow) over the first Galaxy actor, in front of the camera, or around it. aps.Stars.GpuDebug 0 removes it."),
			FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunDebugCommand));

		FAutoConsoleCommand GReportCommand(
			TEXT("aps.Stars.GpuReport"),
			TEXT("Logs the GPU star renderer counters (sets, resident points, points per frame, GPU counters with aps.Stars.GpuStats 1)."),
			FConsoleCommandDelegate::CreateStatic(&RunReportCommand));
	}
}
