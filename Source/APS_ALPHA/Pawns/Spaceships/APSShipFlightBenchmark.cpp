#include "APSShipFlightBenchmark.h"

#include "Spaceship.h"
#include "APSShipFlightModel.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Camera/CameraComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Math/RotationMatrix.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "RenderTimer.h"
#include "UnrealClient.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#endif

namespace APSShipBenchmark
{
	struct FStep
	{
		double SpeedMps{0.0};
		TArray<float> FrameMs;
		double GameMsSum{0.0};
		double RenderMsSum{0.0};
		double GpuMsSum{0.0};
		int32 GameSamples{0};
		int32 RenderSamples{0};
		int32 GpuSamples{0};
		double MoveMsSum{0.0};
		double MoveMsMax{0.0};
		int32 MoveSamples{0};
		double LodsInGenerationSum{0.0};
		int32 LodsInGenerationMax{0};
		double DistanceCm{0.0};
		int32 BlockedMoves{0};
	};

	struct FRun
	{
		TWeakObjectPtr<ASpaceship> Ship;
		TWeakObjectPtr<APlanetaryBody> Planet;
		FString ShipName;
		FString PlanetName;
		double BodyRadiusCm{0.0};
		double OrbitRadiusCm{0.0};
		double AltitudeM{0.0};
		FVector AxisU{FVector::ZeroVector};
		FVector AxisV{FVector::ZeroVector};
		double AngleRad{0.0};
		double SecondsPerStep{8.0};
		bool bSweep{true};
		bool bHadPhysicalImpulse{false};
		int32 StepIndex{0};
		double StepElapsed{0.0};
		TArray<FStep> Steps;
		bool bActive{false};
		/** Terrain root of the planet: the generator's own root, or an authored root placed in the map. */
		TWeakObjectPtr<AWorldScapeRoot> Terrain;
		/** Unreal Insights region of the current step (ShipBench_<step>_<speed>). */
		FString Region;
	};

	FRun GRun;

	void BeginStepRegion()
	{
		if (GRun.Steps.IsValidIndex(GRun.StepIndex))
		{
			GRun.Region = FString::Printf(TEXT("ShipBench_%d_%.0f"), GRun.StepIndex + 1, GRun.Steps[GRun.StepIndex].SpeedMps);
			TRACE_BEGIN_REGION(*GRun.Region);
		}
	}

	void EndStepRegion()
	{
		if (!GRun.Region.IsEmpty())
		{
			TRACE_END_REGION(*GRun.Region);
			GRun.Region.Reset();
		}
	}

	double Percentile(TArray<float> Values, double Fraction)
	{
		if (Values.IsEmpty())
		{
			return 0.0;
		}
		Values.Sort();
		const int32 Index = FMath::Clamp(FMath::CeilToInt(Fraction * Values.Num()) - 1, 0, Values.Num() - 1);
		return Values[Index];
	}

	double Average(const TArray<float>& Values)
	{
		double Sum = 0.0;
		for (const float Value : Values)
		{
			Sum += Value;
		}
		return Values.IsEmpty() ? 0.0 : Sum / Values.Num();
	}

	int32 LodsInGeneration(const AWorldScapeRoot* Root)
	{
		return IsValid(Root) ? Root->WorldScapeLodInGeneration.Num() : -1;
	}

	AWorldScapeRoot* FindTerrainRoot(const APlanetaryBody* Planet)
	{
		if (!IsValid(Planet) || !Planet->GetWorld())
		{
			return nullptr;
		}
		const APlanetarySurfaceGenerator* Surface = Planet->PlanetaryEnvironmentGenerator;
		if (IsValid(Surface) && IsValid(Surface->WorldScapeRootInstance))
		{
			return Surface->WorldScapeRootInstance;
		}
		AWorldScapeRoot* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<AWorldScapeRoot> It(Planet->GetWorld()); It; ++It)
		{
			const double Distance = FVector::Distance(It->GetActorLocation(), Planet->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		return Best;
	}

	ASpaceship* FindPilotedShipInWorld(const UWorld* World)
	{
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		return PlayerController ? Cast<ASpaceship>(PlayerController->GetPawn()) : nullptr;
	}

	/** The console may pass the editor world while the ship flies in PIE, so game worlds are searched too. */
	ASpaceship* FindPilotedShip(UWorld* World)
	{
		if (ASpaceship* Ship = FindPilotedShipInWorld(World))
		{
			return Ship;
		}
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
				{
					if (ASpaceship* Ship = FindPilotedShipInWorld(Context.World()))
					{
						return Ship;
					}
				}
			}
		}
		return nullptr;
	}

	APlanetaryBody* FindPlanet(const ASpaceship& Ship)
	{
		if (APlanetaryBody* Source = Cast<APlanetaryBody>(Ship.ActiveGravitySource.Get()))
		{
			return Source;
		}
		APlanetaryBody* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<APlanetaryBody> It(Ship.GetWorld()); It; ++It)
		{
			const double Distance = FVector::Distance(It->GetActorLocation(), Ship.GetActorLocation())
				- It->GetWorldScapeBodyRadiusCm();
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		return Best;
	}

	FString DescribeStep(const FStep& Step)
	{
		return FString::Printf(
			TEXT("speed=%.0f m/s frames=%d frame avg %.2f p95 %.2f p99 %.2f max %.2f ms | game %.2f render %.2f gpu %.2f ms"
				" | move avg %.3f max %.3f ms | WorldScape LODs generating avg %.1f max %d | flown %.1f km blocked=%d"),
			Step.SpeedMps, Step.FrameMs.Num(), Average(Step.FrameMs), Percentile(Step.FrameMs, 0.95),
			Percentile(Step.FrameMs, 0.99), Percentile(Step.FrameMs, 1.0),
			Step.GameSamples ? Step.GameMsSum / Step.GameSamples : -1.0,
			Step.RenderSamples ? Step.RenderMsSum / Step.RenderSamples : -1.0,
			Step.GpuSamples ? Step.GpuMsSum / Step.GpuSamples : -1.0,
			Step.MoveSamples ? Step.MoveMsSum / Step.MoveSamples : 0.0, Step.MoveMsMax,
			Step.FrameMs.Num() ? Step.LodsInGenerationSum / Step.FrameMs.Num() : 0.0, Step.LodsInGenerationMax,
			Step.DistanceCm / 100000.0, Step.BlockedMoves);
	}

	void Finish(const TCHAR* Reason)
	{
		if (!GRun.bActive)
		{
			return;
		}
		GRun.bActive = false;
		EndStepRegion();
		if (ASpaceship* Ship = GRun.Ship.Get())
		{
			FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
			FAPSShipFlightBenchmark::SetBenchmarkKinematic(*Ship, false, GRun.bHadPhysicalImpulse);
		}

		FString Csv = TEXT("ship,planet,altitude_m,sweep,speed_mps,frames,frame_avg_ms,frame_p95_ms,frame_p99_ms,frame_max_ms,")
			TEXT("game_ms,render_ms,gpu_ms,move_avg_ms,move_max_ms,lods_generating_avg,lods_generating_max,flown_km,blocked\n");
		for (const FStep& Step : GRun.Steps)
		{
			if (Step.FrameMs.IsEmpty())
			{
				continue;
			}
			Csv += FString::Printf(TEXT("%s,%s,%.0f,%d,%.0f,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.2f,%d,%.2f,%d\n"),
				*GRun.ShipName, *GRun.PlanetName, GRun.AltitudeM, GRun.bSweep ? 1 : 0, Step.SpeedMps, Step.FrameMs.Num(),
				Average(Step.FrameMs), Percentile(Step.FrameMs, 0.95), Percentile(Step.FrameMs, 0.99),
				Percentile(Step.FrameMs, 1.0),
				Step.GameSamples ? Step.GameMsSum / Step.GameSamples : -1.0,
				Step.RenderSamples ? Step.RenderMsSum / Step.RenderSamples : -1.0,
				Step.GpuSamples ? Step.GpuMsSum / Step.GpuSamples : -1.0,
				Step.MoveSamples ? Step.MoveMsSum / Step.MoveSamples : 0.0, Step.MoveMsMax,
				Step.LodsInGenerationSum / Step.FrameMs.Num(), Step.LodsInGenerationMax,
				Step.DistanceCm / 100000.0, Step.BlockedMoves);
		}
		const FString Path = FPaths::ProjectSavedDir() / TEXT("Diagnostics/ShipFlight")
			/ FString::Printf(TEXT("%s_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")), *GRun.ShipName);
		const bool bSaved = FFileHelper::SaveStringToFile(Csv, *Path);
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] finished (%s) ship=%s steps=%d csv=%s saved=%d"),
			Reason, *GRun.ShipName, GRun.Steps.Num(), *FPaths::ConvertRelativePathToFull(Path), bSaved ? 1 : 0);
		GRun = FRun();
	}

	void Start(const TArray<FString>& Args, UWorld* World)
	{
		if (GRun.bActive)
		{
			Finish(TEXT("restarted"));
		}
		ASpaceship* Ship = FindPilotedShip(World);
		if (!Ship)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ShipBench] pilot a ship first (no player-controlled ASpaceship found)."));
			return;
		}
		APlanetaryBody* Planet = FindPlanet(*Ship);
		if (!Planet)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ShipBench] no planetary body in %s."), *GetNameSafe(Ship->GetWorld()));
			return;
		}

		FRun Run;
		Run.AltitudeM = Args.Num() > 0 ? FMath::Max(FCString::Atod(*Args[0]), 10.0) : 1000.0;
		Run.SecondsPerStep = Args.Num() > 1 ? FMath::Clamp(FCString::Atod(*Args[1]), 2.0, 120.0) : 8.0;
		// '/' also separates speeds: -ExecCmds splits its commands at commas.
		FString SpeedList = Args.Num() > 2 ? Args[2] : FString(TEXT("0,100,400,1600,6400,25600"));
		SpeedList.ReplaceCharInline(TEXT('/'), TEXT(','));
		TArray<FString> SpeedTokens;
		SpeedList.ParseIntoArray(SpeedTokens, TEXT(","));
		for (const FString& Token : SpeedTokens)
		{
			FStep Step;
			Step.SpeedMps = FMath::Max(FCString::Atod(*Token), 0.0);
			Run.Steps.Add(MoveTemp(Step));
		}
		Run.bSweep = Args.Num() > 3 ? FCString::Atoi(*Args[3]) != 0 : true;
		if (Run.Steps.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ShipBench] no speeds given."));
			return;
		}

		const FVector Center = Planet->GetActorLocation();
		FVector Radial = (Ship->GetActorLocation() - Center).GetSafeNormal();
		if (Radial.IsNearlyZero())
		{
			Radial = FVector::UpVector;
		}
		FVector Tangent = FVector::VectorPlaneProject(Ship->GetShipForwardVector(), Radial).GetSafeNormal();
		if (Tangent.IsNearlyZero())
		{
			Tangent = FVector::CrossProduct(Radial, FMath::Abs(Radial.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector)
				.GetSafeNormal();
		}
		Run.Ship = Ship;
		Run.Planet = Planet;
		Run.ShipName = Ship->GetClass()->GetName();
		Run.PlanetName = Planet->AstroName.IsNone() ? Planet->GetName() : Planet->AstroName.ToString();
		Run.BodyRadiusCm = Planet->GetWorldScapeBodyRadiusCm();
		Run.OrbitRadiusCm = Run.BodyRadiusCm + Run.AltitudeM * 100.0;
		Run.AxisU = Radial;
		Run.AxisV = Tangent;
		Run.Terrain = FindTerrainRoot(Planet);
		Run.bActive = true;

		FAPSShipFlightBenchmark::SetBenchmarkKinematic(*Ship, true, Run.bHadPhysicalImpulse);
		Ship->SetActorLocationAndRotation(Center + Radial * Run.OrbitRadiusCm,
			FAPSShipFlightBenchmark::GetRotationForFlightAxes(*Ship, Tangent, Radial), false, nullptr,
			ETeleportType::TeleportPhysics);
		GRun = MoveTemp(Run);
		BeginStepRegion();
		FString Speeds;
		for (const FStep& Step : GRun.Steps)
		{
			Speeds += FString::Printf(TEXT("%s%.0f"), Speeds.IsEmpty() ? TEXT("") : TEXT(","), Step.SpeedMps);
		}
		UE_LOG(LogTemp, Log,
			TEXT("[APS.ShipBench] start ship=%s planet=%s terrain=%s radius=%.0f km altitude=%.0f m step=%.1f s speeds=%s m/s sweep=%d precheck=%d"),
			*GRun.ShipName, *GRun.PlanetName, *GetNameSafe(GRun.Terrain.Get()), GRun.BodyRadiusCm / 100000.0, GRun.AltitudeM,
			GRun.SecondsPerStep,
			*Speeds, GRun.bSweep ? 1 : 0,
			IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Ship.SweepPrecheck"))
				? IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Ship.SweepPrecheck"))->GetInt() : -1);
		FAPSShipFlightBenchmark::LogShipReport(*Ship);
	}

	void Stop(UWorld*)
	{
		Finish(TEXT("stopped"));
	}

	/**
	 * Puts the player's pawn into the nearest free ship whose actor or class name contains Filter. bSkipChildActors
	 * skips display copies that stations carry as child actors (the generated home system has one in the headquarters).
	 */
	ASpaceship* BoardShip(APawn* Pawn, const FString& Filter, bool bSkipChildActors = false)
	{
		if (!IsValid(Pawn))
		{
			return nullptr;
		}
		if (ASpaceship* Current = Cast<ASpaceship>(Pawn))
		{
			return Current;
		}
		ASpaceship* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<ASpaceship> It(Pawn->GetWorld()); It; ++It)
		{
			ASpaceship* Candidate = *It;
			if (!IsValid(Candidate) || Candidate->HasPilot() || (bSkipChildActors && Candidate->IsChildActor())
				|| (!Filter.IsEmpty() && !Candidate->GetName().Contains(Filter) && !Candidate->GetClass()->GetName().Contains(Filter)))
			{
				continue;
			}
			const double Distance = FVector::Distance(Candidate->GetActorLocation(), Pawn->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Candidate;
			}
		}
		if (!Best || !Best->RequestVehicleControl(Pawn))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ShipBench] no free ship matching '%s' could take %s."), *Filter, *Pawn->GetName());
			return nullptr;
		}
		if (!Best->bEngineRunning)
		{
			Best->SwitchEngines();
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] boarded %s (%s) at %.0f m"), *Best->GetName(), *Best->GetClass()->GetName(),
			BestDistance / 100.0);
		return Best;
	}

	APawn* FindPlayerPawn(UWorld* World)
	{
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		if (PlayerController && PlayerController->GetPawn())
		{
			return PlayerController->GetPawn();
		}
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
				{
					const APlayerController* ContextController = Context.World() ? Context.World()->GetFirstPlayerController() : nullptr;
					if (ContextController && ContextController->GetPawn())
					{
						return ContextController->GetPawn();
					}
				}
			}
		}
		return nullptr;
	}

	void Board(const TArray<FString>& Args, UWorld* World)
	{
		BoardShip(FindPlayerPawn(World), Args.Num() > 0 ? Args[0] : FString());
	}

	/** aps.Ship.Drive: a pilot holding power step, thrust and boost; the ship's own flight code does the rest. */
	struct FDrive
	{
		bool bActive{false};
		TWeakObjectPtr<ASpaceship> Ship;
		TWeakObjectPtr<APlanetaryBody> Planet;
		/** aim=next: the planet flown to, logged every second with its surface readiness. */
		TWeakObjectPtr<APlanetaryBody> Target;
		TWeakObjectPtr<AWorldScapeRoot> Terrain;
		int32 Power{3};
		float Forward{1.0f};
		bool bBoost{false};
		FString Aim;
		double PitchDegrees{0.0};
		double MinimumAltitudeKm{0.0};
		/** startly=<ly>: before driving, carry the ship this far from the world origin along aim=out (edge tests). */
		double StartLightYears{0.0};
		/** Held yaw stick, -1..1: a steady turn, as when a pilot steers with the mouse. */
		float Yaw{0.0f};
		/** Engine key pressed once at the start: 1 Impulse, 2 SpaceWrap, 3 Offset, 0 keeps the current one. */
		int32 Engine{0};
		/** shots=3/8/13: at these drive seconds capture Burst consecutive frames of the scene (no UI) to
		 * Saved/Screenshots/ShipDrive/<label>_t<second>_f<frame>.png. The hull is still relative to the camera, so
		 * consecutive frames should agree on it; temporal shimmer shows up as frame-to-frame differences. */
		TArray<double> ShotTimes;
		int32 Burst{4};
		int32 ShotIndex{0};
		int32 BurstFrame{0};
		int32 BurstRemaining{0};
		FString ShotLabel{TEXT("drive")};
		/** shotui=1: the shots include the HUD and viewport overlays (layout checks); by default the scene only. */
		bool bShotUI{false};
		bool bAimed{false};
		double Elapsed{0.0};
		double MaxSpeedKmS{0.0};
		double MinAltitudeSeenKm{TNumericLimits<double>::Max()};
		int32 Second{0};
		/** Unreal Insights region of the current second (ShipDrive_<second>), matching the per-second log line. */
		FString Region;
		// Current one-second window.
		double WindowElapsed{0.0};
		int32 Frames{0};
		double FrameSum{0.0};
		double FrameMax{0.0};
		int32 Hitches{0};
		double GameSum{0.0};
		double RenderSum{0.0};
		double GpuSum{0.0};
		int32 GameSamples{0};
		int32 RenderSamples{0};
		int32 GpuSamples{0};
		double LodsSum{0.0};
		int32 LodsMax{0};
		// Camera relative to the hull over the window, as the pilot sees the ship: distance range, largest frame-to-frame
		// move, largest change of that move (a jerk shows as a spike) and largest turn ([APS.ShipCam], K12).
		bool bCameraSampled{false};
		bool bCameraStepped{false};
		FVector CameraOffset{FVector::ZeroVector};
		FVector CameraStep{FVector::ZeroVector};
		FQuat CameraRotation{FQuat::Identity};
		double CameraDistanceMin{TNumericLimits<double>::Max()};
		double CameraDistanceMax{0.0};
		double CameraStepMax{0.0};
		double CameraJerkMax{0.0};
		double CameraTurnMax{0.0};
		// Whole drive.
		int32 TotalFrames{0};
		double TotalFrameSum{0.0};
		int32 TotalHitches{0};
		TArray<float> AllFrameMs;
	};
	FDrive GDrive;

	double AltitudeKm(const ASpaceship& Ship, const APlanetaryBody* Planet)
	{
		return IsValid(Planet)
			? (FVector::Distance(Ship.GetActorLocation(), Planet->GetActorLocation()) - Planet->GetWorldScapeBodyRadiusCm()) / 100000.0
			: -1.0;
	}

	void BeginDriveRegion()
	{
		GDrive.Region = FString::Printf(TEXT("ShipDrive_%03d"), GDrive.Second);
		TRACE_BEGIN_REGION(*GDrive.Region);
	}

	void EndDriveRegion()
	{
		if (!GDrive.Region.IsEmpty())
		{
			TRACE_END_REGION(*GDrive.Region);
			GDrive.Region.Reset();
		}
	}

	void StopDrive(const TCHAR* Reason)
	{
		if (!GDrive.bActive)
		{
			return;
		}
		EndDriveRegion();
		if (ASpaceship* Ship = GDrive.Ship.Get())
		{
			FAPSShipFlightBenchmark::SetPilotControls(*Ship, GDrive.Power, 0.0f, false, false);
		}
		UE_LOG(LogTemp, Log,
			TEXT("[APS.ShipDrive] finished (%s) after %.1f s: frames=%d frame avg %.2f p95 %.2f p99 %.2f max %.2f ms hitches(>33 ms)=%d | top speed %.2f km/s, lowest altitude %.1f km"),
			Reason, GDrive.Elapsed, GDrive.TotalFrames, GDrive.TotalFrames ? GDrive.TotalFrameSum / GDrive.TotalFrames : 0.0,
			Percentile(GDrive.AllFrameMs, 0.95), Percentile(GDrive.AllFrameMs, 0.99), Percentile(GDrive.AllFrameMs, 1.0),
			GDrive.TotalHitches, GDrive.MaxSpeedKmS,
			GDrive.MinAltitudeSeenKm < TNumericLimits<double>::Max() ? GDrive.MinAltitudeSeenKm : -1.0);
		GDrive = FDrive();
	}

	/** Points the nose at the planet (aim=planet), away from it (up), along the horizon, then pitches towards "up". */
	void AimShip(ASpaceship& Ship)
	{
		const bool bKeep = GDrive.Aim.IsEmpty() || GDrive.Aim.Equals(TEXT("keep"), ESearchCase::IgnoreCase);
		if (bKeep && FMath::IsNearlyZero(GDrive.PitchDegrees))
		{
			return;
		}
		const APlanetaryBody* Planet = GDrive.Planet.Get();
		const FVector Radial = IsValid(Planet)
			? (Ship.GetActorLocation() - Planet->GetActorLocation()).GetSafeNormal() : Ship.GetActorUpVector();
		FVector Forward = Ship.GetShipForwardVector();
		if (GDrive.Aim.Equals(TEXT("planet"), ESearchCase::IgnoreCase))
		{
			Forward = -Radial;
		}
		else if (GDrive.Aim.Equals(TEXT("up"), ESearchCase::IgnoreCase))
		{
			Forward = Radial;
		}
		else if (GDrive.Aim.Equals(TEXT("next"), ESearchCase::IgnoreCase))
		{
			// The nearest other planet with a streamed surface: an approach from afar (arrival forecast, 01.10).
			double Best = TNumericLimits<double>::Max();
			for (TActorIterator<APlanetaryBody> It(Ship.GetWorld()); It && !GDrive.Target.IsValid(); ++It)
			{
				const double Distance = FVector::Dist(Ship.GetActorLocation(), It->GetActorLocation());
				if (IsValid(*It) && *It != Planet && It->bStreamWorldScapeSurface && !It->IsA<AMoon>() && Distance < Best)
				{
					Best = Distance;
					GDrive.Target = *It;
				}
			}
			if (const APlanetaryBody* Target = GDrive.Target.Get())
			{
				Forward = (Target->GetActorLocation() - Ship.GetActorLocation()).GetSafeNormal();
			}
		}
		else if (GDrive.Aim.Equals(TEXT("out"), ESearchCase::IgnoreCase))
		{
			// Away from the world origin (the spawn point), toward the edge of charted space.
			const FVector FromOrigin = Ship.GetActorLocation().GetSafeNormal();
			Forward = FromOrigin.IsNearlyZero() ? Forward : FromOrigin;
		}
		else if (GDrive.Aim.Equals(TEXT("horizon"), ESearchCase::IgnoreCase))
		{
			Forward = FVector::VectorPlaneProject(Forward, Radial).GetSafeNormal();
			if (Forward.IsNearlyZero())
			{
				Forward = FVector::CrossProduct(Radial, FMath::Abs(Radial.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector)
					.GetSafeNormal();
			}
		}
		if (!FMath::IsNearlyZero(GDrive.PitchDegrees))
		{
			FVector Axis = FVector::CrossProduct(Forward, Radial).GetSafeNormal();
			if (Axis.IsNearlyZero())
			{
				Axis = FVector::CrossProduct(Forward, Ship.GetActorForwardVector()).GetSafeNormal();
			}
			if (Axis.IsNearlyZero())
			{
				Axis = FVector::CrossProduct(Forward, Ship.GetActorUpVector()).GetSafeNormal();
			}
			Forward = Forward.RotateAngleAxis(GDrive.PitchDegrees, Axis).GetSafeNormal();
		}
		FVector Up = FVector::VectorPlaneProject(Radial, Forward).GetSafeNormal();
		if (Up.IsNearlyZero())
		{
			Up = FVector::VectorPlaneProject(Ship.GetActorUpVector(), Forward).GetSafeNormal();
		}
		if (Up.IsNearlyZero())
		{
			Up = FVector::VectorPlaneProject(Ship.GetActorForwardVector(), Forward).GetSafeNormal();
		}
		Ship.SetActorRotation(FAPSShipFlightBenchmark::GetRotationForFlightAxes(Ship, Forward, Up), ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] aimed aim=%s pitch=%.0f deg: nose %.2f deg from the planet direction"),
			bKeep ? TEXT("keep") : *GDrive.Aim, GDrive.PitchDegrees,
			FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Ship.GetShipForwardVector(), -Radial), -1.0, 1.0))));
	}

	void TickDrive(ASpaceship& Ship, float DeltaTime)
	{
		if (!Ship.HasPilot())
		{
			StopDrive(TEXT("pilot left the ship"));
			return;
		}
		if (!GDrive.bAimed)
		{
			if (GDrive.StartLightYears > 0.0)
			{
				// Edge-of-charted-space tests: carry the ship (with its seated pilot) far out in one step.
				FVector Out = Ship.GetActorLocation().GetSafeNormal();
				if (Out.IsNearlyZero())
				{
					Out = Ship.GetShipForwardVector();
				}
				Ship.DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				Ship.SetActorLocation(Out * GDrive.StartLightYears * 9.4607e17, false, nullptr, ETeleportType::TeleportPhysics);
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] moved to %.2f ly from the world origin"),
					Ship.GetActorLocation().Size() / 9.4607e17);
			}
			if (GDrive.Aim.Equals(TEXT("next"), ESearchCase::IgnoreCase))
			{
				// Arrival tests: start halfway to the target in open space, out of the home planet's gravity well.
				AimShip(Ship);
				if (const APlanetaryBody* Target = GDrive.Target.Get())
				{
					const FVector Start = FMath::Lerp(Ship.GetActorLocation(), Target->GetActorLocation(), 0.5);
					Ship.DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
					Ship.SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
					UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] moved halfway to %s: its surface %.0f km away"),
						*Target->GetName(),
						(FVector::Dist(Start, Target->GetActorLocation()) - Target->GetWorldScapeBodyRadiusCm()) / 1.0e5);
				}
			}
			AimShip(Ship);
			if (GDrive.Engine > 0)
			{
				FAPSShipFlightBenchmark::SelectEngine(Ship, GDrive.Engine);
			}
			GDrive.bAimed = true;
			BeginDriveRegion();
		}
		FAPSShipFlightBenchmark::SetPilotControls(Ship, GDrive.Power, GDrive.Forward, GDrive.bBoost, false, GDrive.Yaw);

		// The engine publishes the previous frame's thread times.
		const float FrameMs = DeltaTime * 1000.0f;
		GDrive.Elapsed += DeltaTime;
		GDrive.WindowElapsed += DeltaTime;
		++GDrive.Frames;
		GDrive.FrameSum += FrameMs;
		GDrive.FrameMax = FMath::Max(GDrive.FrameMax, static_cast<double>(FrameMs));
		GDrive.Hitches += FrameMs > 33.4f ? 1 : 0;
		++GDrive.TotalFrames;
		GDrive.TotalFrameSum += FrameMs;
		GDrive.TotalHitches += FrameMs > 33.4f ? 1 : 0;
		GDrive.AllFrameMs.Add(FrameMs);
		if (const uint32 GameCycles = GGameThreadTime)
		{
			GDrive.GameSum += FPlatformTime::ToMilliseconds(GameCycles);
			++GDrive.GameSamples;
		}
		if (const uint32 RenderCycles = GRenderThreadTime)
		{
			GDrive.RenderSum += FPlatformTime::ToMilliseconds(RenderCycles);
			++GDrive.RenderSamples;
		}
		if (const uint32 GpuCycles = GGPUFrameTime)
		{
			GDrive.GpuSum += FPlatformTime::ToMilliseconds(GpuCycles);
			++GDrive.GpuSamples;
		}
		if (IsValid(Ship.CameraComponent))
		{
			const FTransform View = Ship.CameraComponent->GetComponentTransform().GetRelativeTransform(Ship.GetActorTransform());
			const FVector Offset = View.GetLocation();
			if (GDrive.bCameraSampled)
			{
				const FVector Step = Offset - GDrive.CameraOffset;
				GDrive.CameraStepMax = FMath::Max(GDrive.CameraStepMax, Step.Size());
				if (GDrive.bCameraStepped)
				{
					GDrive.CameraJerkMax = FMath::Max(GDrive.CameraJerkMax, (Step - GDrive.CameraStep).Size());
				}
				GDrive.CameraTurnMax = FMath::Max(GDrive.CameraTurnMax,
					FMath::RadiansToDegrees(View.GetRotation().AngularDistance(GDrive.CameraRotation)));
				GDrive.CameraStep = Step;
				GDrive.bCameraStepped = true;
			}
			GDrive.CameraOffset = Offset;
			GDrive.CameraRotation = View.GetRotation();
			GDrive.bCameraSampled = true;
			GDrive.CameraDistanceMin = FMath::Min(GDrive.CameraDistanceMin, Offset.Size());
			GDrive.CameraDistanceMax = FMath::Max(GDrive.CameraDistanceMax, Offset.Size());
		}
		const int32 Lods = LodsInGeneration(GDrive.Terrain.Get());
		GDrive.LodsSum += FMath::Max(Lods, 0);
		GDrive.LodsMax = FMath::Max(GDrive.LodsMax, Lods);

		const double SpeedKmS = Ship.GetShipSpeedMetersPerSecond() / 1000.0;
		const double Altitude = AltitudeKm(Ship, GDrive.Planet.Get());
		if (GDrive.BurstRemaining <= 0 && GDrive.ShotTimes.IsValidIndex(GDrive.ShotIndex)
			&& GDrive.Elapsed >= GDrive.ShotTimes[GDrive.ShotIndex])
		{
			GDrive.BurstRemaining = GDrive.Burst;
			GDrive.BurstFrame = 0;
			++GDrive.ShotIndex;
		}
		if (GDrive.BurstRemaining > 0)
		{
			const FString File = FPaths::ScreenShotDir() / TEXT("ShipDrive") / FString::Printf(TEXT("%s_t%03.0f_f%d.png"),
				*GDrive.ShotLabel, GDrive.ShotTimes[GDrive.ShotIndex - 1], GDrive.BurstFrame);
			FScreenshotRequest::RequestScreenshot(File, GDrive.bShotUI, false);
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] shot %s speed=%.2f km/s frame=%.2f ms"),
				*FPaths::GetCleanFilename(File), SpeedKmS, FrameMs);
			++GDrive.BurstFrame;
			--GDrive.BurstRemaining;
		}
		GDrive.MaxSpeedKmS = FMath::Max(GDrive.MaxSpeedKmS, SpeedKmS);
		if (Altitude >= 0.0)
		{
			GDrive.MinAltitudeSeenKm = FMath::Min(GDrive.MinAltitudeSeenKm, Altitude);
		}

		if (GDrive.WindowElapsed >= 1.0)
		{
			const APlanetaryBody* Planet = GDrive.Planet.Get();
			UE_LOG(LogTemp, Log,
				TEXT("[APS.ShipDrive] t=%3d s speed=%8.2f km/s altitude=%9.1f km planet=%s drive=%s engine=%s env=%s | frames=%d frame avg %.2f max %.2f ms hitches=%d | game %.2f render %.2f gpu %.2f ms | WorldScape LODs generating avg %.1f max %d"),
				GDrive.Second + 1, SpeedKmS, Altitude,
				IsValid(Planet) ? (Planet->AstroName.IsNone() ? *Planet->GetName() : *Planet->AstroName.ToString()) : TEXT("none"),
				Ship.FlightModel && Ship.FlightModel->IsBandFlightActive()
					? *Ship.FlightModel->GetBandSettings(Ship.FlightModel->GetFlightBand()).Name : *Ship.GetDriveModeName(),
				Ship.FlightModel && Ship.FlightModel->IsBandFlightActive()
					? *FString::Printf(TEXT("band limit %.2f km/s"), Ship.FlightModel->GetCurrentSpeedLimit() / 1000.0)
					: *Ship.GetEngineModeName(),
				*Ship.GetFlightEnvironmentName(), GDrive.Frames,
				GDrive.FrameSum / GDrive.Frames, GDrive.FrameMax, GDrive.Hitches,
				GDrive.GameSamples ? GDrive.GameSum / GDrive.GameSamples : -1.0,
				GDrive.RenderSamples ? GDrive.RenderSum / GDrive.RenderSamples : -1.0,
				GDrive.GpuSamples ? GDrive.GpuSum / GDrive.GpuSamples : -1.0,
				GDrive.LodsSum / GDrive.Frames, GDrive.LodsMax);
			if (const APlanetaryBody* Target = GDrive.Target.Get())
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] t=%3d s target %s: surface %.0f km away, surface ready=%d"),
					GDrive.Second + 1, Target->AstroName.IsNone() ? *Target->GetName() : *Target->AstroName.ToString(),
					(FVector::Dist(Ship.GetActorLocation(), Target->GetActorLocation()) - Target->GetWorldScapeBodyRadiusCm())
						/ 1.0e5, Target->bWorldScapeSurfaceReady ? 1 : 0);
			}
			if (GDrive.bCameraSampled)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipCam] t=%3d s arm %.1f-%.1f m | step max %.2f cm jerk max %.3f cm turn max %.3f deg"),
					GDrive.Second + 1, GDrive.CameraDistanceMin / 100.0, GDrive.CameraDistanceMax / 100.0,
					GDrive.CameraStepMax, GDrive.CameraJerkMax, GDrive.CameraTurnMax);
			}
			GDrive.CameraDistanceMin = TNumericLimits<double>::Max();
			GDrive.CameraDistanceMax = GDrive.CameraStepMax = GDrive.CameraJerkMax = GDrive.CameraTurnMax = 0.0;
			EndDriveRegion();
			++GDrive.Second;
			GDrive.WindowElapsed = 0.0;
			GDrive.Frames = 0;
			GDrive.FrameSum = GDrive.FrameMax = 0.0;
			GDrive.Hitches = 0;
			GDrive.GameSum = GDrive.RenderSum = GDrive.GpuSum = 0.0;
			GDrive.GameSamples = GDrive.RenderSamples = GDrive.GpuSamples = 0;
			GDrive.LodsSum = 0.0;
			GDrive.LodsMax = 0;
			// The nearest body changes along the way (station, planet, moon).
			GDrive.Planet = FindPlanet(Ship);
			GDrive.Terrain = FindTerrainRoot(GDrive.Planet.Get());
			BeginDriveRegion();
		}
		if (GDrive.MinimumAltitudeKm > 0.0 && Altitude >= 0.0 && Altitude < GDrive.MinimumAltitudeKm)
		{
			// Stop short of the ground: a kinematic hull without a sweep would fly through the terrain.
			FAPSShipFlightBenchmark::SetKinematicVelocity(Ship, FVector::ZeroVector);
			StopDrive(TEXT("minimum altitude reached, velocity cleared"));
		}
	}

	void StartDrive(const TArray<FString>& Args, UWorld* World)
	{
		ASpaceship* Ship = FindPilotedShip(World);
		if (!Ship)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ShipDrive] pilot a ship first (no player-controlled ASpaceship found)."));
			return;
		}
		StopDrive(TEXT("restarted"));
		FDrive Drive;
		Drive.Ship = Ship;
		Drive.Power = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 0, 6) : 3;
		Drive.Forward = Args.Num() > 1 ? FMath::Clamp(FCString::Atof(*Args[1]), -1.0f, 1.0f) : 1.0f;
		Drive.bBoost = Args.Num() > 2 && FCString::Atoi(*Args[2]) != 0;
		for (int32 Index = 3; Index < Args.Num(); ++Index)
		{
			const FString& Arg = Args[Index];
			if (Arg.StartsWith(TEXT("aim="), ESearchCase::IgnoreCase))
			{
				Drive.Aim = Arg.RightChop(4);
			}
			else if (Arg.StartsWith(TEXT("pitch="), ESearchCase::IgnoreCase))
			{
				Drive.PitchDegrees = FMath::Clamp(FCString::Atod(*Arg.RightChop(6)), -90.0, 90.0);
			}
			else if (Arg.StartsWith(TEXT("minalt="), ESearchCase::IgnoreCase))
			{
				Drive.MinimumAltitudeKm = FMath::Max(FCString::Atod(*Arg.RightChop(7)), 0.0);
			}
			else if (Arg.StartsWith(TEXT("startly="), ESearchCase::IgnoreCase))
			{
				Drive.StartLightYears = FMath::Clamp(FCString::Atod(*Arg.RightChop(8)), 0.0, 60.0);
			}
			else if (Arg.StartsWith(TEXT("yaw="), ESearchCase::IgnoreCase))
			{
				Drive.Yaw = FMath::Clamp(FCString::Atof(*Arg.RightChop(4)), -1.0f, 1.0f);
			}
			else if (Arg.StartsWith(TEXT("engine="), ESearchCase::IgnoreCase))
			{
				Drive.Engine = FMath::Clamp(FCString::Atoi(*Arg.RightChop(7)), 0, 3);
			}
			else if (Arg.StartsWith(TEXT("shots="), ESearchCase::IgnoreCase))
			{
				TArray<FString> Times;
				Arg.RightChop(6).ParseIntoArray(Times, TEXT("/"));
				for (const FString& Time : Times)
				{
					Drive.ShotTimes.Add(FMath::Max(FCString::Atod(*Time), 0.0));
				}
				Drive.ShotTimes.Sort();
			}
			else if (Arg.StartsWith(TEXT("burst="), ESearchCase::IgnoreCase))
			{
				Drive.Burst = FMath::Clamp(FCString::Atoi(*Arg.RightChop(6)), 1, 30);
			}
			else if (Arg.StartsWith(TEXT("shotlabel="), ESearchCase::IgnoreCase))
			{
				Drive.ShotLabel = Arg.RightChop(10);
			}
			else if (Arg.StartsWith(TEXT("shotui="), ESearchCase::IgnoreCase))
			{
				Drive.bShotUI = FCString::Atoi(*Arg.RightChop(7)) != 0;
			}
		}
		Drive.Planet = FindPlanet(*Ship);
		Drive.Terrain = FindTerrainRoot(Drive.Planet.Get());
		Drive.bActive = true;
		GDrive = MoveTemp(Drive);
		UE_LOG(LogTemp, Log,
			TEXT("[APS.ShipDrive] start ship=%s class=%s power=%d forward=%.2f boost=%d yaw=%.2f aim=%s pitch=%.0f minalt=%.0f km speedFov=%d planet=%s terrain=%s altitude=%.1f km"),
			*Ship->GetName(), *Ship->GetClass()->GetName(), GDrive.Power, GDrive.Forward, GDrive.bBoost ? 1 : 0, GDrive.Yaw,
			GDrive.Aim.IsEmpty() ? TEXT("keep") : *GDrive.Aim, GDrive.PitchDegrees, GDrive.MinimumAltitudeKm,
			IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Ship.SpeedFov"))
				? IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Ship.SpeedFov"))->GetInt() : -1,
			*GetNameSafe(GDrive.Planet.Get()), *GetNameSafe(GDrive.Terrain.Get()), AltitudeKm(*Ship, GDrive.Planet.Get()));
		FAPSShipFlightBenchmark::LogShipReport(*Ship);
	}

	void StopDriveCommand(UWorld*)
	{
		StopDrive(TEXT("stopped"));
	}

#if WITH_DEV_AUTOMATION_TESTS
	/** aps.Ship.StartGenerated: the menu's own Civilization start into L_WorldGeneration, without clicking through the UI. */
	struct FGeneratedStart
	{
		bool bActive{false};
		bool bOpened{false};
		int32 SpawnPlace{0};
		/** Moons around the home planet for lunar starts (-1: the menu's own); applied before the start is chosen. */
		int32 Moons{-1};
		/** Total starting ships, the home ship included (-1: the menu's own; CORE is 10), for fleet command runs. */
		int32 Fleet{-1};
		bool bMoonsApplied{false};
		FString ShipClassPath;
		FString CharacterClassPath;
		double StartSeconds{0.0};
		double ReadySeconds{0.0};
		FTSTicker::FDelegateHandle Ticker;
	};
	FGeneratedStart GGeneratedStart;

	AMainMenuController* FindMenuController()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World())
			{
				if (AMainMenuController* Controller = Cast<AMainMenuController>(Context.World()->GetFirstPlayerController()))
				{
					return Controller;
				}
			}
		}
		return nullptr;
	}

	bool TickGeneratedStart(float)
	{
		if (!GGeneratedStart.bActive)
		{
			return false;
		}
		const double Now = FPlatformTime::Seconds();
		if (Now - GGeneratedStart.StartSeconds > 600.0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ShipBench] generated start timed out (opened=%d)"), GGeneratedStart.bOpened ? 1 : 0);
			GGeneratedStart.bActive = false;
			return false;
		}
		AMainMenuController* Controller = FindMenuController();
		UWorldGenerationViewModel* ViewModel = Controller ? Controller->GetWorldGenerationViewModel() : nullptr;
		if (!ViewModel)
		{
			return true;
		}
		if (!GGeneratedStart.bOpened)
		{
			// Fails until the Slate menu is installed; the ticker simply retries.
			if (Controller->OpenAstronomicalGenerationForAutomation(EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Civilization))
			{
				GGeneratedStart.bOpened = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: Civilization generator opened on the home planet"));
			}
			return true;
		}
		if (!ViewModel->bPreviewReady || !ViewModel->SpawnParameters)
		{
			GGeneratedStart.ReadySeconds = 0.0;
			return true;
		}
		if (GGeneratedStart.ReadySeconds <= 0.0)
		{
			GGeneratedStart.ReadySeconds = Now;
		}
		// A player spends a few seconds on the page; the preview keeps settling meanwhile.
		if (Now - GGeneratedStart.ReadySeconds < 5.0)
		{
			return true;
		}
		if (GGeneratedStart.Moons >= 0 && !GGeneratedStart.bMoonsApplied)
		{
			// Lunar starts need moons: set them, then wait for the regenerated preview before choosing the start.
			GGeneratedStart.bMoonsApplied = true;
			ViewModel->SetMoonsAmount(GGeneratedStart.Moons);
			GGeneratedStart.ReadySeconds = 0.0;
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: %d moon(s) around the home planet"), GGeneratedStart.Moons);
			return true;
		}
		const auto LoadBlueprintClass = [](FString ObjectPath, UClass* Base) -> UClass*
		{
			if (!ObjectPath.Contains(TEXT(".")))
			{
				ObjectPath += TEXT(".") + FPackageName::GetShortName(ObjectPath) + TEXT("_C");
			}
			UClass* Class = LoadClass<UObject>(nullptr, *ObjectPath);
			if (!Class || !Class->IsChildOf(Base))
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.ShipBench] generated start: class %s not found or not a %s, keeping the menu's"),
					*ObjectPath, *Base->GetName());
				return nullptr;
			}
			return Class;
		};
		if (!GGeneratedStart.ShipClassPath.IsEmpty() && GGeneratedStart.ShipClassPath != TEXT("-"))
		{
			ViewModel->SetSpawnClass(EAPSStartAssetSlot::Spaceship,
				LoadBlueprintClass(GGeneratedStart.ShipClassPath, ASpaceship::StaticClass()));
		}
		if (!GGeneratedStart.CharacterClassPath.IsEmpty())
		{
			ViewModel->SetSpawnClass(EAPSStartAssetSlot::Character,
				LoadBlueprintClass(GGeneratedStart.CharacterClassPath, APawn::StaticClass()));
		}
		ViewModel->SetCharacterSpawnPlace(GGeneratedStart.SpawnPlace);
		if (GGeneratedStart.Fleet > 0)
		{
			// As the menu's TOTAL FLEET SHIPS control sets it.
			ViewModel->SpawnParameters->StartingFleetSize = FMath::Clamp(GGeneratedStart.Fleet, 1, USpawnParameters::MaxStartingFleetSize);
		}
		const USpawnParameters* Spawn = ViewModel->SpawnParameters;
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: committing spawnPlace=%d ship=%s character=%s fleet=%d"),
			static_cast<int32>(Spawn->CharacterSpawnPlace), *GetNameSafe(Spawn->BP_HomeSpaceship.Get()),
			*GetNameSafe(Spawn->BP_CharacterClass.Get()), Spawn->StartingFleetSize);
		GGeneratedStart.bActive = false;
		ViewModel->CommitAndOpenLevel(TEXT("L_WorldGeneration"));
		return false;
	}

	void StartGenerated(const TArray<FString>& Args, UWorld*)
	{
		if (GGeneratedStart.bActive)
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GGeneratedStart.Ticker);
		}
		GGeneratedStart = FGeneratedStart();
		GGeneratedStart.bActive = true;
		GGeneratedStart.SpawnPlace = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		GGeneratedStart.ShipClassPath = Args.Num() > 1 ? Args[1] : FString();
		GGeneratedStart.CharacterClassPath = Args.Num() > 2 && Args[2] != TEXT("-") ? Args[2] : FString();
		GGeneratedStart.Moons = Args.Num() > 3 ? FCString::Atoi(*Args[3]) : -1;
		GGeneratedStart.Fleet = Args.Num() > 4 ? FCString::Atoi(*Args[4]) : -1;
		GGeneratedStart.StartSeconds = FPlatformTime::Seconds();
		GGeneratedStart.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickGeneratedStart), 0.5f);
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start armed: spawnPlace=%d ship=%s character=%s"),
			GGeneratedStart.SpawnPlace,
			GGeneratedStart.ShipClassPath.IsEmpty() || GGeneratedStart.ShipClassPath == TEXT("-")
				? TEXT("menu default") : *GGeneratedStart.ShipClassPath,
			GGeneratedStart.CharacterClassPath.IsEmpty() ? TEXT("menu default") : *GGeneratedStart.CharacterClassPath);
	}

	FAutoConsoleCommandWithWorldAndArgs StartGeneratedCommand(
		TEXT("aps.Ship.StartGenerated"),
		TEXT("aps.Ship.StartGenerated [SpawnPlace=0 orbit] [ShipBlueprintPath|-] [CharacterBlueprintPath|-] [Moons|-1] [FleetShips]: from the main menu, ")
		TEXT("opens the Civilization generator on the home planet and starts L_WorldGeneration like the menu's Start ")
		TEXT("(optionally with another home ship or pilot)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartGenerated));
#endif

	/** Unattended sequence for -game runs: wait for the pawn and streaming, board, benchmark, optionally quit. */
	struct FAutoRun
	{
		bool bActive{false};
		FString Filter;
		TArray<FString> BenchmarkArgs;
		bool bQuit{false};
		double WarmupSeconds{15.0};
		double StartSeconds{0.0};
		double PawnSeenSeconds{0.0};
		double BoardedSeconds{0.0};
		bool bBoarded{false};
		bool bBenchmarkStarted{false};
		/** exp: after boarding, swap into an experimental ship (aps.ExpShip.Spawn). */
		bool bSpawnExperimental{false};
		bool bExperimentalSpawned{false};
		/** drive=Band/Forward/Boost: aps.ExpShip.Drive arguments after the swap. */
		FString DriveArgs;
		/** hold=Seconds: fly (or idle) this long instead of running the benchmark. */
		double HoldSeconds{0.0};
		/** map=Substring: wait for a pawn in that map (a menu start travels to L_WorldGeneration first). */
		FString Map;
		/** free: board only free-standing ships, not display copies carried by stations as child actors. */
		bool bSkipChildActors{false};
		/** shipdrive=Power/Forward/Boost (+ aim=, pitch=, minalt=): aps.Ship.Drive in the boarded ship itself. */
		FString ShipDriveArgs;
		bool bShipDriveStarted{false};
		FTSTicker::FDelegateHandle Ticker;
	};
	FAutoRun GAutoRun;

	void EndAutoRun(const TCHAR* Reason)
	{
		StopDrive(Reason);
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] auto run ended: %s"), Reason);
		const bool bQuit = GAutoRun.bQuit;
		GAutoRun.bActive = false;
		if (bQuit)
		{
			FPlatformMisc::RequestExit(false, TEXT("aps.Ship.AutoBenchmark"));
		}
	}

	bool TickAutoRun(float)
	{
		if (!GAutoRun.bActive)
		{
			return false;
		}
		const double Now = FPlatformTime::Seconds();
		if (Now - GAutoRun.StartSeconds > 900.0)
		{
			EndAutoRun(TEXT("timeout after 15 min"));
			return false;
		}
		APawn* Pawn = FindPlayerPawn(nullptr);
		if (!Pawn || (!GAutoRun.Map.IsEmpty() && !Pawn->GetWorld()->GetMapName().Contains(GAutoRun.Map)))
		{
			return true;
		}
		if (GAutoRun.PawnSeenSeconds <= 0.0)
		{
			GAutoRun.PawnSeenSeconds = Now;
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] auto run: player pawn %s, warming up %.0f s"), *Pawn->GetName(),
				GAutoRun.WarmupSeconds);
		}
		if (!GAutoRun.bBoarded)
		{
			if (Now - GAutoRun.PawnSeenSeconds < GAutoRun.WarmupSeconds)
			{
				return true;
			}
			if (!BoardShip(Pawn, GAutoRun.Filter, GAutoRun.bSkipChildActors))
			{
				EndAutoRun(TEXT("boarding failed"));
				return false;
			}
			GAutoRun.bBoarded = true;
			GAutoRun.BoardedSeconds = Now;
			return true;
		}
		if (GAutoRun.bSpawnExperimental && !GAutoRun.bExperimentalSpawned)
		{
			if (Now - GAutoRun.BoardedSeconds < 2.0)
			{
				return true;
			}
			GEngine->Exec(Pawn->GetWorld(), TEXT("aps.ExpShip.Spawn"));
			if (!GAutoRun.DriveArgs.IsEmpty())
			{
				GEngine->Exec(Pawn->GetWorld(), *(TEXT("aps.ExpShip.Drive ") + GAutoRun.DriveArgs.Replace(TEXT("/"), TEXT(" "))));
			}
			GAutoRun.bExperimentalSpawned = true;
			GAutoRun.BoardedSeconds = Now;
			return true;
		}
		if (!GAutoRun.ShipDriveArgs.IsEmpty())
		{
			if (!GAutoRun.bShipDriveStarted)
			{
				if (Now - GAutoRun.BoardedSeconds < 3.0)
				{
					return true;
				}
				GEngine->Exec(Pawn->GetWorld(), *(TEXT("aps.Ship.Drive ") + GAutoRun.ShipDriveArgs));
				if (!GDrive.bActive)
				{
					EndAutoRun(TEXT("drive did not start"));
					return false;
				}
				GAutoRun.bShipDriveStarted = true;
				GAutoRun.BoardedSeconds = Now;
				return true;
			}
			if (!GDrive.bActive)
			{
				EndAutoRun(TEXT("drive finished"));
				return false;
			}
		}
		if (GAutoRun.HoldSeconds > 0.0)
		{
			if (Now - GAutoRun.BoardedSeconds < GAutoRun.HoldSeconds)
			{
				return true;
			}
			EndAutoRun(TEXT("hold finished"));
			return false;
		}
		if (!GAutoRun.bBenchmarkStarted)
		{
			if (Now - GAutoRun.BoardedSeconds < 3.0)
			{
				return true;
			}
			Start(GAutoRun.BenchmarkArgs, Pawn->GetWorld());
			if (!GRun.bActive)
			{
				EndAutoRun(TEXT("benchmark did not start"));
				return false;
			}
			GAutoRun.bBenchmarkStarted = true;
			return true;
		}
		if (!GRun.bActive)
		{
			EndAutoRun(TEXT("benchmark finished"));
			return false;
		}
		return true;
	}

	void AutoBenchmark(const TArray<FString>& Args, UWorld*)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.ShipBench] aps.Ship.AutoBenchmark <ShipNameFilter> [Altitude] [SecondsPerStep] [Speeds a/b/c] [Sweep] [quit] [warmup=15]"));
			return;
		}
		if (GAutoRun.bActive)
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GAutoRun.Ticker);
		}
		GAutoRun = FAutoRun();
		GAutoRun.bActive = true;
		GAutoRun.Filter = Args[0];
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			if (Args[Index].Equals(TEXT("quit"), ESearchCase::IgnoreCase))
			{
				GAutoRun.bQuit = true;
			}
			else if (Args[Index].StartsWith(TEXT("warmup="), ESearchCase::IgnoreCase))
			{
				GAutoRun.WarmupSeconds = FMath::Clamp(FCString::Atod(*Args[Index].RightChop(7)), 0.0, 300.0);
			}
			else if (Args[Index].Equals(TEXT("exp"), ESearchCase::IgnoreCase))
			{
				GAutoRun.bSpawnExperimental = true;
			}
			else if (Args[Index].StartsWith(TEXT("drive="), ESearchCase::IgnoreCase))
			{
				GAutoRun.DriveArgs = Args[Index].RightChop(6);
			}
			else if (Args[Index].StartsWith(TEXT("hold="), ESearchCase::IgnoreCase))
			{
				GAutoRun.HoldSeconds = FMath::Clamp(FCString::Atod(*Args[Index].RightChop(5)), 0.0, 600.0);
			}
			else if (Args[Index].StartsWith(TEXT("map="), ESearchCase::IgnoreCase))
			{
				GAutoRun.Map = Args[Index].RightChop(4);
			}
			else if (Args[Index].Equals(TEXT("free"), ESearchCase::IgnoreCase))
			{
				GAutoRun.bSkipChildActors = true;
			}
			else if (Args[Index].StartsWith(TEXT("shipdrive="), ESearchCase::IgnoreCase))
			{
				GAutoRun.ShipDriveArgs = Args[Index].RightChop(10).Replace(TEXT("/"), TEXT(" ")) + GAutoRun.ShipDriveArgs;
			}
			else if (Args[Index].StartsWith(TEXT("aim="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("pitch="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("minalt="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("startly="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("yaw="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("engine="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("shots="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("burst="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("shotlabel="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("shotui="), ESearchCase::IgnoreCase))
			{
				GAutoRun.ShipDriveArgs += TEXT(" ") + Args[Index];
			}
			else
			{
				GAutoRun.BenchmarkArgs.Add(Args[Index]);
			}
		}
		GAutoRun.StartSeconds = FPlatformTime::Seconds();
		GAutoRun.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickAutoRun), 0.5f);
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] auto run armed: ship '%s' args=%d quit=%d warmup=%.0f s map='%s' free=%d shipdrive='%s' hold=%.0f s"),
			*GAutoRun.Filter, GAutoRun.BenchmarkArgs.Num(), GAutoRun.bQuit ? 1 : 0, GAutoRun.WarmupSeconds, *GAutoRun.Map,
			GAutoRun.bSkipChildActors ? 1 : 0, *GAutoRun.ShipDriveArgs, GAutoRun.HoldSeconds);
	}

	void Report(UWorld* World)
	{
		if (const ASpaceship* Ship = FindPilotedShip(World))
		{
			FAPSShipFlightBenchmark::LogShipReport(*Ship);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ShipReport] pilot a ship first."));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs BenchmarkCommand(
		TEXT("aps.Ship.Benchmark"),
		TEXT("aps.Ship.Benchmark [AltitudeMeters=1000] [SecondsPerStep=8] [SpeedsMps=0,100,400,1600,6400,25600] [Sweep=1]: ")
		TEXT("flies the piloted ship around the nearest planet at each speed and logs frame/game/render/GPU time."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Start));

	FAutoConsoleCommandWithWorldAndArgs BoardCommand(
		TEXT("aps.Ship.Board"),
		TEXT("aps.Ship.Board [NameFilter]: the player's pawn takes the nearest free ship whose actor or class name contains the filter."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Board));

	FAutoConsoleCommandWithWorldAndArgs AutoBenchmarkCommand(
		TEXT("aps.Ship.AutoBenchmark"),
		TEXT("aps.Ship.AutoBenchmark <ShipNameFilter> [Altitude] [SecondsPerStep] [Speeds a/b/c] [Sweep] [quit] [warmup=15]: ")
		TEXT("waits for the player pawn, boards the ship, runs aps.Ship.Benchmark and optionally quits (for -game runs)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AutoBenchmark));

	FAutoConsoleCommandWithWorld StopCommand(
		TEXT("aps.Ship.BenchmarkStop"),
		TEXT("Stops aps.Ship.Benchmark and writes its CSV."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Stop));

	FAutoConsoleCommandWithWorldAndArgs DriveCommand(
		TEXT("aps.Ship.Drive"),
		TEXT("aps.Ship.Drive <Power 1-6> [Forward=1] [Boost=0] [aim=keep|planet|horizon|up] [pitch=Deg] [minalt=Km]: holds the ")
		TEXT("piloted ship's power step, thrust and boost like a pilot and logs speed, altitude and frame/game/render/GPU time every second."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartDrive));

	FAutoConsoleCommandWithWorld DriveStopCommand(
		TEXT("aps.Ship.DriveStop"),
		TEXT("Releases the controls held by aps.Ship.Drive and logs the drive summary."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&StopDriveCommand));

	FAutoConsoleCommandWithWorld ReportCommand(
		TEXT("aps.Ship.Report"),
		TEXT("Logs the piloted ship's moving primitives, collision/overlap flags, body shapes and attached lights."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Report));
}

bool FAPSShipFlightBenchmark::IsRunning()
{
	return APSShipBenchmark::GRun.bActive;
}

FQuat FAPSShipFlightBenchmark::GetRotationForFlightAxes(const ASpaceship& Ship, const FVector& Forward, const FVector& Up)
{
	const FQuat LocalFrame = FRotationMatrix::MakeFromXZ(Ship.FlightForwardLocalAxis, Ship.FlightUpLocalAxis).ToQuat();
	const FQuat WorldFrame = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
	return (WorldFrame * LocalFrame.Inverse()).GetNormalized();
}

void FAPSShipFlightBenchmark::SetBenchmarkKinematic(ASpaceship& Ship, bool bKinematic, bool& bInOutHadPhysicalImpulse)
{
	if (bKinematic)
	{
		bInOutHadPhysicalImpulse = Ship.ActiveClassPreset.bUsesPhysicalImpulse;
		Ship.ActiveClassPreset.bUsesPhysicalImpulse = false;
		if (Ship.SpaceshipHull && Ship.SpaceshipHull->IsSimulatingPhysics())
		{
			Ship.SpaceshipHull->SetSimulatePhysics(false);
		}
		return;
	}
	if (bInOutHadPhysicalImpulse)
	{
		Ship.ActiveClassPreset.bUsesPhysicalImpulse = true;
		Ship.ApplyEngineState();
	}
}

void FAPSShipFlightBenchmark::SetKinematicVelocity(ASpaceship& Ship, const FVector& Velocity)
{
	Ship.KinematicVelocity = Velocity;
	Ship.CurrentAngularVelocityDegrees = FVector::ZeroVector;
}

bool FAPSShipFlightBenchmark::TickShip(ASpaceship& Ship, float DeltaTime)
{
	using namespace APSShipBenchmark;
	if (GDrive.bActive && GDrive.Ship.Get() == &Ship)
	{
		// The ship's own flight code moves it with the held controls.
		TickDrive(Ship, DeltaTime);
		return false;
	}
	if (!GRun.bActive || GRun.Ship.Get() != &Ship)
	{
		return false;
	}
	APlanetaryBody* Planet = GRun.Planet.Get();
	if (!IsValid(Planet) || !IsValid(Ship.Pilot) || GRun.OrbitRadiusCm <= UE_DOUBLE_SMALL_NUMBER)
	{
		Finish(TEXT("planet or pilot lost"));
		return false;
	}

	// The engine publishes the previous frame's thread times; the first frame of a step still belongs to the
	// previous speed and is skipped.
	if (GRun.StepElapsed > 0.0)
	{
		FStep& Step = GRun.Steps[GRun.StepIndex];
		Step.FrameMs.Add(DeltaTime * 1000.0f);
		const uint32 GameCycles = GGameThreadTime;
		const uint32 RenderCycles = GRenderThreadTime;
		const uint32 GpuCycles = GGPUFrameTime;
		if (GameCycles)
		{
			Step.GameMsSum += FPlatformTime::ToMilliseconds(GameCycles);
			++Step.GameSamples;
		}
		if (RenderCycles)
		{
			Step.RenderMsSum += FPlatformTime::ToMilliseconds(RenderCycles);
			++Step.RenderSamples;
		}
		if (GpuCycles)
		{
			Step.GpuMsSum += FPlatformTime::ToMilliseconds(GpuCycles);
			++Step.GpuSamples;
		}
		const int32 Lods = LodsInGeneration(GRun.Terrain.Get());
		Step.LodsInGenerationSum += FMath::Max(Lods, 0);
		Step.LodsInGenerationMax = FMath::Max(Step.LodsInGenerationMax, Lods);
	}

	GRun.StepElapsed += DeltaTime;
	if (GRun.StepElapsed >= GRun.SecondsPerStep)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] step %d/%d %s"), GRun.StepIndex + 1, GRun.Steps.Num(),
			*DescribeStep(GRun.Steps[GRun.StepIndex]));
		EndStepRegion();
		++GRun.StepIndex;
		GRun.StepElapsed = 0.0;
		if (!GRun.Steps.IsValidIndex(GRun.StepIndex))
		{
			Finish(TEXT("completed"));
			return false;
		}
		BeginStepRegion();
	}

	FStep& Step = GRun.Steps[GRun.StepIndex];
	const double SpeedCm = Step.SpeedMps * 100.0;
	GRun.AngleRad += SpeedCm * DeltaTime / GRun.OrbitRadiusCm;
	const double Cos = FMath::Cos(GRun.AngleRad);
	const double Sin = FMath::Sin(GRun.AngleRad);
	const FVector Radial = GRun.AxisU * Cos + GRun.AxisV * Sin;
	const FVector Tangent = GRun.AxisV * Cos - GRun.AxisU * Sin;
	const FVector Target = Planet->GetActorLocation() + Radial * GRun.OrbitRadiusCm;
	const FVector Before = Ship.GetActorLocation();

	const double MoveStart = FPlatformTime::Seconds();
	Ship.SetActorRotation(GetRotationForFlightAxes(Ship, Tangent, Radial), ETeleportType::None);
	FHitResult Hit;
	Ship.MoveShipKinematic(Target - Before, GRun.bSweep, Hit);
	if (Hit.bBlockingHit)
	{
		// Terrain above the base sphere: keep the path so every step covers the same ground.
		++Step.BlockedMoves;
		Ship.SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
	}
	const double MoveMs = (FPlatformTime::Seconds() - MoveStart) * 1000.0;
	if (GRun.StepElapsed > DeltaTime)
	{
		Step.MoveMsSum += MoveMs;
		Step.MoveMsMax = FMath::Max(Step.MoveMsMax, MoveMs);
		++Step.MoveSamples;
	}
	Step.DistanceCm += FVector::Distance(Before, Ship.GetActorLocation());
	// Camera, HUD and speed readouts see the benchmark speed like a real flight.
	SetKinematicVelocity(Ship, Tangent * SpeedCm);
	return true;
}

void FAPSShipFlightBenchmark::SetPilotControls(ASpaceship& Ship, int32 Power, float Forward, bool bBoost, bool bBrake, float Yaw)
{
	if (Ship.FlightModel && Ship.FlightModel->IsBandFlightActive())
	{
		// Band model: Power is the band key 1-5, or 0 for AUTO (the ship picks the band itself).
		if (Power <= 0)
		{
			if (!Ship.FlightModel->IsAutoBandActive())
			{
				Ship.FlightModel->SetAutoBands();
			}
		}
		else
		{
			const EAPSFlightBand Band = static_cast<EAPSFlightBand>(FMath::Clamp(Power, 1, 5) - 1);
			if (Ship.FlightModel->GetFlightBand() != Band || Ship.FlightModel->IsAutoBandActive())
			{
				Ship.FlightModel->SetFlightBand(Band);
			}
		}
	}
	else
	{
		const EShipDriveMode DriveMode = static_cast<EShipDriveMode>(FMath::Clamp(Power, 1, 6) - 1);
		if (Ship.SelectedDriveMode != DriveMode)
		{
			Ship.SetDriveMode(DriveMode, false);
		}
	}
	Ship.ForwardInput = Forward;
	Ship.bIsAccelerating = bBoost;
	Ship.bIsDecelerating = bBrake;
	Ship.YawInput = Yaw;
}

void FAPSShipFlightBenchmark::SelectEngine(ASpaceship& Ship, int32 Engine)
{
	switch (Engine)
	{
	case 1: Ship.SetEngineMode(EEngineMode::Impulse, false); break;
	case 2: Ship.SetEngineMode(EEngineMode::SpaceWrap, false); break;
	case 3: Ship.SetEngineMode(EEngineMode::Offset, false); break;
	default: break;
	}
}

void FAPSShipFlightBenchmark::LogShipReport(const ASpaceship& Ship)
{
	TArray<AActor*> Actors{const_cast<ASpaceship*>(&Ship)};
	Ship.GetAttachedActors(Actors, false, true);

	struct FPrimitiveLine
	{
		FString Name;
		int32 Shapes{0};
		bool bQuery{false};
		bool bPhysics{false};
		bool bOverlaps{false};
		bool bComplexAsSimple{false};
	};
	TArray<FPrimitiveLine> Lines;
	int32 Primitives = 0;
	int32 Visible = 0;
	int32 Query = 0;
	int32 Physics = 0;
	int32 Overlaps = 0;
	int32 Simulating = 0;
	int32 Shapes = 0;
	int32 ComplexAsSimple = 0;
	int32 ShadowCasters = 0;
	int32 NaniteMeshes = 0;
	int32 Lights = 0;
	int32 VisibleLights = 0;
	int32 ShadowLights = 0;
	double MaxLightRadius = 0.0;
	for (const AActor* Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		TArray<UPrimitiveComponent*> Components;
		Actor->GetComponents(Components);
		for (const UPrimitiveComponent* Component : Components)
		{
			if (!IsValid(Component) || !Component->IsRegistered())
			{
				continue;
			}
			++Primitives;
			Visible += Component->IsVisible() ? 1 : 0;
			Simulating += Component->IsSimulatingPhysics() ? 1 : 0;
			ShadowCasters += Component->IsVisible() && Component->CastShadow ? 1 : 0;
			FPrimitiveLine Line;
			Line.Name = FString::Printf(TEXT("%s.%s"), *Actor->GetName(), *Component->GetName());
			Line.bQuery = Component->IsQueryCollisionEnabled();
			Line.bPhysics = Component->IsPhysicsCollisionEnabled();
			Line.bOverlaps = Component->GetGenerateOverlapEvents();
			if (const UBodySetup* BodySetup = const_cast<UPrimitiveComponent*>(Component)->GetBodySetup();
				BodySetup && Component->IsCollisionEnabled())
			{
				Line.Shapes = BodySetup->AggGeom.GetElementCount();
				Line.bComplexAsSimple = BodySetup->CollisionTraceFlag == CTF_UseComplexAsSimple;
			}
			if (const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
				Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->IsNaniteEnabled())
			{
				++NaniteMeshes;
			}
			Query += Line.bQuery ? 1 : 0;
			Physics += Line.bPhysics ? 1 : 0;
			Overlaps += Line.bOverlaps ? 1 : 0;
			Shapes += Line.Shapes;
			ComplexAsSimple += Line.bComplexAsSimple ? 1 : 0;
			Lines.Add(MoveTemp(Line));
		}
		TArray<ULocalLightComponent*> LightComponents;
		Actor->GetComponents(LightComponents);
		for (const ULocalLightComponent* Light : LightComponents)
		{
			if (!IsValid(Light))
			{
				continue;
			}
			++Lights;
			if (Light->IsVisible())
			{
				++VisibleLights;
				ShadowLights += Light->CastShadows ? 1 : 0;
				MaxLightRadius = FMath::Max(MaxLightRadius, static_cast<double>(Light->AttenuationRadius));
			}
		}
	}

	const UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Ship.GetRootComponent());
	UE_LOG(LogTemp, Log,
		TEXT("[APS.ShipReport] ship=%s class=%s size=%s engine=%s env=%s physicalImpulse=%d simulating=%d hullConvex=%d ")
		TEXT("generatedHull=%d flightProxy=%d proxyBoxes=%d rootQuery=%d rootRadius=%.0f m"),
		*Ship.GetName(), *Ship.GetClass()->GetName(), *Ship.GetSizeClassName(), *Ship.GetEngineModeName(),
		*Ship.GetFlightEnvironmentName(), Ship.ActiveClassPreset.bUsesPhysicalImpulse ? 1 : 0,
		Ship.SpaceshipHull && Ship.SpaceshipHull->IsSimulatingPhysics() ? 1 : 0,
		Ship.HullHasFittedConvexCollision() ? 1 : 0, Ship.bGenerateSimpleHullCollision ? 1 : 0,
		Ship.bFlightCollisionOptimizationActive ? 1 : 0, Ship.GetGeneratedCollisionCount(),
		Root && Root->IsQueryCollisionEnabled() ? 1 : 0, Root ? Root->Bounds.SphereRadius / 100.0 : 0.0);
	UE_LOG(LogTemp, Log,
		TEXT("[APS.ShipReport] actors=%d primitives=%d visible=%d nanite=%d shadowCasters=%d query=%d physics=%d ")
		TEXT("overlapEvents=%d simulating=%d bodyShapes=%d complexAsSimple=%d | lights=%d visible=%d shadowed=%d maxRadius=%.0f m"),
		Actors.Num(), Primitives, Visible, NaniteMeshes, ShadowCasters, Query, Physics, Overlaps, Simulating, Shapes,
		ComplexAsSimple, Lights, VisibleLights, ShadowLights, MaxLightRadius / 100.0);

	Lines.Sort([](const FPrimitiveLine& Left, const FPrimitiveLine& Right)
	{
		return Left.Shapes > Right.Shapes;
	});
	for (int32 Index = 0; Index < FMath::Min(Lines.Num(), 12); ++Index)
	{
		const FPrimitiveLine& Line = Lines[Index];
		if (Line.Shapes == 0 && !Line.bOverlaps && !Line.bQuery)
		{
			break;
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipReport]   %s shapes=%d query=%d physics=%d overlaps=%d complexAsSimple=%d"),
			*Line.Name, Line.Shapes, Line.bQuery ? 1 : 0, Line.bPhysics ? 1 : 0, Line.bOverlaps ? 1 : 0,
			Line.bComplexAsSimple ? 1 : 0);
	}
}
