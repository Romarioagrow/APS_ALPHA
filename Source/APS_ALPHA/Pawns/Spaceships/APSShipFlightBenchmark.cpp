#include "APSShipFlightBenchmark.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Characters/APSSpeedModeCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Spaceship.h"
#include "APSShipFlightModel.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Gameplay/Construction/APSConstructionMode.h"
#include "APS_ALPHA/Gameplay/Construction/APSShipBuildComponent.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyGpuStars.h"
#include "APS_ALPHA/Core/Rendering/APSStellarViewOptics.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Camera/CameraComponent.h"
#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "Components/LocalLightComponent.h"
#include "Engine/TargetPoint.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
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
// Rio 06.10 (packaged build): aps.Test.DumpHome walks AAstroGenerator in every configuration, so its header is not test-only.
#include "APS_ALPHA/Generation/AstroGenerator.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
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
		/** aim=next or aim=moon: the body flown to, logged every second with its surface readiness. */
		TWeakObjectPtr<APlanetaryBody> Target;
		/** The nearest matching body is chosen once, on the first aim; later aims keep it. */
		bool bTargetChosen{false};
		/** aim=star: the nearest catalogue star of the cluster other than the home one (stars at speed, 02.10). */
		bool bHasStar{false};
		FVector StarLocation{FVector::ZeroVector};
		int32 StarIndex{INDEX_NONE};
		/** jump=Km: start this far above the target's surface instead of halfway (moon landings, exit tests). */
		double JumpKm{0.0};
		/** exit=1: at the minimum altitude the pilot leaves the ship (gravity and orientation after an exit, 01.10). */
		bool bExitAtEnd{false};
		/** walk=1 (Rio 05.10, "the pilot walking aboard at speed shakes and falls through the floor"): once the pilot gets
		 * up (aps.Test.Key F) the ship flies on with the held controls (the flight model's test drive) and the run keeps
		 * its shots and logs; [APS.ShipWalk] logs the walker's place on the deck every second (a fall shows as a drop). */
		bool bWalk{false};
		/** walk=2: the walker also walks the deck, two seconds forward and two back, again and again. */
		bool bWalkAround{false};
		bool bWalking{false};
		double WalkSince{0.0};
		bool bWalkSampled{false};
		FVector WalkOffset{FVector::ZeroVector};
		double WalkStepMax{0.0};
		double WalkUpMin{TNumericLimits<double>::Max()};
		double WalkUpMax{-TNumericLimits<double>::Max()};
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

	/** exit=1: the pilot's own view a few seconds after leaving the ship. */
	struct FExitShots
	{
		double ExitSeconds{-1.0};
		FString Label;
		int32 Taken{0};
	};
	FExitShots GExitShots;
	bool LookAtNearestShip(UWorld* World, APawn* Pawn, bool bLog);

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
			if (GDrive.bWalking && Ship->FlightModel)
			{
				Ship->FlightModel->SetDebugDrive(false, 0.0f, false);
			}
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
		else if (GDrive.Aim.Equals(TEXT("next"), ESearchCase::IgnoreCase) || GDrive.Aim.Equals(TEXT("moon"), ESearchCase::IgnoreCase))
		{
			// The nearest other planet (or moon) with a streamed surface: an approach from afar (arrival forecast) or a
			// landing on a moon (gravity after an exit, 01.10).
			// The default menu world has one planet and no moons: start with aps.Ship.StartGenerated's Moons argument.
			const bool bMoon = GDrive.Aim.Equals(TEXT("moon"), ESearchCase::IgnoreCase);
			double Best = TNumericLimits<double>::Max();
			for (TActorIterator<APlanetaryBody> It(Ship.GetWorld()); It && !GDrive.bTargetChosen; ++It)
			{
				const double Distance = FVector::Dist(Ship.GetActorLocation(), It->GetActorLocation());
				if (IsValid(*It) && *It != Planet && It->bStreamWorldScapeSurface && It->IsA<AMoon>() == bMoon
					&& Distance < Best)
				{
					Best = Distance;
					GDrive.Target = *It;
				}
			}
			if (!GDrive.bTargetChosen)
			{
				GDrive.bTargetChosen = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] target for aim=%s: %s"), *GDrive.Aim,
					GDrive.Target.IsValid() ? *GDrive.Target->GetName() : TEXT("none (no such body in this world)"));
			}
			if (const APlanetaryBody* Target = GDrive.Target.Get())
			{
				Forward = (Target->GetActorLocation() - Ship.GetActorLocation()).GetSafeNormal();
			}
		}
		else if (GDrive.Aim.Equals(TEXT("star"), ESearchCase::IgnoreCase))
		{
			if (!GDrive.bTargetChosen)
			{
				GDrive.bTargetChosen = true;
				double Best = TNumericLimits<double>::Max();
				for (TActorIterator<AStarCluster> It(Ship.GetWorld()); It; ++It)
				{
					for (const FClusterStarSystemRecord& Record : It->PotentialStarSystems)
					{
						if (Record.InstanceIndex == INDEX_NONE || Record.bMaterialized || Record.MaterializedSystem.IsValid()) continue;
						const FVector Location = It->GetPotentialSystemWorldLocation(Record);
						const double Distance = FVector::Dist(Location, Ship.GetActorLocation());
						if (Distance > 1.0e12 && Distance < Best)
						{
							Best = Distance;
							GDrive.bHasStar = true;
							GDrive.StarLocation = Location;
							GDrive.StarIndex = Record.InstanceIndex;
						}
					}
				}
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] target for aim=star: %s"), GDrive.bHasStar
					? *FString::Printf(TEXT("cluster star %d, %.2f AU away"), GDrive.StarIndex, Best / 1.495978707e13)
					: TEXT("none (no cluster in this world)"));
			}
			if (GDrive.bHasStar)
			{
				Forward = (GDrive.StarLocation - Ship.GetActorLocation()).GetSafeNormal();
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
		const APlayerController* Player = Ship.GetWorld() ? Ship.GetWorld()->GetFirstPlayerController() : nullptr;
		APawn* Walker = Player ? Player->GetPawn() : nullptr;
		const bool bAboard = Walker && Walker->GetAttachParentActor() == &Ship;
		if (!Ship.HasPilot())
		{
			if (!GDrive.bWalk)
			{
				StopDrive(TEXT("pilot left the ship"));
				return;
			}
			if (!GDrive.bWalking)
			{
				GDrive.bWalking = true;
				GDrive.WalkSince = GDrive.Elapsed;
				if (Ship.FlightModel)
				{
					Ship.FlightModel->SetDebugDrive(true, GDrive.Forward, GDrive.bBoost);
				}
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] the pilot got up (%s): the ship flies on with the held controls"),
					*GetNameSafe(Walker));
			}
			// Getting up takes a few frames (a new pawn, then it settles on the deck).
			if (!bAboard && GDrive.Elapsed - GDrive.WalkSince > 3.0)
			{
				StopDrive(TEXT("the pilot got up but is not aboard 3 s later"));
				return;
			}
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
			if (GDrive.Aim.Equals(TEXT("next"), ESearchCase::IgnoreCase) || GDrive.Aim.Equals(TEXT("moon"), ESearchCase::IgnoreCase))
			{
				// Arrival tests start halfway to the target in open space, out of the home planet's gravity well;
				// jump=Km starts that far above the target's surface, on the side facing the ship.
				AimShip(Ship);
				if (APlanetaryBody* Target = GDrive.Target.Get())
				{
					const FVector Away = (Ship.GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
					const FVector Start = GDrive.JumpKm > 0.0
						? Target->GetActorLocation() + Away * (Target->GetWorldScapeBodyRadiusCm() + GDrive.JumpKm * 1.0e5)
						: FMath::Lerp(Ship.GetActorLocation(), Target->GetActorLocation(), 0.5);
					if (GDrive.JumpKm > 0.0)
					{
						// Altitude and the minimum altitude now refer to the target.
						GDrive.Planet = Target;
					}
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
		if (!GDrive.bWalking)
		{
			FAPSShipFlightBenchmark::SetPilotControls(Ship, GDrive.Power, GDrive.Forward, GDrive.bBoost, false, GDrive.Yaw);
		}
		else if (bAboard)
		{
			if (GDrive.bWalkAround)
			{
				const bool bForward = FMath::Fmod(GDrive.Elapsed - GDrive.WalkSince, 4.0) < 2.0;
				Walker->AddMovementInput(Walker->GetActorForwardVector(), bForward ? 1.0f : -1.0f);
			}
			// The walker in the ship's own frame: standing still it should not move at all, whatever the speed.
			const FVector Offset = Ship.GetActorTransform().InverseTransformPosition(Walker->GetActorLocation());
			if (GDrive.bWalkSampled)
			{
				GDrive.WalkStepMax = FMath::Max(GDrive.WalkStepMax, (Offset - GDrive.WalkOffset).Size());
			}
			GDrive.WalkOffset = Offset;
			GDrive.bWalkSampled = true;
			GDrive.WalkUpMin = FMath::Min(GDrive.WalkUpMin, Offset.Z);
			GDrive.WalkUpMax = FMath::Max(GDrive.WalkUpMax, Offset.Z);
		}

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
			if (GDrive.bHasStar)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] t=%3d s star %d: %.4f AU away"), GDrive.Second + 1, GDrive.StarIndex,
					FVector::Dist(Ship.GetActorLocation(), GDrive.StarLocation) / 1.495978707e13);
			}
			if (GDrive.bCameraSampled)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipCam] t=%3d s arm %.1f-%.1f m | step max %.2f cm jerk max %.3f cm turn max %.3f deg"),
					GDrive.Second + 1, GDrive.CameraDistanceMin / 100.0, GDrive.CameraDistanceMax / 100.0,
					GDrive.CameraStepMax, GDrive.CameraJerkMax, GDrive.CameraTurnMax);
			}
			if (GDrive.bWalking && GDrive.bWalkSampled)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipWalk] t=%3d s on deck at %s m | ship-local z %.2f..%.2f m | step max %.3f cm | attached=%d"),
					GDrive.Second + 1, *(GDrive.WalkOffset / 100.0).ToCompactString(), GDrive.WalkUpMin / 100.0,
					GDrive.WalkUpMax / 100.0, GDrive.WalkStepMax, bAboard ? 1 : 0);
				GDrive.WalkStepMax = 0.0;
				GDrive.WalkUpMin = TNumericLimits<double>::Max();
				GDrive.WalkUpMax = -TNumericLimits<double>::Max();
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
			if (GDrive.bExitAtEnd)
			{
				FAPSShipFlightBenchmark::SetPilotControls(Ship, 0, 0.0f, false, false, 0.0f);
				const FString Body = GetNameSafe(GDrive.Planet.Get());
				const bool bLeft = Ship.RequestReleaseVehicleControl();
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] pilot %s the ship %.1f km above %s"),
					bLeft ? TEXT("left") : TEXT("could not leave"), Altitude, *Body);
				GExitShots.ExitSeconds = bLeft ? FPlatformTime::Seconds() : -1.0;
				GExitShots.Label = GDrive.ShotLabel;
				GExitShots.Taken = 0;
			}
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
			else if (Arg.StartsWith(TEXT("jump="), ESearchCase::IgnoreCase))
			{
				Drive.JumpKm = FMath::Clamp(FCString::Atod(*Arg.RightChop(5)), 0.0, 1.0e6);
			}
			else if (Arg.StartsWith(TEXT("exit="), ESearchCase::IgnoreCase))
			{
				Drive.bExitAtEnd = FCString::Atoi(*Arg.RightChop(5)) != 0;
			}
			else if (Arg.StartsWith(TEXT("walk="), ESearchCase::IgnoreCase))
			{
				const int32 Walk = FCString::Atoi(*Arg.RightChop(5));
				Drive.bWalk = Walk != 0;
				Drive.bWalkAround = Walk >= 2;
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
		/** Planets in the home system (-1: the menu's own, one in the default world), for flights between planets. */
		int32 Planets{-1};
		bool bMoonsApplied{false};
		bool bPlanetsApplied{false};
		/** Rio 08.10 night (aps.Ship.StartHomeType): the home planet's surface family was set for this start. */
		bool bHomeTypeApplied{false};
		/** Rio 09.10 (A23, aps.Ship.StartHomeIndex): the start planet index was set, then the new home focused. */
		bool bHomeIndexApplied{false};
		bool bHomeIndexFocused{false};
		/** Rio 05.10 (real scale, stage 2): REAL SCALE was switched on for this start (aps.Galaxy.RealScaleStart). */
		bool bRealScaleApplied{false};
		/** C19 ground start as "package/pad/vehicles" (0-2 / 0-1 / 0-7), empty: the menu's own. */
		FString Ground;
		/** The headquarters Blueprint (C20 HQ Alpha checks), empty: the menu's own. */
		FString HeadquartersClassPath;
		FString ShipClassPath;
		FString CharacterClassPath;
		double StartSeconds{0.0};
		double ReadySeconds{0.0};
		FTSTicker::FDelegateHandle Ticker;
	};
	FGeneratedStart GGeneratedStart;

	TAutoConsoleVariable<int32> CVarRealScaleStart(
		TEXT("aps.Galaxy.RealScaleStart"), 0,
		TEXT("Rio 05.10 (REAL SCALE, stage 2; test runs): 1 makes aps.Ship.StartGenerated switch REAL SCALE on in the menu and ")
		TEXT("wait for the rebuilt preview before it starts the game. 0: the menu's own setting."));

	TAutoConsoleVariable<FString> CVarStartHomeType(
		TEXT("aps.Ship.StartHomeType"), TEXT(""),
		TEXT("Rio 08.10 night (test runs): an EPlanetType name (Oceanic, Terrestrial, Forest, Volcanic, ...) that ")
		TEXT("aps.Ship.StartGenerated sets on the home planet, as the menu's SURFACE FAMILY does, before the start. ")
		TEXT("Empty: the menu's own."));

	TAutoConsoleVariable<int32> CVarStartHomeIndex(
		TEXT("aps.Ship.StartHomeIndex"), 0,
		TEXT("Rio 09.10 (A23, test runs): HOME START PLANET INDEX that aps.Ship.StartGenerated sets in the menu (after the ")
		TEXT("planet count) before the start: the world on that orbit becomes the home as it is. 0: the menu's own."));

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
		if (CVarRealScaleStart.GetValueOnGameThread() != 0 && !GGeneratedStart.bRealScaleApplied)
		{
			// Rio 05.10 (real scale, stage 2): the same world at real distances; the preview rebuilds before anything else.
			GGeneratedStart.bRealScaleApplied = true;
			ViewModel->SetRealScale(true);
			GGeneratedStart.ReadySeconds = 0.0;
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: REAL SCALE on (aps.Galaxy.RealScaleStart), active=%d"),
				ViewModel->IsRealScaleActive() ? 1 : 0);
			return true;
		}
		if (GGeneratedStart.Planets > 0 && !GGeneratedStart.bPlanetsApplied)
		{
			// Inter-planet flights need neighbours: set them, then wait for the regenerated preview.
			GGeneratedStart.bPlanetsApplied = true;
			ViewModel->SetPlanetsAmount(GGeneratedStart.Planets);
			GGeneratedStart.ReadySeconds = 0.0;
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: %d planet(s) in the home system"), GGeneratedStart.Planets);
			return true;
		}
		if (const int32 HomeIndex = CVarStartHomeIndex.GetValueOnGameThread(); HomeIndex > 0 && !GGeneratedStart.bHomeIndexApplied)
		{
			// Rio 09.10 (A23): the world on orbit N becomes the home as it is; the family is logged before and after.
			GGeneratedStart.bHomeIndexApplied = true;
			ViewModel->LogHomeFamily(TEXT("before StartHomeIndex"));
			const int32 OldIndex = ViewModel->GeneratedWorld ? ViewModel->GeneratedWorld->StartPlanetIndex : 0;
			ViewModel->SetStartPlanetIndex(HomeIndex);
			GGeneratedStart.ReadySeconds = 0.0;
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: home start planet %d -> %d (asked %d of %d)"), OldIndex,
				ViewModel->GeneratedWorld ? ViewModel->GeneratedWorld->StartPlanetIndex : 0, HomeIndex,
				ViewModel->GetHomeStartPlanetCount());
			return true;
		}
		if (GGeneratedStart.bHomeIndexApplied && !GGeneratedStart.bHomeIndexFocused)
		{
			GGeneratedStart.bHomeIndexFocused = true;
			if (const AAstroGenerator* Generator = ViewModel->GetPreviewGenerator(); Generator && IsValid(Generator->HomePlanet))
			{
				ViewModel->FocusPreviewBody(Generator->HomePlanet);
			}
			ViewModel->LogHomeFamily(TEXT("after StartHomeIndex"));
			GGeneratedStart.ReadySeconds = 0.0;
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
		if (!GGeneratedStart.bHomeTypeApplied)
		{
			// Rio 08.10 night: the home planet's family (ocean worlds and the like), as the SURFACE FAMILY stepper sets it.
			GGeneratedStart.bHomeTypeApplied = true;
			const FString TypeName = CVarStartHomeType.GetValueOnGameThread().TrimStartAndEnd();
			if (!TypeName.IsEmpty())
			{
				const UEnum* TypeEnum = StaticEnum<EPlanetType>();
				const int64 Value = TypeEnum->GetValueByNameString(
					TypeName.Contains(TEXT("::")) ? TypeName : TEXT("EPlanetType::") + TypeName);
				if (Value != INDEX_NONE)
				{
					ViewModel->SetEnumValue(TypeEnum, static_cast<int32>(Value));
					GGeneratedStart.ReadySeconds = 0.0;
					UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: home planet type %s"), *TypeName);
					return true;
				}
				UE_LOG(LogTemp, Warning, TEXT("[APS.ShipBench] generated start: unknown home planet type %s, keeping the menu's"), *TypeName);
			}
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
		if (!GGeneratedStart.HeadquartersClassPath.IsEmpty())
		{
			ViewModel->SetSpawnClass(EAPSStartAssetSlot::Headquarters,
				LoadBlueprintClass(GGeneratedStart.HeadquartersClassPath, ASpaceHeadquarters::StaticClass()));
		}
		ViewModel->SetCharacterSpawnPlace(GGeneratedStart.SpawnPlace);
		if (GGeneratedStart.Fleet > 0)
		{
			// As the menu's TOTAL FLEET SHIPS control sets it.
			ViewModel->SpawnParameters->StartingFleetSize = FMath::Clamp(GGeneratedStart.Fleet, 1, USpawnParameters::MaxStartingFleetSize);
		}
		if (TArray<FString> Ground; GGeneratedStart.Ground.ParseIntoArray(Ground, TEXT("/")) == 3)
		{
			// As the menu's GROUND step sets them.
			ViewModel->SpawnParameters->ColonyStartPackage = static_cast<EAPSColonyStartPackage>(FMath::Clamp(FCString::Atoi(*Ground[0]), 0, 2));
			ViewModel->SpawnParameters->LaunchPadStart = static_cast<EAPSLaunchPadStart>(FMath::Clamp(FCString::Atoi(*Ground[1]), 0, 1));
			ViewModel->SpawnParameters->GroundVehicleMask = FMath::Clamp(FCString::Atoi(*Ground[2]), 0, 7);
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: ground package=%d pad=%d vehicles=%d"),
				static_cast<int32>(ViewModel->SpawnParameters->ColonyStartPackage),
				static_cast<int32>(ViewModel->SpawnParameters->LaunchPadStart), ViewModel->SpawnParameters->GroundVehicleMask);
		}
		const USpawnParameters* Spawn = ViewModel->SpawnParameters;
		UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] generated start: committing spawnPlace=%d ship=%s character=%s hq=%s fleet=%d"),
			static_cast<int32>(Spawn->CharacterSpawnPlace), *GetNameSafe(Spawn->BP_HomeSpaceship.Get()),
			*GetNameSafe(Spawn->BP_CharacterClass.Get()), *GetNameSafe(Spawn->BP_HomeSpaceHeadquarters.Get()),
			Spawn->StartingFleetSize);
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
		GGeneratedStart.Planets = Args.Num() > 5 ? FCString::Atoi(*Args[5]) : -1;
		GGeneratedStart.Ground = Args.Num() > 6 && Args[6] != TEXT("-") ? Args[6] : FString();
		GGeneratedStart.HeadquartersClassPath = Args.Num() > 7 && Args[7] != TEXT("-") ? Args[7] : FString();
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
		TEXT("aps.Ship.StartGenerated [SpawnPlace=0 orbit] [ShipBlueprintPath|-] [CharacterBlueprintPath|-] [Moons|-1] [FleetShips|-1] [Planets|-1] [package/pad/vehicles|-] [HeadquartersBlueprintPath|-]: from the main menu, ")
		TEXT("opens the Civilization generator on the home planet and starts L_WorldGeneration like the menu's Start ")
		TEXT("(optionally with another home ship or pilot)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartGenerated));

	/** Rio 09.10 (playtest 08.10 items 1-3, seat checks in Single): the main menu's Start Single Game, without clicking. */
	FTSTicker::FDelegateHandle GStartSingleTicker;

	bool TickStartSingle(float)
	{
		AMainMenuController* Controller = FindMenuController();
		if (!Controller)
		{
			return true; // the menu is not up yet
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] aps.Test.StartSingle: Start Single Game, as the menu's button does"));
		GStartSingleTicker.Reset();
		Controller->LaunchSingleGame();
		return false;
	}

	void StartSingle(const TArray<FString>&, UWorld*)
	{
		if (!GStartSingleTicker.IsValid())
		{
			GStartSingleTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickStartSingle), 0.5f);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs StartSingleCommand(
		TEXT("aps.Test.StartSingle"),
		TEXT("aps.Test.StartSingle: from the main menu, Start Single Game as its button does (the authored L_APS_SinglePlay_StartLocation)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartSingle));
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
		/** at=<seconds>:<command>: console commands at those seconds after boarding (+ stands for a space). */
		TArray<TPair<double, FString>> Timed;
		/** noboard: the player stays on foot; the timed commands count from the end of the warmup, then the hold ends the run. */
		bool bNoBoard{false};
		double BoardedAt{0.0};
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
		// Rio 06.10 (star approach harness, the 900 s course leg): a hold longer than the old 15 min budget gets its warmup,
		// its hold and five minutes of loading; every shorter run keeps the 15 min timeout.
		const double TimeoutSeconds = FMath::Max(900.0, GAutoRun.WarmupSeconds + GAutoRun.HoldSeconds + 300.0);
		if (Now - GAutoRun.StartSeconds > TimeoutSeconds)
		{
			EndAutoRun(TimeoutSeconds > 900.0 ? *FString::Printf(TEXT("timeout after %.0f s"), TimeoutSeconds)
				: TEXT("timeout after 15 min"));
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
			if (GAutoRun.bNoBoard)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] auto run: staying on foot as %s"), *Pawn->GetName());
				GAutoRun.ShipDriveArgs.Reset();
			}
			else if (!BoardShip(Pawn, GAutoRun.Filter, GAutoRun.bSkipChildActors))
			{
				EndAutoRun(TEXT("boarding failed"));
				return false;
			}
			GAutoRun.bBoarded = true;
			GAutoRun.BoardedSeconds = Now;
			GAutoRun.BoardedAt = Now;
			return true;
		}
		for (int32 Index = 0; Index < GAutoRun.Timed.Num(); ++Index)
		{
			if (Now - GAutoRun.BoardedAt >= GAutoRun.Timed[Index].Key)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ShipBench] at %.0f s: %s"), GAutoRun.Timed[Index].Key, *GAutoRun.Timed[Index].Value);
				GEngine->Exec(Pawn->GetWorld(), *GAutoRun.Timed[Index].Value);
				GAutoRun.Timed.RemoveAt(Index--);
			}
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
				// After an exit (exit=1): the pilot's own view 2 s later, then (Rio 04.10) at the ship 5 and 9 s later: its
				// hull must still be drawn after the exit's world shift. Then finish.
				if (GExitShots.ExitSeconds >= 0.0 && GExitShots.Taken < 3)
				{
					if (GExitShots.Taken >= 1)
					{
						LookAtNearestShip(Pawn ? Pawn->GetWorld() : nullptr, Pawn, false);
					}
					static const double Offsets[] = {2.0, 5.0, 9.0};
					if (Now - GExitShots.ExitSeconds >= Offsets[GExitShots.Taken])
					{
						const FString File = FPaths::ScreenShotDir() / TEXT("ShipDrive")
							/ FString::Printf(TEXT("%s_texit%.0f.png"), *GExitShots.Label, Offsets[GExitShots.Taken]);
						FScreenshotRequest::RequestScreenshot(File, false, false);
						UE_LOG(LogTemp, Log, TEXT("[APS.ShipDrive] exit shot %s pawn=%s"), *FPaths::GetCleanFilename(File),
							*GetNameSafe(Pawn));
						++GExitShots.Taken;
					}
					return true;
				}
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
				// Rio 06.10 (star approach harness): up to 30 min (the course leg holds 900 s); was 600 s.
				GAutoRun.HoldSeconds = FMath::Clamp(FCString::Atod(*Args[Index].RightChop(5)), 0.0, 1800.0);
			}
			else if (Args[Index].StartsWith(TEXT("map="), ESearchCase::IgnoreCase))
			{
				GAutoRun.Map = Args[Index].RightChop(4);
			}
			else if (Args[Index].Equals(TEXT("free"), ESearchCase::IgnoreCase))
			{
				GAutoRun.bSkipChildActors = true;
			}
			else if (Args[Index].Equals(TEXT("noboard"), ESearchCase::IgnoreCase))
			{
				GAutoRun.bNoBoard = true;
			}
			else if (Args[Index].StartsWith(TEXT("at="), ESearchCase::IgnoreCase))
			{
				FString Seconds;
				FString Command;
				if (Args[Index].RightChop(3).Split(TEXT(":"), &Seconds, &Command))
				{
					GAutoRun.Timed.Emplace(FCString::Atod(*Seconds), Command.Replace(TEXT("+"), TEXT(" ")));
				}
			}
			else if (Args[Index].StartsWith(TEXT("shipdrive="), ESearchCase::IgnoreCase))
			{
				GAutoRun.ShipDriveArgs = Args[Index].RightChop(10).Replace(TEXT("/"), TEXT(" ")) + GAutoRun.ShipDriveArgs;
			}
			else if (Args[Index].StartsWith(TEXT("aim="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("pitch="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("minalt="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("startly="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("jump="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("exit="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("walk="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("yaw="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("engine="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("shots="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("burst="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("shotlabel="), ESearchCase::IgnoreCase)
				|| Args[Index].StartsWith(TEXT("shotui="), ESearchCase::IgnoreCase))
			{
				GAutoRun.ShipDriveArgs += TEXT(" ") + Args[Index];
				// Timed aps.Test.Shot commands can come before the drive starts: they take the run's label too.
				if (Args[Index].StartsWith(TEXT("shotlabel="), ESearchCase::IgnoreCase))
				{
					GDrive.ShotLabel = Args[Index].RightChop(10);
				}
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

	void TestMap(UWorld* World)
	{
		AGravityPlayerController* Controller = World ? Cast<AGravityPlayerController>(World->GetFirstPlayerController()) : nullptr;
		if (!Controller)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.Map: no gravity player controller"));
			return;
		}
		Controller->ToggleStrategicMap();
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] F10 map %s"), Controller->IsStrategicMapOpen() ? TEXT("open") : TEXT("closed"));
	}

	void TestBuildMode(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		ACustomGravityCharacter* Character = Controller ? Cast<ACustomGravityCharacter>(Controller->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.BuildMode: the player is not on foot"));
			return;
		}
		Character->ToggleBuildMode();
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] build mode %s"), Character->IsInBuildMode() ? TEXT("on") : TEXT("off"));
	}

	void TestShot(const TArray<FString>& Args, UWorld*)
	{
		// The run script collects <label>_t*.png from Saved/Screenshots/ShipDrive.
		const FString Name = Args.IsEmpty() ? FString::Printf(TEXT("%.0f"), FPlatformTime::Seconds()) : Args[0];
		const FString File = FPaths::ScreenShotDir() / TEXT("ShipDrive") / FString::Printf(TEXT("%s_t%s.png"), *GDrive.ShotLabel, *Name);
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] shot %s"), *FPaths::GetCleanFilename(File));
	}

	void TestKey(const TArray<FString>& Args, UWorld* World)
	{
		// Rio 04.10 checks: a key pressed as the player would (the F10 map's G, the cockpit's Y); Slate routes it to the focus.
		const FKey Key = Args.IsEmpty() ? FKey() : FKey(*Args[0]);
		if (!Key.IsValid() || !FSlateApplication::IsInitialized())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.Key <Key>: unknown key or no Slate"));
			return;
		}
		FSlateApplication& Slate = FSlateApplication::Get();
		const uint32 User = Slate.GetUserIndexForKeyboard();
		bool bHandled = Slate.ProcessKeyDownEvent(FKeyEvent(Key, FModifierKeysState(), User, false, 0, 0));
		Slate.ProcessKeyUpEvent(FKeyEvent(Key, FModifierKeysState(), User, false, 0, 0));
		// An offscreen run's viewport may hold no Slate focus: the key then goes to the player's input as the game's own.
		APlayerController* Controller = !bHandled && World ? World->GetFirstPlayerController() : nullptr;
		if (Controller)
		{
			bHandled = Controller->InputKey(FInputKeyParams(Key, IE_Pressed, 1.0, false));
			Controller->InputKey(FInputKeyParams(Key, IE_Released, 0.0, false));
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] key %s %s%s"), *Key.ToString(), bHandled ? TEXT("handled") : TEXT("not handled"),
			Controller ? TEXT(" (player input)") : TEXT(""));
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Rio 10.10 night (freezes on foot, zero-G "small jerks"): an offscreen run's viewport gets no key input, so the walker
	// is driven here: once the player's gravity character stands in the world, after a warm-up it walks straight on (pace 3,
	// optional sprint, optional zero-G) and every frame's dt, walker place and camera place go to a CSV
	// (Saved/Diagnostics/Walk); hitches come from [APS.Perf]. Test-only.
	struct FTestWalk
	{
		TWeakObjectPtr<UWorld> World;
		double Warmup{30.0};
		double Seconds{30.0};
		bool bSprint{false};
		bool bZeroG{false};
		bool bQuit{false};
		double Waited{0.0};
		double Elapsed{0.0};
		bool bWalking{false};
		FVector Dir{FVector::ZeroVector};
		FVector CheckPlace{FVector::ZeroVector};
		double CheckTime{0.0};
		int32 Turns{0};
		FString Csv;
		FTSTicker::FDelegateHandle Handle;
	};
	FTestWalk GTestWalk;

	bool TickTestWalk(float DeltaTime)
	{
		UWorld* World = nullptr;
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld() && Context.World()->GetName().Contains(TEXT("L_WorldGeneration")))
				{
					World = Context.World();
				}
			}
		}
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		ACustomGravityCharacter* Walker = Controller ? Cast<ACustomGravityCharacter>(Controller->GetPawn()) : nullptr;
		if (!GTestWalk.bWalking)
		{
			// As aps.Char.AutoProbe: the warm-up counts only once the spawn's surface hold has released the walker.
			GTestWalk.Waited = Walker && !Walker->IsSurfaceHandoffSuspended() ? GTestWalk.Waited + DeltaTime : 0.0;
			if (!Walker || GTestWalk.Waited < GTestWalk.Warmup)
			{
				return true;
			}
			GTestWalk.bWalking = true;
			if (AAPSSpeedModeCharacter* Paced = Cast<AAPSSpeedModeCharacter>(Walker))
			{
				Paced->SetSpeedMode(EAPSSpeedMode::Open);
			}
			Walker->SetSprintHeldForTest(GTestWalk.bSprint);
			if (GTestWalk.bZeroG)
			{
				Walker->ToggleManualZeroGOverride();
			}
			GTestWalk.Csv = TEXT("time,dt_ms,pawn_x,pawn_y,pawn_z,cam_x,cam_y,cam_z\n");
			const UCharacterMovementComponent* Movement = Walker->GetCharacterMovement();
			UE_LOG(LogTemp, Log, TEXT("[APS.TestWalk] start walker=%s sprint=%d zeroG=%d seconds=%.0f mode=%d moveInputIgnored=%d maxWalk=%.0f"),
				*GetNameSafe(Walker), GTestWalk.bSprint ? 1 : 0, GTestWalk.bZeroG ? 1 : 0, GTestWalk.Seconds,
				Movement ? static_cast<int32>(Movement->MovementMode.GetValue()) : -1, Controller->IsMoveInputIgnored() ? 1 : 0,
				Movement ? Movement->MaxWalkSpeed : -1.f);
		}
		if (!Walker || GTestWalk.Elapsed >= GTestWalk.Seconds)
		{
			if (Walker)
			{
				Walker->SetSprintHeldForTest(false);
			}
			const FString Path = FPaths::ProjectSavedDir() / TEXT("Diagnostics/Walk")
				/ FString::Printf(TEXT("%s_walk.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
			FFileHelper::SaveStringToFile(GTestWalk.Csv, *Path);
			UE_LOG(LogTemp, Log, TEXT("[APS.TestWalk] %s after %.1f s (%d turns at obstacles), csv=%s"),
				Walker ? TEXT("finished") : TEXT("walker lost"), GTestWalk.Elapsed, GTestWalk.Turns,
				*FPaths::ConvertRelativePathToFull(Path));
			GTestWalk.Handle.Reset();
			if (GTestWalk.bQuit)
			{
				FPlatformMisc::RequestExit(false, TEXT("aps.Test.Walk"));
			}
			return false;
		}
		GTestWalk.Elapsed += DeltaTime;
		// Straight on, kept on the gravity plane on the ground (pace and sprint as the player). A walker that made under
		// 30 cm in half a second (a wall of the base it starts beside) turns 75 deg about its up and goes on.
		const FVector Up = Walker->GetActorUpVector();
		if (GTestWalk.Dir.IsNearlyZero())
		{
			// Weightless: straight up, away from the base (nothing to bump into: a clean camera-follow measure).
			GTestWalk.Dir = GTestWalk.bZeroG ? Up
				: FVector::VectorPlaneProject(Walker->GetActorForwardVector(), Up).GetSafeNormal();
			GTestWalk.CheckPlace = Walker->GetActorLocation();
			GTestWalk.CheckTime = GTestWalk.Elapsed;
		}
		if (GTestWalk.Elapsed - GTestWalk.CheckTime >= 0.5)
		{
			if (FVector::Dist(Walker->GetActorLocation(), GTestWalk.CheckPlace) < 30.0)
			{
				GTestWalk.Dir = FQuat(Up, FMath::DegreesToRadians(75.0)).RotateVector(GTestWalk.Dir);
				++GTestWalk.Turns;
			}
			GTestWalk.CheckPlace = Walker->GetActorLocation();
			GTestWalk.CheckTime = GTestWalk.Elapsed;
		}
		if (!GTestWalk.bZeroG)
		{
			GTestWalk.Dir = FVector::VectorPlaneProject(GTestWalk.Dir, Up).GetSafeNormal();
		}
		// Forced: a run's start screens may leave the controller ignoring move input; the walker itself still walks.
		Walker->AddMovementInput(GTestWalk.Dir, 1.0f, true);
		// Last frame's walker and camera (both final for that frame): the camera's motion against the walker's.
		const FVector Pawn = Walker->GetActorLocation();
		const FVector Cam = Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
		GTestWalk.Csv += FString::Printf(TEXT("%.4f,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n"), GTestWalk.Elapsed, DeltaTime * 1000.0,
			Pawn.X, Pawn.Y, Pawn.Z, Cam.X, Cam.Y, Cam.Z);
		return true;
	}

	void TestWalk(const TArray<FString>& Args, UWorld*)
	{
		GTestWalk = FTestWalk();
		for (const FString& Arg : Args)
		{
			FString Key, Value;
			if (Arg.Split(TEXT("="), &Key, &Value))
			{
				if (Key == TEXT("warmup")) GTestWalk.Warmup = FCString::Atod(*Value);
				else if (Key == TEXT("duration")) GTestWalk.Seconds = FCString::Atod(*Value);
				else if (Key == TEXT("sprint")) GTestWalk.bSprint = FCString::Atoi(*Value) != 0;
				else if (Key == TEXT("zerog")) GTestWalk.bZeroG = FCString::Atoi(*Value) != 0;
			}
			else if (Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase))
			{
				GTestWalk.bQuit = true;
			}
		}
		GTestWalk.Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickTestWalk));
		UE_LOG(LogTemp, Log, TEXT("[APS.TestWalk] armed: warmup=%.0f duration=%.0f sprint=%d zeroG=%d quit=%d"), GTestWalk.Warmup,
			GTestWalk.Seconds, GTestWalk.bSprint ? 1 : 0, GTestWalk.bZeroG ? 1 : 0, GTestWalk.bQuit ? 1 : 0);
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Rio 06.10 (star approach v2, stage A harness): galaxy targets, a registration watch, arrival stages and a steering
	// pilot for the offscreen star approach runs (F:/ChatGPT/APOSFERA/work/flight/run_star_approach.ps1, checked by
	// star_approach_check.py). Test-only: nothing here runs unless a run's console commands ask for it.

	constexpr double TestLightYearCm = 9.4607304725808e17;
	constexpr double TestAuCm = 1.495978707e13;

	TAutoConsoleVariable<FString> CVarSteerDrawnDir(
		TEXT("aps.Test.SteerDrawnDir"), TEXT(""),
		TEXT("Rio 06.10 (star approach harness): '<frame> <x> <y> <z>', the unit direction from the view to the traced star's ")
		TEXT("drawn dot as the approach trace (aps.Stars.ApproachTrace) writes it; aps.Test.Steer aims there while it is fresh. ")
		TEXT("Empty or stale: the pilot aims at the star's exact catalogue place."));

	/** The exact place where the sky draws a galaxy catalogue star now (its record through the indexed galaxy). */
	bool GalaxyStarExactPlace(const UWorld* World, const int64 CatalogIndex, FVector& OutPlace)
	{
		const AGalaxy* Galaxy = APSGalaxyGpuStars::GetIndexedGalaxy(World);
		FGalaxyCatalogStarRecord Record;
		return IsValid(Galaxy) && CatalogIndex != INDEX_NONE && Galaxy->StarCatalog.ResolveStar(CatalogIndex, Record)
			&& APSGalaxyGpuStars::ProjectCatalogueLocation(World, Record.GalaxyLocalLocation, OutPlace);
	}

	/** The approach trace's star (aps.Stars.ApproachTraceTarget, APSGalaxyGpuStars.cpp; found by name); INDEX_NONE without one. */
	int64 TracedCatalogIndex()
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Stars.ApproachTraceTarget"));
		const FString Value = Variable ? Variable->GetString() : FString();
		return Value.IsNumeric() ? FCString::Atoi64(*Value) : static_cast<int64>(INDEX_NONE);
	}

	/** What the registration watch saw when a galaxy system came into the registry. */
	struct FWatchedGalaxySystem
	{
		/** The owed travel grew in that frame: a registration of the failing class (its place may carry that frame's step). */
		bool bInFlight{false};
		double OwedLy{0.0};
		double StepLy{0.0};
	};

	/** aps.Test.WatchRegistrations: every frame, the galaxy systems the registry added since the last one. */
	struct FRegistrationWatch
	{
		bool bActive{false};
		TWeakObjectPtr<UWorld> World;
		int32 Seen{0};
		FVector LastSky{FVector::ZeroVector};
		TMap<int64, FWatchedGalaxySystem> Galaxy;
		FTSTicker::FDelegateHandle Ticker;
	};
	FRegistrationWatch GWatch;

	bool TickRegistrationWatch(float)
	{
		if (!GWatch.bActive)
		{
			return false;
		}
		APawn* Pawn = FindPlayerPawn(nullptr);
		UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
		const FAPSStarSystems* Systems = World ? APSStarSystemsFind(World) : nullptr;
		if (!Systems)
		{
			return true;
		}
		// The core ticker runs between frames: the sky offset's change since the last one is the frame's owed step (a pay
		// clears it, so a frame that also paid reads as not in flight).
		const FVector Sky = UAPSWorldOriginSubsystem::SkyOffsetOf(World);
		const bool bFirstLook = GWatch.World.Get() != World || GWatch.Seen > Systems->Num();
		if (bFirstLook)
		{
			GWatch.World = World;
			GWatch.Seen = 0;
			GWatch.Galaxy.Reset();
			GWatch.LastSky = Sky;
		}
		const double StepLy = FVector::Dist(Sky, GWatch.LastSky) / TestLightYearCm;
		GWatch.LastSky = Sky;
		for (; GWatch.Seen < Systems->Num(); ++GWatch.Seen)
		{
			const FAPSStarSystemInfo* Info = Systems->Get(GWatch.Seen);
			if (!Info || Info->GalaxyIndex == INDEX_NONE)
			{
				continue;
			}
			FWatchedGalaxySystem& Entry = GWatch.Galaxy.FindOrAdd(Info->GalaxyIndex);
			Entry.OwedLy = Sky.Size() / TestLightYearCm;
			Entry.StepLy = StepLy;
			Entry.bInFlight = !bFirstLook && Entry.OwedLy > 0.0 && StepLy > 0.0;
			FVector Exact = FVector::ZeroVector;
			const double AeKm = GalaxyStarExactPlace(World, Info->GalaxyIndex, Exact) ? FVector::Dist(Info->Location, Exact) / 1.0e5 : -1.0;
			const double RadiusKm = Info->StarRadiusCm / 1.0e5;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Test] galaxy system registered: %s catalogue %lld radius %.0f km, AE %.6g km (%.4g radii), owed %.6g ly, ")
				TEXT("sky step %.6g ly (%s)"),
				*Info->Name.Replace(TEXT(" "), TEXT("_")), Info->GalaxyIndex, RadiusKm, AeKm,
				RadiusKm > 0.0 && AeKm >= 0.0 ? AeKm / RadiusKm : -1.0, Entry.OwedLy, StepLy,
				bFirstLook ? TEXT("first look") : Entry.bInFlight ? TEXT("in flight") : TEXT("at rest"));
		}
		return true;
	}

	void TestWatchRegistrations(const TArray<FString>& Args, UWorld*)
	{
		const bool bOn = Args.IsEmpty() || FCString::Atoi(*Args[0]) != 0;
		if (bOn && !GWatch.bActive)
		{
			GWatch = FRegistrationWatch();
			GWatch.bActive = true;
			GWatch.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickRegistrationWatch), 0.0f);
		}
		else if (!bOn && GWatch.bActive)
		{
			GWatch.bActive = false;
			FTSTicker::GetCoreTicker().RemoveTicker(GWatch.Ticker);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] galaxy registration watch %s"), GWatch.bActive ? TEXT("on") : TEXT("off"));
	}

	/** The ship's flight up axis in the world (ASpaceship::GetShipUpVector is protected): its rotation times the local flight
	 * frame that FAPSShipFlightBenchmark::GetRotationForFlightAxes turns onto the world axes. */
	FVector TestShipUpVector(const ASpaceship& Ship)
	{
		const FQuat ToWorldAxes = FAPSShipFlightBenchmark::GetRotationForFlightAxes(Ship, FVector::ForwardVector, FVector::UpVector);
		return Ship.GetActorQuat().RotateVector(ToWorldAxes.Inverse().RotateVector(FVector::UpVector)).GetSafeNormal(
			UE_DOUBLE_SMALL_NUMBER, Ship.GetActorUpVector());
	}

	/** The player view's pixel tangent as the stellar view measures it (aps.Test.Pose disc, the far<N> selector's log). */
	double TestViewPixelTangent(APlayerController& Controller)
	{
		int32 Width = 0;
		int32 Height = 0;
		Controller.GetViewportSize(Width, Height);
		const double Fov = Controller.PlayerCameraManager ? Controller.PlayerCameraManager->GetFOVAngle() : 90.0;
		return APSStellarViewOptics::PixelTangent(&Controller, 2.0 * FMath::Tan(FMath::DegreesToRadians(Fov * 0.5)) / FMath::Max(Width, 320));
	}

	/**
	 * Rio 06.10 (star approach v2, stage B harness, selector far<N>): a GPU-drawn galaxy star about N ly from the ship whose
	 * course far take falls well inside the flight: around the six points N ly along the ship's forward, back, right, left, up
	 * and down (the index box is only ~4600 ly, so one direction may leave it), the 256 drawn stars nearest each within 200 ly;
	 * kept when the GPU layer draws it (aps.Stars.ApproachTrace's DescribeGpuStar), its unclamped course take radius lies in
	 * [30, 0.9 x 500] ly (the clamp never applies, so a run with aps.Stars.ApproachCourseMaxLy 0 picks the same star and still
	 * fails the far take) and the ship starts at least 1.5 radii from it. The one whose twin stands farthest off its exact place
	 * is registered (direct, at rest) and returned as a registry index; INDEX_NONE (a Warning) when none qualifies.
	 */
	int32 FindFarGalaxyStar(const UWorld* World, const FVector& From, const double DistanceLy)
	{
		FAPSStarSystems* Registry = World ? APSStarSystemsFind(World) : nullptr;
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Registry || !Pawn || !(DistanceLy > 0.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] far%.0f: no registry, no pawn or no distance"), DistanceLy);
			return INDEX_NONE;
		}
		constexpr double CourseCapLy = 500.0;
		constexpr double MinRawLy = 30.0;
		constexpr double MaxRawLy = 0.9 * CourseCapLy;
		const ASpaceship* PilotedShip = Cast<ASpaceship>(Pawn);
		const FVector Forward = (PilotedShip ? PilotedShip->GetShipForwardVector() : Pawn->GetActorForwardVector()).GetSafeNormal(
			UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
		const FVector Up = (PilotedShip ? TestShipUpVector(*PilotedShip) : Pawn->GetActorUpVector()).GetSafeNormal(
			UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
		const FVector Aside = FVector::CrossProduct(Up, Forward).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Pawn->GetActorRightVector());
		const FVector Directions[] = {Forward, -Forward, Aside, -Aside, Up, -Up};
		const TCHAR* const DirectionNames[] = {TEXT("forward"), TEXT("back"), TEXT("right"), TEXT("left"), TEXT("up"), TEXT("down")};
		struct FFarCandidate
		{
			int64 CatalogIndex{INDEX_NONE};
			APSGalaxyGpuStars::FGpuStarInfo Info;
			double DistanceCm{0.0};
			int32 Direction{0};
		};
		TArray<FFarCandidate> Candidates;
		TSet<int64> Seen;
		int32 Considered = 0;
		int32 GpuDrawn = 0;
		for (int32 DirectionIndex = 0; DirectionIndex < static_cast<int32>(UE_ARRAY_COUNT(Directions)); ++DirectionIndex)
		{
			TArray<APSGalaxyGpuStars::FNearStar> Near;
			if (!APSGalaxyGpuStars::FindNearStars(World, From + Directions[DirectionIndex] * DistanceLy * TestLightYearCm, 256,
				200.0 * TestLightYearCm, Near))
			{
				continue;
			}
			for (const APSGalaxyGpuStars::FNearStar& Star : Near)
			{
				bool bSeen = false;
				Seen.Add(Star.CatalogIndex, &bSeen);
				if (bSeen)
				{
					continue;
				}
				++Considered;
				FFarCandidate Candidate;
				if (!APSGalaxyGpuStars::DescribeGpuStar(World, Star.CatalogIndex, Candidate.Info) || !Candidate.Info.bGpu)
				{
					continue;
				}
				++GpuDrawn;
				const double RawLy = Candidate.Info.CourseTakeRawCm / TestLightYearCm;
				Candidate.DistanceCm = FVector::Dist(From, Candidate.Info.ExactWorld);
				if (RawLy < MinRawLy || RawLy > MaxRawLy || Candidate.DistanceCm < 1.5 * Candidate.Info.CourseTakeRawCm)
				{
					continue;
				}
				Candidate.CatalogIndex = Star.CatalogIndex;
				Candidate.Direction = DirectionIndex;
				Candidates.Add(Candidate);
			}
		}
		Candidates.Sort([](const FFarCandidate& A, const FFarCandidate& B)
		{
			return A.Info.TwinOffsetCm > B.Info.TwinOffsetCm;
		});
		// The farthest twin first; one the registry refuses (the home sphere, a cluster room) gives way to the next.
		for (int32 Attempt = 0; Attempt < FMath::Min(Candidates.Num(), 8); ++Attempt)
		{
			const FFarCandidate& Best = Candidates[Attempt];
			int32 Index = Registry->IndexOfGalaxyStar(Best.CatalogIndex);
			if (Index == INDEX_NONE)
			{
				Index = Registry->RegisterGalaxyStar(Best.CatalogIndex);
			}
			const FAPSStarSystemInfo* Info = Index != INDEX_NONE ? Registry->Get(Index) : nullptr;
			if (!Info)
			{
				continue;
			}
			const double PixelTangent = TestViewPixelTangent(*Controller);
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Test] far galaxy star %s catalogue %lld: %.6g ly, L%d, twin %.4g AU off (%.3f px at 3 ly), course take radius ")
				TEXT("%.4g ly (unclamped %.4g ly); %s of the ship, %d drawn star(s) looked at, %d GPU-drawn, %d qualified"),
				*Info->Name.Replace(TEXT(" "), TEXT("_")), Best.CatalogIndex, Best.DistanceCm / TestLightYearCm, Best.Info.Level,
				Best.Info.TwinOffsetCm / TestAuCm,
				PixelTangent > 0.0 ? Best.Info.TwinOffsetCm / (3.0 * TestLightYearCm * PixelTangent) : -1.0,
				Best.Info.CourseTakeCm / TestLightYearCm, Best.Info.CourseTakeRawCm / TestLightYearCm,
				DirectionNames[Best.Direction], Considered, GpuDrawn, Candidates.Num());
			return Index;
		}
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Test] far%.0f: no galaxy star qualifies (%d drawn star(s) looked at, %d GPU-drawn, %d with an unclamped course ")
			TEXT("take radius in %.0f-%.0f ly at 1.5 radii or more; none registered)"),
			DistanceLy, Considered, GpuDrawn, Candidates.Num(), MinRawLy, MaxRawLy);
		return INDEX_NONE;
	}

	/** galaxyN, galaxyowedN, cat<index>, far<ly> or traced (FindGalaxyTarget); anything else is a name. */
	bool IsGalaxySelector(const FString& Selector)
	{
		const auto Ranked = [&Selector](const TCHAR* Prefix)
		{
			const int32 Length = FCString::Strlen(Prefix);
			return Selector.Len() > Length && Selector.StartsWith(Prefix, ESearchCase::IgnoreCase)
				&& Selector.RightChop(Length).IsNumeric();
		};
		return Ranked(TEXT("galaxyowed")) || Ranked(TEXT("galaxy")) || Ranked(TEXT("cat")) || Ranked(TEXT("far"))
			|| Selector.Equals(TEXT("traced"), ESearchCase::IgnoreCase);
	}

	/**
	 * A registered galaxy system for a run, as a registry index (INDEX_NONE: none). galaxy<N>: the N-th nearest by its exact
	 * place (the same order with and without a registry skew); galaxyowed<N>: the same among those the registration watch
	 * saw registered while the owed travel grew (the failing class); cat<index>: by catalogue index; far<ly>: a GPU star about
	 * that far whose course far take falls inside the flight (FindFarGalaxyStar, registered now); traced: the one
	 * aps.Stars.ApproachTraceTarget follows. The ranked ones skip the home, systems inside it and the materialized one.
	 */
	int32 FindGalaxyTarget(const UWorld* World, const FVector& From, const FString& Selector)
	{
		const FAPSStarSystems* Systems = APSStarSystemsFind(World);
		if (!Systems)
		{
			return INDEX_NONE;
		}
		if (Selector.Equals(TEXT("traced"), ESearchCase::IgnoreCase))
		{
			const int64 Traced = TracedCatalogIndex();
			return Traced != INDEX_NONE ? Systems->IndexOfGalaxyStar(Traced) : INDEX_NONE;
		}
		if (Selector.StartsWith(TEXT("cat"), ESearchCase::IgnoreCase))
		{
			const int64 Wanted = FCString::Atoi64(*Selector.RightChop(3));
			const int32 Known = Systems->IndexOfGalaxyStar(Wanted);
			if (Known != INDEX_NONE)
			{
				return Known;
			}
			// The pair's fix run flies the off run's star (run_star_approach.ps1 -Set both): when this run's neighbour scan has
			// not registered it, it is registered now (direct, at rest), so the run still flies to the same star.
			FAPSStarSystems* Registry = APSStarSystemsFind(World);
			const int32 Added = Registry ? Registry->RegisterGalaxyStar(Wanted) : INDEX_NONE;
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] %s: not registered by this run's scan; %s"), *Selector,
				Added != INDEX_NONE ? TEXT("registered now (direct, at rest)")
				: TEXT("it cannot be (no nearest-star index, the home sphere or a cluster room)"));
			return Added;
		}
		if (Selector.StartsWith(TEXT("far"), ESearchCase::IgnoreCase))
		{
			// Rio 06.10 (star approach v2, stage B harness): the course star's far take (FindFarGalaxyStar).
			return FindFarGalaxyStar(World, From, FCString::Atod(*Selector.RightChop(3)));
		}
		const bool bOwed = Selector.StartsWith(TEXT("galaxyowed"), ESearchCase::IgnoreCase);
		const int32 Rank = FMath::Max(FCString::Atoi(*Selector.RightChop(bOwed ? 10 : 6)), 1);
		const FAPSSystemMaterializer* Materializer = Systems->GetMaterializer();
		const int32 Active = Materializer ? Materializer->GetActiveIndex() : INDEX_NONE;
		TArray<TPair<double, int32>> Candidates;
		for (int32 Index = 0; Index < Systems->Num(); ++Index)
		{
			const FAPSStarSystemInfo* Info = Systems->Get(Index);
			const FWatchedGalaxySystem* Watched = Info ? GWatch.Galaxy.Find(Info->GalaxyIndex) : nullptr;
			FVector Exact = FVector::ZeroVector;
			if (!Info || Info->GalaxyIndex == INDEX_NONE || Info->bHome || Info->bInsideHome || Index == Active
				|| (bOwed && !(Watched && Watched->bInFlight)) || !GalaxyStarExactPlace(World, Info->GalaxyIndex, Exact))
			{
				continue;
			}
			Candidates.Emplace(FVector::DistSquared(From, Exact), Index);
		}
		Candidates.Sort([](const TPair<double, int32>& Left, const TPair<double, int32>& Right) { return Left.Key < Right.Key; });
		if (!Candidates.IsValidIndex(Rank - 1))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] %s: only %d galaxy system(s) qualify%s"), *Selector, Candidates.Num(),
				bOwed && !GWatch.bActive ? TEXT(" (aps.Test.WatchRegistrations is off)") : TEXT(""));
			return INDEX_NONE;
		}
		return Candidates[Rank - 1].Value;
	}

	/** Logs a galaxy run target with what the check needs (its radius, its exact place against the registry's, how it was
	 * registered) and makes it the approach trace's star unless told not to. */
	void AnnounceGalaxyTarget(const UWorld* World, const FAPSStarSystemInfo& Info, const FVector& From, const bool bTrace)
	{
		FVector Exact = Info.Location;
		const bool bExact = GalaxyStarExactPlace(World, Info.GalaxyIndex, Exact);
		const FWatchedGalaxySystem* Watched = GWatch.Galaxy.Find(Info.GalaxyIndex);
		const double RadiusKm = Info.StarRadiusCm / 1.0e5;
		const double AeKm = bExact ? FVector::Dist(Info.Location, Exact) / 1.0e5 : -1.0;
		const FString How = !Watched ? FString(TEXT("unwatched")) : Watched->bInFlight
			? FString::Printf(TEXT("in flight (owed %.6g ly, step %.6g ly)"), Watched->OwedLy, Watched->StepLy) : FString(TEXT("at rest"));
		const FString Name = Info.Name.Replace(TEXT(" "), TEXT("_"));
		UE_LOG(LogTemp, Log,
			TEXT("[APS.Test] galaxy target %s catalogue %lld radius %.0f km: %.6g ly to its exact place, AE %.6g km (%.4g radii), ")
			TEXT("registered %s"),
			*Name, Info.GalaxyIndex, RadiusKm, FVector::Dist(From, Exact) / TestLightYearCm, AeKm,
			RadiusKm > 0.0 && AeKm >= 0.0 ? AeKm / RadiusKm : -1.0, *How);
		if (!bTrace)
		{
			return;
		}
		IConsoleVariable* TraceTarget = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Stars.ApproachTraceTarget"));
		if (!TraceTarget)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] no aps.Stars.ApproachTraceTarget in this build: %s is not traced"), *Name);
			return;
		}
		TraceTarget->Set(*FString::Printf(TEXT("%lld"), Info.GalaxyIndex), ECVF_SetByConsole);
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] trace target %s catalogue %lld (aps.Stars.ApproachTraceTarget)"), *Name, Info.GalaxyIndex);
	}

	/** Turns the pawn (a ship by its flight axes) and the view to a direction, N degrees aside about the view's up, as
	 * aps.Test.Pose star does; bStop stops a ship first. */
	void FaceForTest(APlayerController& Controller, APawn& Pawn, const FVector& Direction, const double AsideDegrees, const bool bStop)
	{
		const FVector To = Direction.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Pawn.GetActorForwardVector());
		FVector Up = FVector::VectorPlaneProject(FVector::UpVector, To).GetSafeNormal();
		if (Up.IsNearlyZero())
		{
			Up = FVector::VectorPlaneProject(FVector::ForwardVector, To).GetSafeNormal();
		}
		const FVector Forward = FQuat(Up, FMath::DegreesToRadians(-AsideDegrees)).RotateVector(To);
		if (ASpaceship* Ship = Cast<ASpaceship>(&Pawn))
		{
			if (bStop)
			{
				FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
			}
			Pawn.SetActorRotation(FAPSShipFlightBenchmark::GetRotationForFlightAxes(*Ship, Forward, Up), ETeleportType::TeleportPhysics);
		}
		Controller.SetControlRotation(Forward.Rotation());
	}

	/**
	 * aps.Test.OnArrival / aps.Test.OnLog: test stages that wait for a log line. Rio 06.10 (star approach v2, stage B harness):
	 * the arrival watch generalised to a token table. arrived: the piloted ship's autopilot arrivals, read from the flight
	 * model's own line ("[APS.Autopilot] <ship> off: arrived"), so an abort or a helm takeover never counts (critique: not
	 * IsAutopilotEngaged going false); coursetake: the course star's far take ("[APS.Stars] approach point <index> is the
	 * course star's far take"); standup: a system stands ("[APS.Stars] <name> stands: "). The autopilot watchdog also reads
	 * the flight model's contact lines ("[APS.Flight] <ship> contact <actor.component> ..."). Any thread may log: the hits wait
	 * under a lock until the core ticker takes them.
	 */
	struct FTestLogToken
	{
		const TCHAR* Name;
		const TCHAR* Head;
		const TCHAR* Tail;
	};
	/** The text between Head and Tail is the hit's payload (arrived: the ship, which must be the piloted one). */
	const FTestLogToken GTestLogTokens[] = {
		{TEXT("arrived"), TEXT("[APS.Autopilot] "), TEXT(" off: arrived")},
		{TEXT("coursetake"), TEXT("[APS.Stars] approach point "), TEXT(" is the course star's far take")},
		{TEXT("standup"), TEXT("[APS.Stars] "), TEXT(" stands: ")},
	};
	constexpr int32 TestLogArrived = 0;
	constexpr int32 NumTestLogTokens = static_cast<int32>(UE_ARRAY_COUNT(GTestLogTokens));

	class FArrivalLogWatch final : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type, const FName&) override
		{
			if (!Message)
			{
				return;
			}
			for (int32 Token = 0; Token < NumTestLogTokens; ++Token)
			{
				const TCHAR* Start = FCString::Strstr(Message, GTestLogTokens[Token].Head);
				const TCHAR* Payload = Start ? Start + FCString::Strlen(GTestLogTokens[Token].Head) : nullptr;
				const TCHAR* End = Payload ? FCString::Strstr(Payload, GTestLogTokens[Token].Tail) : nullptr;
				if (End)
				{
					FScopeLock Lock(&Mutex);
					Hits.Emplace(Token, FString(Payload).Left(static_cast<int32>(End - Payload)));
				}
			}
			const TCHAR* Flight = FCString::Strstr(Message, TEXT("[APS.Flight] "));
			const TCHAR* Contact = Flight ? FCString::Strstr(Flight, TEXT(" contact ")) : nullptr;
			if (Contact)
			{
				const TCHAR* ShipName = Flight + FCString::Strlen(TEXT("[APS.Flight] "));
				const TCHAR* What = Contact + FCString::Strlen(TEXT(" contact "));
				const TCHAR* WhatEnd = FCString::Strchr(What, TEXT(' '));
				FScopeLock Lock(&Mutex);
				ContactShip = FString(ShipName).Left(static_cast<int32>(Contact - ShipName));
				LastContact = WhatEnd ? FString(What).Left(static_cast<int32>(WhatEnd - What)) : FString(What);
				++ContactLines;
			}
		}
		virtual bool CanBeUsedOnAnyThread() const override { return true; }
		virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
		/** The hits since the last call: (token, payload) in log order. */
		TArray<TPair<int32, FString>> Take()
		{
			FScopeLock Lock(&Mutex);
			TArray<TPair<int32, FString>> Taken = MoveTemp(Hits);
			Hits.Reset();
			return Taken;
		}
		/** The last contact line of that ship since ResetContacts, and how many contact lines came. */
		bool LastContactOf(const FString& Ship, FString& OutWhat, int32& OutLines)
		{
			FScopeLock Lock(&Mutex);
			OutLines = ContactLines;
			OutWhat = ContactShip == Ship ? LastContact : FString();
			return !OutWhat.IsEmpty();
		}
		void ResetContacts()
		{
			FScopeLock Lock(&Mutex);
			ContactShip.Reset();
			LastContact.Reset();
			ContactLines = 0;
		}

	private:
		FCriticalSection Mutex;
		TArray<TPair<int32, FString>> Hits;
		FString ContactShip;
		FString LastContact;
		int32 ContactLines{0};
	};

	/** A command of a stage that fired, in world seconds. */
	struct FDueTestCommand
	{
		double Seconds{0.0};
		FString Command;
		int32 Token{0};
		int32 Hit{0};
	};

	/** The queued stages (one per aps.Test.OnLog / OnArrival call, each with its token) and the commands of those that fired. */
	struct FArrivalStages
	{
		TArray<TPair<int32, TArray<TPair<double, FString>>>> Waiting;
		TArray<FDueTestCommand> Due;
		int32 HitCounts[NumTestLogTokens] = {};
		/** Made once and never freed: the log may still hold it while the engine shuts down. */
		FArrivalLogWatch* Log{nullptr};
		bool bListening{false};
		bool bTicking{false};
		FTSTicker::FDelegateHandle Ticker;
	};
	FArrivalStages GArrivals;

	/**
	 * Rio 06.10 (star approach v2, stage B harness; the ctl stall: the autopilot engaged on the HQ pad, 338 pad contacts, speed
	 * 0 to the end, no 'off' line): after each aps.Test.Autopilot engage, the piloted ship 15 world seconds later. Still under
	 * 5 m/s with its autopilot on: a Warning with its last contact; otherwise one Log line. It runs 20 s.
	 */
	struct FAutopilotWatch
	{
		bool bActive{false};
		bool bChecked{false};
		TWeakObjectPtr<ASpaceship> Ship;
		FString TargetName;
		double EngageSeconds{0.0};
		FTSTicker::FDelegateHandle Ticker;
	};
	FAutopilotWatch GAutopilotWatch;

	/** The log device listens while a stage waits or is due, or the autopilot watchdog runs. */
	void UpdateTestLogListening()
	{
		const bool bWanted = !GArrivals.Waiting.IsEmpty() || !GArrivals.Due.IsEmpty() || GAutopilotWatch.bActive;
		if (bWanted == GArrivals.bListening)
		{
			return;
		}
		if (bWanted && !GArrivals.Log)
		{
			GArrivals.Log = new FArrivalLogWatch();
		}
		GArrivals.bListening = bWanted;
		if (GLog && GArrivals.Log)
		{
			if (bWanted)
			{
				GLog->AddOutputDevice(GArrivals.Log);
			}
			else
			{
				GLog->RemoveOutputDevice(GArrivals.Log);
			}
		}
	}

	bool TickArrivals(float)
	{
		APawn* Pawn = FindPlayerPawn(nullptr);
		UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
		const double Now = World ? World->GetTimeSeconds() : 0.0;
		const FString Piloted = Cast<ASpaceship>(Pawn) ? Pawn->GetName() : FString();
		TArray<TPair<int32, FString>> Hits;
		if (GArrivals.Log)
		{
			Hits = GArrivals.Log->Take();
		}
		for (const TPair<int32, FString>& Hit : Hits)
		{
			const int32 Token = Hit.Key;
			if (Token == TestLogArrived && (Piloted.IsEmpty() || Hit.Value != Piloted))
			{
				continue;
			}
			const int32 Count = ++GArrivals.HitCounts[Token];
			const int32 Stage = GArrivals.Waiting.IndexOfByPredicate([Token](const TPair<int32, TArray<TPair<double, FString>>>& Each)
			{
				return Each.Key == Token;
			});
			if (Stage == INDEX_NONE)
			{
				if (Token == TestLogArrived)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Test] arrival %d of %s: no stage waits"), Count, *Hit.Value);
				}
				continue;
			}
			for (const TPair<double, FString>& Step : GArrivals.Waiting[Stage].Value)
			{
				FDueTestCommand& Due = GArrivals.Due.AddDefaulted_GetRef();
				Due.Seconds = Now + Step.Key;
				Due.Command = Step.Value;
				Due.Token = Token;
				Due.Hit = Count;
			}
			if (Token == TestLogArrived)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Test] arrival %d of %s: its stage runs %d command(s)"), Count, *Hit.Value,
					GArrivals.Waiting[Stage].Value.Num());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Test] log %s %d (%s): its stage runs %d command(s)"), GTestLogTokens[Token].Name, Count,
					*Hit.Value, GArrivals.Waiting[Stage].Value.Num());
			}
			GArrivals.Waiting.RemoveAt(Stage);
		}
		for (int32 Index = 0; World && Index < GArrivals.Due.Num(); ++Index)
		{
			if (Now >= GArrivals.Due[Index].Seconds)
			{
				const FDueTestCommand Command = GArrivals.Due[Index];
				GArrivals.Due.RemoveAt(Index--);
				if (Command.Token == TestLogArrived)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Test] arrival %d stage: %s"), Command.Hit, *Command.Command);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Test] log %s %d stage: %s"), GTestLogTokens[Command.Token].Name, Command.Hit,
						*Command.Command);
				}
				GEngine->Exec(World, *Command.Command);
			}
		}
		if (GArrivals.Waiting.IsEmpty() && GArrivals.Due.IsEmpty())
		{
			GArrivals.bTicking = false;
			UpdateTestLogListening();
			return false;
		}
		return true;
	}

	/** Queues one stage ("<s>:<command>;<s>:<command>") for the next hit of the token. */
	void QueueLogStage(const int32 Token, const TArray<FString>& StepArgs, const TCHAR* CommandName)
	{
		// The console split it at spaces, the stage splits at ';' and a step at its first ':'.
		TArray<FString> Steps;
		FString::Join(StepArgs, TEXT(" ")).ParseIntoArray(Steps, TEXT(";"));
		TArray<TPair<double, FString>> Stage;
		for (const FString& Step : Steps)
		{
			FString Seconds;
			FString Command;
			if (Step.Split(TEXT(":"), &Seconds, &Command) && Seconds.TrimStartAndEnd().IsNumeric() && !Command.TrimStartAndEnd().IsEmpty())
			{
				Stage.Emplace(FMath::Max(FCString::Atod(*Seconds.TrimStartAndEnd()), 0.0), Command.TrimStartAndEnd());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Test] %s: '%s' is not <seconds>:<command>"), CommandName, *Step);
			}
		}
		if (Stage.IsEmpty())
		{
			return;
		}
		const int32 Commands = Stage.Num();
		GArrivals.Waiting.Emplace(Token, MoveTemp(Stage));
		UpdateTestLogListening();
		if (!GArrivals.bTicking)
		{
			GArrivals.bTicking = true;
			GArrivals.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickArrivals), 0.1f);
		}
		const int32 Waiting = GArrivals.Waiting.FilterByPredicate([Token](const TPair<int32, TArray<TPair<double, FString>>>& Each)
		{
			return Each.Key == Token;
		}).Num();
		if (Token == TestLogArrived)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Test] arrival stage queued: %d command(s), %d stage(s) waiting, %d arrival(s) so far"),
				Commands, Waiting, GArrivals.HitCounts[Token]);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Test] log stage queued for %s: %d command(s), %d stage(s) waiting, %d hit(s) so far"),
				GTestLogTokens[Token].Name, Commands, Waiting, GArrivals.HitCounts[Token]);
		}
	}

	void TestOnArrival(const TArray<FString>& Args, UWorld*)
	{
		QueueLogStage(TestLogArrived, Args, TEXT("aps.Test.OnArrival"));
	}

	void TestOnLog(const TArray<FString>& Args, UWorld*)
	{
		int32 Token = INDEX_NONE;
		for (int32 Index = 0; !Args.IsEmpty() && Index < NumTestLogTokens; ++Index)
		{
			if (Args[0].Equals(GTestLogTokens[Index].Name, ESearchCase::IgnoreCase))
			{
				Token = Index;
			}
		}
		if (Token == INDEX_NONE || Args.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.OnLog <arrived|coursetake|standup> <s>:<command>[;...]: unknown token or no stage"));
			return;
		}
		QueueLogStage(Token, TArray<FString>(Args.GetData() + 1, Args.Num() - 1), TEXT("aps.Test.OnLog"));
	}

	bool TickAutopilotWatch(float)
	{
		if (!GAutopilotWatch.bActive)
		{
			return false;
		}
		ASpaceship* Ship = GAutopilotWatch.Ship.Get();
		const UWorld* World = Ship ? Ship->GetWorld() : nullptr;
		if (!World)
		{
			GAutopilotWatch.bActive = false;
			UpdateTestLogListening();
			return false;
		}
		const double Elapsed = World->GetTimeSeconds() - GAutopilotWatch.EngageSeconds;
		if (!GAutopilotWatch.bChecked && Elapsed >= 15.0)
		{
			GAutopilotWatch.bChecked = true;
			const double SpeedMps = Ship->GetKinematicVelocity().Size() / 100.0;
			const bool bEngaged = Ship->FlightModel && Ship->FlightModel->IsAutopilotEngaged();
			FString What;
			int32 ContactLines = 0;
			const bool bContact = GArrivals.Log && GArrivals.Log->LastContactOf(Ship->GetName(), What, ContactLines);
			// The approach layer's course star now (stage B; -1 none or aps.Stars.ApproachCourseMaxLy 0) beside it.
			APSGalaxyGpuStars::ECourseSource CourseSource = APSGalaxyGpuStars::ECourseSource::None;
			const int64 CourseStar = APSGalaxyGpuStars::GetCourseStar(World, &CourseSource);
			const TCHAR* CourseSourceText = CourseSource == APSGalaxyGpuStars::ECourseSource::Autopilot ? TEXT("ap")
				: CourseSource == APSGalaxyGpuStars::ECourseSource::Navigation ? TEXT("nav")
				: CourseSource == APSGalaxyGpuStars::ECourseSource::Boresight ? TEXT("bore") : TEXT("none");
			const FString Contacts = (bContact
				? FString::Printf(TEXT("to %s; last contact %s, %d contact line(s) since the engage"), *GAutopilotWatch.TargetName, *What, ContactLines)
				: FString::Printf(TEXT("to %s; no contact line since the engage"), *GAutopilotWatch.TargetName))
				+ FString::Printf(TEXT("; course star %lld (%s)"), static_cast<long long>(CourseStar), CourseSourceText);
			if (bEngaged && SpeedMps < 5.0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Test] autopilot stuck: %s at %.1f m/s %.0f s after the engage (%s)"), *Ship->GetName(),
					SpeedMps, Elapsed, *Contacts);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Test] autopilot moving: %s at %.1f m/s %.0f s after the engage (autopilot %s, %s)"),
					*Ship->GetName(), SpeedMps, Elapsed, bEngaged ? TEXT("on") : TEXT("off"), *Contacts);
			}
		}
		if (Elapsed >= 20.0)
		{
			GAutopilotWatch.bActive = false;
			UpdateTestLogListening();
			return false;
		}
		return true;
	}

	/** Starts (or restarts) the autopilot watchdog for an engage of this ship. */
	void ArmAutopilotWatch(ASpaceship& Ship, const AActor& Target)
	{
		const UWorld* World = Ship.GetWorld();
		if (!World)
		{
			return;
		}
		if (GAutopilotWatch.bActive)
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GAutopilotWatch.Ticker);
		}
		GAutopilotWatch.bActive = true;
		GAutopilotWatch.bChecked = false;
		GAutopilotWatch.Ship = &Ship;
		GAutopilotWatch.TargetName = Target.GetName();
		GAutopilotWatch.EngageSeconds = World->GetTimeSeconds();
		UpdateTestLogListening();
		if (GArrivals.Log)
		{
			GArrivals.Log->ResetContacts();
		}
		GAutopilotWatch.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickAutopilotWatch), 0.25f);
	}

	/** aps.Test.Steer: a pilot without navigation who keeps the nose on the traced star's dot. */
	struct FSteerPilot
	{
		bool bActive{false};
		double PeriodSeconds{0.5};
		double HandOverAu{0.0};
		bool bCarry{true};
		/** Rio 07.10 (sb-boresight-fix-x4 never arrived): the 5th argument, in steer periods (0 = none). Every frame the speed
		 * is held to the way left to the exact place over BrakePeriods x PeriodSeconds, so inside that many seconds of flight
		 * from the star the way left shrinks about e-fold per that time and no frame carries the ship through the hand-over
		 * sphere; near the star a drawn dot whose line misses that sphere gives way to the exact place. */
		double BrakePeriods{0.0};
		/** Logged once per 'on': the brake held the speed / the pilot left a missing drawn dot for the exact place. */
		bool bBrakeLogged{false};
		bool bExactLogged{false};
		double NextSeconds{0.0};
		double NextLogSeconds{0.0};
		FTSTicker::FDelegateHandle Ticker;
	};
	FSteerPilot GSteer;

	/** The traced star's drawn dot as the approach trace wrote it (aps.Test.SteerDrawnDir), while it is from the last frames. */
	bool SteerDrawnDirection(FVector& OutDirection)
	{
		TArray<FString> Parts;
		CVarSteerDrawnDir.GetValueOnGameThread().ParseIntoArrayWS(Parts);
		if (Parts.Num() != 4)
		{
			return false;
		}
		const uint64 Frame = FCString::Strtoui64(*Parts[0], nullptr, 10);
		OutDirection = FVector(FCString::Atod(*Parts[1]), FCString::Atod(*Parts[2]), FCString::Atod(*Parts[3]));
		return Frame + 3 >= GFrameCounter && OutDirection.Normalize();
	}

	bool TickSteer(float)
	{
		if (!GSteer.bActive)
		{
			return false;
		}
		ASpaceship* Ship = FindPilotedShip(nullptr);
		UWorld* World = Ship ? Ship->GetWorld() : nullptr;
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		if (!Controller)
		{
			return true;
		}
		const double Now = World->GetTimeSeconds();
		const bool bSteerTick = Now >= GSteer.NextSeconds;
		// Rio 07.10 (sb-boresight-fix-x4 never arrived: at 700-2000 ly/s the ship flew 10-30 ly a frame, through the 1000 AU
		// sphere between two 0.5 s ticks, and was turned back 180 deg on every tick): the hand-over distance and the brake are
		// looked at every frame; the aim, the carry and the log stay on the steer period. Without either nothing runs between ticks.
		if (!bSteerTick && GSteer.HandOverAu <= 0.0 && GSteer.BrakePeriods <= 0.0)
		{
			return true;
		}
		if (bSteerTick)
		{
			GSteer.NextSeconds = Now + GSteer.PeriodSeconds;
		}
		const int64 Traced = TracedCatalogIndex();
		// The registry name, only for the log lines (not every frame).
		const auto TracedName = [World, Traced]() -> FString
		{
			const FAPSStarSystems* Systems = APSStarSystemsFind(World);
			const int32 Registered = Systems && Traced != INDEX_NONE ? Systems->IndexOfGalaxyStar(Traced) : INDEX_NONE;
			const FAPSStarSystemInfo* Info = Systems && Registered != INDEX_NONE ? Systems->Get(Registered) : nullptr;
			return Info ? Info->Name : FString(TEXT("the traced star"));
		};
		FVector Exact = FVector::ZeroVector;
		if (!GalaxyStarExactPlace(World, Traced, Exact))
		{
			return true;
		}
		FVector Camera = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		Controller->GetPlayerViewPoint(Camera, ViewRotation);
		const double DistanceCm = FVector::Dist(Camera, Exact);
		if (GSteer.HandOverAu > 0.0 && DistanceCm <= GSteer.HandOverAu * TestAuCm)
		{
			// The last stretch as a pilot would fly it once there: the drive released, the autopilot to the same star.
			GSteer.bActive = false;
			UE_LOG(LogTemp, Log, TEXT("[APS.Test] steer: %.6g AU from the exact place of %s, the autopilot takes over"),
				DistanceCm / TestAuCm, *TracedName());
			GEngine->Exec(World, *FString::Printf(TEXT("aps.Ship.Drive 0 0 0 shotlabel=%s"), *GDrive.ShotLabel));
			GEngine->Exec(World, TEXT("aps.Test.Autopilot system traced notrace"));
			return false;
		}
		// The brake: per frame, because the flight model's power ramp (REAL SCALE STELLAR grows the speed up to e^2 a second,
		// the star drive follows its set speed in log space) rebuilds a cut speed within one steer period. Only the size of
		// the velocity changes; the drive keeps W held, so the ship never stalls short of the sphere.
		const FVector ShipVelocity = Ship->GetKinematicVelocity();
		double Speed = ShipVelocity.Size();
		const double BrakeSeconds = GSteer.BrakePeriods * FMath::Max(GSteer.PeriodSeconds, 0.1);
		const bool bBrakeOn = BrakeSeconds > 0.0;
		const double BrakeSpeed = bBrakeOn ? DistanceCm / BrakeSeconds : 0.0;
		if (bBrakeOn && Speed > BrakeSpeed)
		{
			if (!GSteer.bBrakeLogged)
			{
				GSteer.bBrakeLogged = true;
				UE_LOG(LogTemp, Log,
					TEXT("[APS.Test] steer brake f=%llu: %.6g ly from the exact place of %s at %.4g ly/s, the speed held to the way ")
					TEXT("left over %.2f s"),
					static_cast<unsigned long long>(GFrameCounter), DistanceCm / TestLightYearCm, *TracedName(),
					Speed / TestLightYearCm, BrakeSeconds);
			}
			FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, ShipVelocity * (BrakeSpeed / Speed));
			Speed = BrakeSpeed;
		}
		if (!bSteerTick)
		{
			return true;
		}
		FVector Drawn = FVector::ZeroVector;
		const bool bDrawnFresh = SteerDrawnDirection(Drawn);
		const FVector ToExact = (Exact - Camera).GetSafeNormal();
		// With the brake: a drawn dot whose line passes the exact place wider than half the hand-over distance would hold the
		// ship off the sphere for good (sb-boresight-fix-x4: the twin drawn a steady 0.047 ly, about 3000 AU, off the exact
		// place); within 20 such misses of the star the pilot flies at the exact place instead.
		const double DrawnAlong = bDrawnFresh ? FVector::DotProduct(Drawn, ToExact) : 1.0;
		const double DrawnMissCm = !bDrawnFresh ? 0.0
			: DrawnAlong > 0.0 ? DistanceCm * FVector::CrossProduct(Drawn, ToExact).Size() : DistanceCm;
		const bool bDrawnMisses = bBrakeOn && GSteer.HandOverAu > 0.0 && DrawnMissCm > 0.5 * GSteer.HandOverAu * TestAuCm
			&& DistanceCm < 20.0 * DrawnMissCm;
		if (bDrawnMisses && !GSteer.bExactLogged)
		{
			GSteer.bExactLogged = true;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Test] steer brake f=%llu: the drawn dot of %s passes %.6g AU from its exact place, %.6g ly away: ")
				TEXT("the pilot aims at the exact place"),
				static_cast<unsigned long long>(GFrameCounter), *TracedName(), DrawnMissCm / TestAuCm, DistanceCm / TestLightYearCm);
		}
		const bool bDrawn = bDrawnFresh && !bDrawnMisses;
		const FVector Direction = bDrawn ? Drawn : ToExact;
		const FVector OldForward = Ship->GetShipForwardVector().GetSafeNormal();
		FaceForTest(*Controller, *Ship, Direction, 0.0, false);
		if (GSteer.bCarry && Speed > 0.0)
		{
			// The pilot's correction is taken at once (the band model would bring the velocity round over a few seconds).
			FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, Direction * Speed);
		}
		if (Now >= GSteer.NextLogSeconds)
		{
			GSteer.NextLogSeconds = Now + 1.0;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Test] steer f=%llu at the %s of %s: %.6g ly, turned %.4f deg, the drawn dot %.4f deg off the exact place, ")
				TEXT("%.4g ly/s"),
				static_cast<unsigned long long>(GFrameCounter), bDrawn ? TEXT("drawn dot") : TEXT("exact place"),
				*TracedName(), DistanceCm / TestLightYearCm,
				FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(OldForward, Direction), -1.0, 1.0))),
				bDrawn ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Drawn, ToExact), -1.0, 1.0))) : -1.0,
				Speed / TestLightYearCm);
		}
		return true;
	}

	void TestSteer(const TArray<FString>& Args, UWorld*)
	{
		if (!Args.IsEmpty() && Args[0].Equals(TEXT("on"), ESearchCase::IgnoreCase))
		{
			GSteer.PeriodSeconds = Args.Num() > 1 ? FMath::Clamp(FCString::Atod(*Args[1]), 0.0, 10.0) : 0.5;
			GSteer.HandOverAu = Args.Num() > 2 ? FMath::Max(FCString::Atod(*Args[2]), 0.0) : 0.0;
			GSteer.bCarry = Args.Num() <= 3 || FCString::Atoi(*Args[3]) != 0;
			GSteer.BrakePeriods = Args.Num() > 4 ? FMath::Clamp(FCString::Atod(*Args[4]), 0.0, 100.0) : 0.0;
			GSteer.bBrakeLogged = false;
			GSteer.bExactLogged = false;
			GSteer.NextSeconds = 0.0;
			GSteer.NextLogSeconds = 0.0;
			if (!GSteer.bActive)
			{
				GSteer.bActive = true;
				GSteer.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickSteer), 0.0f);
			}
		}
		else if (GSteer.bActive)
		{
			GSteer.bActive = false;
			FTSTicker::GetCoreTicker().RemoveTicker(GSteer.Ticker);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] steer %s: every %.2f s, hand over at %.0f AU, carry %d, brake %.2f periods, drawn dot %s"),
			GSteer.bActive ? TEXT("on") : TEXT("off"), GSteer.PeriodSeconds, GSteer.HandOverAu, GSteer.bCarry ? 1 : 0,
			GSteer.BrakePeriods, CVarSteerDrawnDir.GetValueOnGameThread().IsEmpty() ? TEXT("not written yet") : TEXT("written"));
	}

	/**
	 * Rio 06.10 (star approach v2, stage B harness; the ctl stall): the ship off its pad, at rest, N km along its own up axis
	 * (an autopilot engaged on the HQ's pads toward a target behind the station stalls on them). The aps.Test.Pose back teleport.
	 */
	void UndockForTest(UWorld& World, APawn& Pawn, const double Km)
	{
		const UAPSWorldOriginSubsystem* OriginSubsystem = World.GetSubsystem<UAPSWorldOriginSubsystem>();
		const FVector Origin = OriginSubsystem ? OriginSubsystem->GetOriginOffset() : FVector(World.OriginLocation);
		const FVector From = Origin + Pawn.GetActorLocation();
		ASpaceship* UndockedShip = Cast<ASpaceship>(&Pawn);
		const FVector Up = (UndockedShip ? TestShipUpVector(*UndockedShip) : Pawn.GetActorUpVector()).GetSafeNormal(
			UE_DOUBLE_SMALL_NUMBER, Pawn.GetActorUpVector());
		if (UndockedShip)
		{
			FAPSShipFlightBenchmark::SetKinematicVelocity(*UndockedShip, FVector::ZeroVector);
		}
		Pawn.SetActorLocation(Pawn.GetActorLocation() + Up * Km * 1.0e5, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] undock: %.0f km up the ship's axis from %s"), Km, *From.ToCompactString());
	}

	void TestAutopilot(const TArray<FString>& Args, UWorld* World)
	{
		// Rio 04.10 checks: the piloted ship's autopilot to the nearest actor whose name (or a body's name) holds the filter;
		// "antipode" makes a target 100 km over the far side of the nearest world (the climb out from behind its horizon).
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		ASpaceship* Ship = Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr;
		if (!Ship || !Ship->FlightModel || Args.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.Autopilot <name|antipode>: pilot a ship first"));
			return;
		}
		AActor* Target = nullptr;
		if (Args[0].Equals(TEXT("antipode"), ESearchCase::IgnoreCase))
		{
			if (const APlanetaryBody* Near = FindPlanet(*Ship))
			{
				const FVector Centre = Near->GetActorLocation();
				const FVector Out = (Ship->GetActorLocation() - Centre).GetSafeNormal();
				FActorSpawnParameters Spawn;
				Spawn.ObjectFlags |= RF_Transient;
				Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Target = World->SpawnActor<ATargetPoint>(Centre - Out * (Near->GetWorldScapeBodyRadiusCm() + 1.0e7),
					FRotator::ZeroRotator, Spawn);
			}
		}
		else if (Args[0].Equals(TEXT("system"), ESearchCase::IgnoreCase))
		{
			// Rio 05.10 (real scale, stage 2): a catalogue system by name, or the nearest one outside the home (system nearest),
			// through its anchor (spawned on demand), as a fleet order or the HUD's nav target aims at it.
			if (FAPSStarSystems* Systems = APSStarSystemsFind(World))
			{
				const FString Name = Args.Num() > 1 ? Args[1] : FString(TEXT("nearest"));
				TArray<int32> Found;
				// Rio 06.10 (still ship): "system 40", the 40th nearest, for a long flight between the stars.
				const int32 Rank = Name.IsNumeric() ? FMath::Max(FCString::Atoi(*Name), 1) : 1;
				if (Name.Equals(TEXT("nearest"), ESearchCase::IgnoreCase) || Name.IsNumeric())
				{
					Systems->FindNearest(Ship->GetActorLocation(), Rank + 4, Found);
					Found.RemoveAll([Systems](const int32 Index)
					{
						const FAPSStarSystemInfo* Info = Systems->Get(Index);
						return !Info || Info->bHome || Info->bInsideHome;
					});
					if (Found.Num() >= Rank)
					{
						Found.RemoveAt(0, Rank - 1);
					}
					else if (!Found.IsEmpty())
					{
						Found.RemoveAt(0, Found.Num() - 1);
					}
				}
				else if (IsGalaxySelector(Name))
				{
					// Rio 06.10 (star approach harness): galaxyN, galaxyowedN, cat<index> or traced (FindGalaxyTarget).
					const int32 Picked = FindGalaxyTarget(World, Ship->GetActorLocation(), Name);
					if (Picked != INDEX_NONE)
					{
						Found.Add(Picked);
					}
				}
				else
				{
					Systems->Search(Name, 1, Found);
				}
				if (const FAPSStarSystemInfo* Info = Found.IsEmpty() ? nullptr : Systems->Get(Found[0]))
				{
					Target = Systems->GetAnchor(Info->Id);
					UE_LOG(LogTemp, Log, TEXT("[APS.Test] autopilot system %s (%s), %.4f ly from home, room %.0f AU"), *Info->Name,
						Info->GalaxyIndex != INDEX_NONE ? TEXT("galaxy") : TEXT("cluster"), Info->HomeDistanceCm / 9.4607304725808e17,
						Info->RoomCm / 1.495978707e13);
					// Rio 06.10 (star approach harness): a galaxy system becomes the approach trace's star unless "notrace"
					// follows; "surveyed" lets the civilization know it surveyed first (the screenshot 192 case).
					if (Info->GalaxyIndex != INDEX_NONE)
					{
						const auto HasWord = [&Args](const TCHAR* Word)
						{
							return Args.ContainsByPredicate([Word](const FString& Each) { return Each.Equals(Word, ESearchCase::IgnoreCase); });
						};
						if (HasWord(TEXT("surveyed")))
						{
							Systems->Learn(Info->Id, APSStars::EKnowledge::Surveyed, FText::FromString(TEXT("by a test run")));
						}
						AnnounceGalaxyTarget(World, *Info, Ship->GetActorLocation(), !HasWord(TEXT("notrace")));
					}
				}
			}
		}
		else
		{
			double BestSquared = TNumericLimits<double>::Max();
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				const ACelestialBody* Body = Cast<ACelestialBody>(Actor);
				if (!IsValid(Actor) || Actor == Ship || !(Actor->GetName().Contains(Args[0])
					|| (Body && !Body->AstroName.IsNone() && Body->AstroName.ToString().Contains(Args[0]))))
				{
					continue;
				}
				const double Squared = FVector::DistSquared(Actor->GetActorLocation(), Ship->GetActorLocation());
				if (Squared < BestSquared)
				{
					BestSquared = Squared;
					Target = Actor;
				}
			}
		}
		if (!Target)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.Autopilot: no target for %s"), *Args[0]);
			return;
		}
		// Rio 06.10 (star approach v2, stage B harness): "undock[=km]" lifts the ship off its pad first (default 20 km), and a
		// watchdog reports an autopilot that does not get going.
		double UndockKm = -1.0;
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			if (Args[Index].Equals(TEXT("undock"), ESearchCase::IgnoreCase))
			{
				UndockKm = 20.0;
			}
			else if (Args[Index].StartsWith(TEXT("undock="), ESearchCase::IgnoreCase))
			{
				UndockKm = FMath::Max(FCString::Atod(*Args[Index].RightChop(7)), 0.0);
			}
		}
		if (UndockKm >= 0.0)
		{
			UndockForTest(*World, *Ship, UndockKm);
		}
		Ship->FlightModel->EngageAutopilot(Target);
		ArmAutopilotWatch(*Ship, *Target);
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] autopilot to %s, %.0f km away"), *Target->GetName(),
			FVector::Dist(Target->GetActorLocation(), Ship->GetActorLocation()) / 1.0e5);
	}

	// Rio 07.10 ("buildings on the worlds of other systems must not vanish"): the materialized system's worlds get new names
	// in every generated world, so a run names them through this: %P = its first planet, %N = the system.
	void TestForeign(const TArray<FString>& Args, UWorld* World)
	{
		FAPSStarSystems* Systems = World ? APSStarSystemsFind(World) : nullptr;
		FAPSSystemMaterializer* Materializer = Systems ? Systems->GetMaterializer() : nullptr;
		const int32 Active = Materializer ? Materializer->GetActiveIndex() : INDEX_NONE;
		const FAPSStarSystemInfo* Info = Active != INDEX_NONE ? Systems->Get(Active) : nullptr;
		TArray<APlanet*> Planets;
		if (Materializer)
		{
			Materializer->GetPlanets(Planets);
		}
		APlanet* First = nullptr;
		for (APlanet* Planet : Planets)
		{
			if (IsValid(Planet) && !Planet->AstroName.IsNone())
			{
				First = Planet;
				break;
			}
		}
		if (!Info || !First || Args.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] foreign: no materialized system with a named planet (or no command); nothing run"));
			return;
		}
		FString Command = FString::Join(Args, TEXT(" "));
		Command.ReplaceInline(TEXT("%P"), *First->AstroName.ToString());
		Command.ReplaceInline(TEXT("%N"), *Info->Name);
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] foreign %s (planet %s): %s"), *Info->Name, *First->AstroName.ToString(), *Command);
		GEngine->Exec(World, *Command);
	}

	void TestDumpHome(UWorld* World)
	{
		if (!World) return;
		const auto Describe = [](const AActor* Actor)
		{
			const AActor* Parent = Actor->GetAttachParentActor();
			return FString::Printf(TEXT("%s loc=%s worldScale=%s relScale=%s parent=%s parentScale=%s"), *Actor->GetName(),
				*Actor->GetActorLocation().ToCompactString(), *Actor->GetActorScale3D().ToCompactString(),
				Actor->GetRootComponent() ? *Actor->GetRootComponent()->GetRelativeScale3D().ToCompactString() : TEXT("-"),
				*GetNameSafe(Parent), Parent ? *Parent->GetActorScale3D().ToCompactString() : TEXT("-"));
		};
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Test] generator %s preview=%d continuous=%d"), *Describe(*It),
				It->ActorHasTag(TEXT("WorldGenerationPreview")) ? 1 : 0, It->UsesContinuousPreviewFrame() ? 1 : 0);
		}
		for (TActorIterator<AStar> It(World); It; ++It)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Test] star %s radiusKm=%d"), *Describe(*It), It->StarRadiusKM);
		}
		for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		{
			const APlanet* Planet = Cast<APlanet>(*It);
			const AActor* Star = Planet && IsValid(Planet->ParentStar) ? Planet->ParentStar : nullptr;
			UE_LOG(LogTemp, Log, TEXT("[APS.Test] body %s radiusKm=%d bodyRadiusCm=%.4g toStarAU=%.3f stream=%d"), *Describe(*It),
				It->PlanetRadiusKM, It->GetWorldScapeBodyRadiusCm(),
				Star ? FVector::Dist(It->GetActorLocation(), Star->GetActorLocation()) / 1.495978707e13 : -1.0,
				It->bStreamWorldScapeSurface ? 1 : 0);
		}
	}

	/** aps.Test.Pose: the pawn's pose in absolute coordinates (the world origin may move between save and load). */
	struct FTestPose
	{
		bool bSaved{false};
		FVector Absolute{FVector::ZeroVector};
		FQuat Rotation{FQuat::Identity};
		FRotator Control{FRotator::ZeroRotator};
	};
	FTestPose GTestPose;

	/** Rio 04.10 ("jumped out of the ship and it vanished"): a walker looks at the nearest ship, to shoot its hull after the
	 * exit's world shift (aps.Test.Pose ship, and the exit run's later shots). */
	bool LookAtNearestShip(UWorld* World, APawn* Pawn, const bool bLog)
	{
		ACustomGravityCharacter* Walker = Cast<ACustomGravityCharacter>(Pawn);
		if (!World || !Walker)
		{
			return false;
		}
		const ASpaceship* Nearest = nullptr;
		double NearestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<ASpaceship> It(World); It; ++It)
		{
			const double Distance = FVector::DistSquared(It->GetActorLocation(), Walker->GetActorLocation());
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = *It;
			}
		}
		if (!Nearest)
		{
			return false;
		}
		// Up or down to it too: after a jump from a hovering ship it is overhead.
		const FVector Direction = (Nearest->GetActorLocation() - Walker->GetActorLocation()).GetSafeNormal();
		const double PitchUp = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(
			FVector::DotProduct(Direction, Walker->GetActorUpVector()), -1.0, 1.0)));
		Walker->SetViewDirection(Direction, static_cast<float>(FMath::Clamp(PitchUp, -80.0, 80.0)));
		if (bLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Test] looking at %s, %.1f m away"), *Nearest->GetName(), FMath::Sqrt(NearestDistance) / 100.0);
		}
		return true;
	}

	void TestPose(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn || Args.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.Pose save | load | turn <yaw deg>: no pawn or no argument"));
			return;
		}
		// Rio 05.10: the whole generation frame (the engine origin and the floating origin's double shifts), so a pose
		// saved before a far jump comes back to the same place after the world has shifted under it.
		const UAPSWorldOriginSubsystem* OriginSubsystem = World->GetSubsystem<UAPSWorldOriginSubsystem>();
		const FVector Origin = OriginSubsystem ? OriginSubsystem->GetOriginOffset() : FVector(World->OriginLocation);
		if (Args[0].Equals(TEXT("save"), ESearchCase::IgnoreCase))
		{
			GTestPose.bSaved = true;
			GTestPose.Absolute = Origin + Pawn->GetActorLocation();
			GTestPose.Rotation = Pawn->GetActorQuat();
			GTestPose.Control = Controller->GetControlRotation();
		}
		else if (Args[0].Equals(TEXT("load"), ESearchCase::IgnoreCase) && GTestPose.bSaved)
		{
			if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
			{
				FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
			}
			Pawn->SetActorLocationAndRotation(GTestPose.Absolute - Origin, GTestPose.Rotation, false, nullptr,
				ETeleportType::TeleportPhysics);
			Controller->SetControlRotation(GTestPose.Control);
		}
		else if (Args[0].Equals(TEXT("back"), ESearchCase::IgnoreCase) && Args.Num() > 1)
		{
			// Back along the view by N AU: the same target from further away (B7 shots).
			if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
			{
				FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
			}
			const FVector View = Controller->GetControlRotation().Vector();
			Pawn->SetActorLocation(Pawn->GetActorLocation() - View * FCString::Atod(*Args[1]) * 1.495978707e13, false, nullptr,
				ETeleportType::TeleportPhysics);
		}
		else if (Args[0].Equals(TEXT("up"), ESearchCase::IgnoreCase))
		{
			// Rio 06.10 (star approach v2, stage B harness): off the pad, at rest, N km (default 20) up the ship's own axis.
			UndockForTest(*World, *Pawn, Args.Num() > 1 ? FMath::Max(FCString::Atod(*Args[1]), 0.0) : 20.0);
		}
		else if (Args[0].Equals(TEXT("star"), ESearchCase::IgnoreCase) || Args[0].Equals(TEXT("system"), ESearchCase::IgnoreCase))
		{
			// Faces the nearest star actor (star [deg]) or a catalogue system's point (system <name> [deg]), turned aside
			// by N degrees so the hull does not hide it (B7 shots).
			const bool bSystem = Args[0].Equals(TEXT("system"), ESearchCase::IgnoreCase);
			FVector Target = FVector::ZeroVector;
			FString TargetName;
			int32 TargetRadiusKm = 0;
			bool bFound = false;
			if (bSystem)
			{
				const FAPSStarSystems* Systems = APSStarSystemsFind(World);
				TArray<int32> Found;
				if (Systems && Args.Num() > 1)
				{
					Systems->Search(Args[1], 1, Found);
				}
				if (const FAPSStarSystemInfo* Info = Systems && !Found.IsEmpty() ? Systems->Get(Found[0]) : nullptr)
				{
					Target = Info->Location;
					TargetName = Info->Name;
					bFound = true;
				}
			}
			else
			{
				double NearestDistance = TNumericLimits<double>::Max();
				for (TActorIterator<AStar> It(World); It; ++It)
				{
					const double Distance = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
					if (Distance < NearestDistance)
					{
						NearestDistance = Distance;
						Target = It->GetActorLocation();
						TargetName = It->GetName();
						TargetRadiusKm = It->StarRadiusKM;
						bFound = true;
					}
				}
			}
			if (bFound)
			{
				const int32 AsideArg = bSystem ? 2 : 1;
				const double Aside = Args.Num() > AsideArg ? FCString::Atod(*Args[AsideArg]) : 10.0;
				const FVector ToStar = (Target - Pawn->GetActorLocation()).GetSafeNormal();
				FVector Up = FVector::VectorPlaneProject(FVector::UpVector, ToStar).GetSafeNormal();
				if (Up.IsNearlyZero()) Up = FVector::VectorPlaneProject(FVector::ForwardVector, ToStar).GetSafeNormal();
				const FVector Forward = FQuat(Up, FMath::DegreesToRadians(-Aside)).RotateVector(ToStar);
				if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
				{
					FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
					Pawn->SetActorRotation(FAPSShipFlightBenchmark::GetRotationForFlightAxes(*Ship, Forward, Up),
						ETeleportType::TeleportPhysics);
				}
				Controller->SetControlRotation(Forward.Rotation());
				UE_LOG(LogTemp, Log, TEXT("[APS.Test] facing %s (%d km) %.0f deg aside, %.3f AU away"), *TargetName,
					TargetRadiusKm, Aside, FVector::Dist(Target, Pawn->GetActorLocation()) / 1.495978707e13);
			}
		}
		else if (Args[0].Equals(TEXT("goto"), ESearchCase::IgnoreCase) && Args.Num() > 1)
		{
			// Rio 05.10: the ship at rest this many km (goto <name> [km], default 20 000) from a catalogue system's point,
			// so the system materializes around it as on arrival (its star stands at that point).
			const FAPSStarSystems* Systems = APSStarSystemsFind(World);
			TArray<int32> Found;
			if (Systems)
			{
				Systems->Search(Args[1], 1, Found);
			}
			if (const FAPSStarSystemInfo* Info = Systems && !Found.IsEmpty() ? Systems->Get(Found[0]) : nullptr)
			{
				const double FromPointKm = Args.Num() > 2 ? FCString::Atod(*Args[2]) : 20000.0;
				const FVector Out = (Pawn->GetActorLocation() - Info->Location).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
				if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
				{
					FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
				}
				Pawn->SetActorLocation(Info->Location + Out * FromPointKm * 1.0e5, false, nullptr, ETeleportType::TeleportPhysics);
				Controller->SetControlRotation((-Out).Rotation());
				UE_LOG(LogTemp, Log, TEXT("[APS.Test] goto %s: %.0f km from its point"), *Info->Name, FromPointKm);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Test] goto: no system matches %s"), *Args[1]);
			}
		}
		else if (Args[0].Equals(TEXT("into"), ESearchCase::IgnoreCase))
		{
			// Rio 05.10 (the autopilot stopped inside a star and the game froze at 2.7 s a frame): the ship at rest this
			// many km from the nearest star's centre (into [km], default half its radius), facing that centre.
			const AStar* Nearest = nullptr;
			double NearestDistance = TNumericLimits<double>::Max();
			for (TActorIterator<AStar> It(World); It; ++It)
			{
				const double Distance = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
				if (IsValid(*It) && Distance < NearestDistance)
				{
					NearestDistance = Distance;
					Nearest = *It;
				}
			}
			if (Nearest)
			{
				const double RadiusKm = Nearest->RadiusKM > 0.0 ? Nearest->RadiusKM : static_cast<double>(Nearest->StarRadiusKM);
				const double FromCentreKm = Args.Num() > 1 ? FCString::Atod(*Args[1]) : 0.5 * RadiusKm;
				const FVector Centre = Nearest->GetActorLocation();
				const FVector Out = (Pawn->GetActorLocation() - Centre).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
				if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
				{
					FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
				}
				Pawn->SetActorLocation(Centre + Out * FromCentreKm * 1.0e5, false, nullptr, ETeleportType::TeleportPhysics);
				Controller->SetControlRotation((-Out).Rotation());
				UE_LOG(LogTemp, Log, TEXT("[APS.Test] into %s: %.0f km from its centre (radius %.0f km)"), *Nearest->GetName(),
					FromCentreKm, RadiusKm);
			}
		}
		else if (Args[0].Equals(TEXT("galaxy"), ESearchCase::IgnoreCase) && Args.Num() > 1)
		{
			// Rio 06.10 (star approach harness, a pilot without navigation): stops the ship and faces a registered galaxy
			// system's star where the sky draws it (its exact catalogue place, not the registry's), N degrees aside (0: dead
			// ahead, where the hull may hide it): galaxy <galaxyN | galaxyowedN | cat<index> | traced | name> [deg]. It becomes
			// the traced star.
			const FAPSStarSystems* Systems = APSStarSystemsFind(World);
			int32 Picked = INDEX_NONE;
			if (Systems && IsGalaxySelector(Args[1]))
			{
				Picked = FindGalaxyTarget(World, Pawn->GetActorLocation(), Args[1]);
			}
			else if (Systems)
			{
				TArray<int32> Named;
				Systems->Search(Args[1], 1, Named);
				Picked = Named.IsEmpty() ? INDEX_NONE : Named[0];
			}
			const FAPSStarSystemInfo* Info = Systems && Picked != INDEX_NONE ? Systems->Get(Picked) : nullptr;
			FVector Exact = FVector::ZeroVector;
			if (Info && Info->GalaxyIndex != INDEX_NONE && GalaxyStarExactPlace(World, Info->GalaxyIndex, Exact))
			{
				const double Aside = Args.Num() > 2 ? FCString::Atod(*Args[2]) : 0.0;
				FaceForTest(*Controller, *Pawn, Exact - Pawn->GetActorLocation(), Aside, true);
				AnnounceGalaxyTarget(World, *Info, Pawn->GetActorLocation(), true);
				UE_LOG(LogTemp, Log, TEXT("[APS.Test] facing %s at its exact place %.0f deg aside, %.6g ly away"), *Info->Name, Aside,
					FVector::Dist(Exact, Pawn->GetActorLocation()) / TestLightYearCm);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Test] pose galaxy: no registered galaxy system for %s"), *Args[1]);
			}
		}
		else if (Args[0].Equals(TEXT("disc"), ESearchCase::IgnoreCase) && Args.Num() > 1)
		{
			// Rio 06.10 (star approach harness, the glyph <-> sphere band): stops the ship where the nearest star's disc is N px
			// as the far glyphs measure it (its radius over the distance from its sky place times the view's pixel tangent),
			// facing it deg aside (default 8): disc <px> [deg]. The camera arm (metres) is left out at these distances.
			const AStar* Nearest = nullptr;
			double NearestSquared = TNumericLimits<double>::Max();
			for (TActorIterator<AStar> It(World); It; ++It)
			{
				const double Squared = IsValid(*It)
					? FVector::DistSquared(UAPSWorldOriginSubsystem::SkyPlace(**It), Pawn->GetActorLocation()) : TNumericLimits<double>::Max();
				if (Squared < NearestSquared)
				{
					NearestSquared = Squared;
					Nearest = *It;
				}
			}
			if (Nearest)
			{
				int32 Width = 0;
				int32 Height = 0;
				Controller->GetViewportSize(Width, Height);
				const double Fov = Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetFOVAngle() : 90.0;
				const double PixelTangent = APSStellarViewOptics::PixelTangent(Controller,
					2.0 * FMath::Tan(FMath::DegreesToRadians(Fov * 0.5)) / FMath::Max(Width, 320));
				const double Pixels = FMath::Max(FCString::Atod(*Args[1]), 0.01);
				const double RadiusCm = FMath::Max(static_cast<double>(Nearest->StarRadiusKM), 1.0) * 100000.0;
				const FVector StarInSky = UAPSWorldOriginSubsystem::SkyPlace(*Nearest);
				const double DistanceCm = RadiusCm / FMath::Max(Pixels * PixelTangent, 1.0e-12);
				const FVector Out = (Pawn->GetActorLocation() - StarInSky).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
				if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
				{
					FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
				}
				Pawn->SetActorLocation(StarInSky + Out * DistanceCm, false, nullptr, ETeleportType::TeleportPhysics);
				const double Aside = Args.Num() > 2 ? FCString::Atod(*Args[2]) : 8.0;
				FaceForTest(*Controller, *Pawn, StarInSky - Pawn->GetActorLocation(), Aside, true);
				UE_LOG(LogTemp, Log,
					TEXT("[APS.Test] pose disc %.3f px: %s (%s, %d km) at %.6g AU from its sky place (pixel tangent %.6g), %.0f deg aside"),
					Pixels, *Nearest->AstroName.ToString().Replace(TEXT(" "), TEXT("_")), *Nearest->GetName(), Nearest->StarRadiusKM,
					DistanceCm / TestAuCm, PixelTangent, Aside);
			}
		}
		else if (Args[0].Equals(TEXT("turn"), ESearchCase::IgnoreCase) && Args.Num() > 1)
		{
			// About the pawn's own up: the hull and the view turn together.
			const double Degrees = FCString::Atod(*Args[1]);
			const FQuat Turn(Pawn->GetActorUpVector(), FMath::DegreesToRadians(Degrees));
			Pawn->SetActorRotation(Turn * Pawn->GetActorQuat(), ETeleportType::TeleportPhysics);
			Controller->SetControlRotation((Turn * Controller->GetControlRotation().Quaternion()).Rotator());
		}
		else if (Args[0].Equals(TEXT("ship"), ESearchCase::IgnoreCase))
		{
			LookAtNearestShip(World, Pawn, true);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] pose %s: %s at %s"), *Args[0], *Pawn->GetName(),
			*(Origin + Pawn->GetActorLocation()).ToCompactString());
	}

	void TestBuild(const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		ASpaceship* Ship = Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr;
		UAPSShipBuildComponent* Build = Ship ? Ship->FindComponentByClass<UAPSShipBuildComponent>() : nullptr;
		if (!Build || Args.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Test] aps.Test.Build: no piloted ship with a build component, or no verb"));
			return;
		}
		const FString& Verb = Args[0];
		const double Value = Args.Num() > 1 ? FCString::Atod(*Args[1]) : 0.0;
		if (Verb.Equals(TEXT("toggle"), ESearchCase::IgnoreCase))
		{
			Build->Toggle();
		}
		FAPSConstructionMode* Mode = Build->GetMode();
		bool bPlaced = false;
		if (Mode)
		{
			if (Verb.Equals(TEXT("slot"), ESearchCase::IgnoreCase)) Mode->SelectSlot(static_cast<int32>(Value));
			else if (Verb.Equals(TEXT("section"), ESearchCase::IgnoreCase)) Mode->ToggleSection();
			else if (Verb.Equals(TEXT("place"), ESearchCase::IgnoreCase)) bPlaced = Mode->Place();
			else if (Verb.Equals(TEXT("turn"), ESearchCase::IgnoreCase)) Mode->Rotate(static_cast<float>(Value));
			else if (Verb.Equals(TEXT("tilt"), ESearchCase::IgnoreCase)) Mode->RotatePitch(static_cast<float>(Value));
			else if (Verb.Equals(TEXT("far"), ESearchCase::IgnoreCase)) Mode->AdjustDistance(static_cast<float>(Value));
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Test] build %s: building=%d selection=%s placed=%d status=%s"), *Verb,
			Build->IsBuilding() ? 1 : 0, Mode ? *Mode->GetSelectedId().ToString() : TEXT("-"), bPlaced ? 1 : 0,
			Mode ? *Mode->GetStatus().ToString() : *Build->GetHintText());
	}

	FAutoConsoleCommandWithWorldAndArgs TestBuildCommand(TEXT("aps.Test.Build"),
		TEXT("Test runs: aps.Test.Build toggle | slot N | section | place | turn D | tilt D | far S (the piloted ship's orbital build mode)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestBuild));

	FAutoConsoleCommandWithWorldAndArgs TestPoseCommand(TEXT("aps.Test.Pose"),
		TEXT("Test runs: aps.Test.Pose save | load | turn <yaw deg> | back <AU> | up [km=20] | star [deg aside] | galaxy <galaxyN|")
		TEXT("galaxyowedN|cat<index>|far<ly>|traced|name> [deg aside] | disc <px> [deg aside] (the pawn's pose, for before/after shots ")
		TEXT("and the star approach runs)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestPose));

	FAutoConsoleCommandWithWorldAndArgs TestForeignCommand(TEXT("aps.Test.Foreign"),
		TEXT("Test runs: aps.Test.Foreign <console command>: runs it with %P = the materialized system's first planet, %N = that ")
		TEXT("system (aps.Test.Foreign aps.Mega.Raise SpaceHub %P)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestForeign));
	FAutoConsoleCommandWithWorld TestDumpHomeCommand(TEXT("aps.Test.DumpHome"),
		TEXT("Test runs: logs the home system's hierarchy (locations, scales, parents)."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&TestDumpHome));

	FAutoConsoleCommandWithWorld TestMapCommand(TEXT("aps.Test.Map"),
		TEXT("Test runs: toggles the F10 strategic map."), FConsoleCommandWithWorldDelegate::CreateStatic(&TestMap));
	FAutoConsoleCommandWithWorld TestBuildModeCommand(TEXT("aps.Test.BuildMode"),
		TEXT("Test runs: toggles build mode of the player on foot."), FConsoleCommandWithWorldDelegate::CreateStatic(&TestBuildMode));
	FAutoConsoleCommandWithWorldAndArgs TestWalkCommand(TEXT("aps.Test.Walk"),
		TEXT("Test runs: aps.Test.Walk [warmup=30] [duration=30] [sprint=0|1] [zerog=0|1] [quit]: once the player's gravity character ")
		TEXT("stands in L_WorldGeneration, after the warm-up it walks straight on at pace 3 (sprint held, or weightless with G) and ")
		TEXT("writes each frame's dt, walker and camera place to Saved/Diagnostics/Walk."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestWalk));
	FAutoConsoleCommandWithWorldAndArgs TestKeyCommand(TEXT("aps.Test.Key"),
		TEXT("Test runs: aps.Test.Key <Key>: presses and releases a key through Slate, as the player would (G, Y, M...)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestKey));
	FAutoConsoleCommandWithWorldAndArgs TestAutopilotCommand(TEXT("aps.Test.Autopilot"),
		TEXT("Test runs: aps.Test.Autopilot <name|antipode|system <name|nearest|N|galaxyN|galaxyowedN|cat<index>|far<ly>|traced>> ")
		TEXT("[notrace] [surveyed] [undock[=km]]: the piloted ship's autopilot to the nearest actor so named, to a point 100 km ")
		TEXT("over the far side of the nearest world, or to a catalogue system; undock lifts the ship off its pad first. A ")
		TEXT("watchdog logs the ship's speed 15 s after the engage ('autopilot stuck' under 5 m/s)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestAutopilot));
	FAutoConsoleCommandWithWorldAndArgs TestShotCommand(TEXT("aps.Test.Shot"),
		TEXT("Test runs: aps.Test.Shot <name>: a screenshot with the UI to Saved/Screenshots/ShipDrive/<shotlabel>_t<name>.png."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestShot));
	FAutoConsoleCommandWithWorldAndArgs TestWatchRegistrationsCommand(TEXT("aps.Test.WatchRegistrations"),
		TEXT("Test runs (star approach harness): aps.Test.WatchRegistrations [1|0]: logs every galaxy system as it is registered, ")
		TEXT("its registry place against its exact place (AE) and whether the owed travel grew in that frame (in flight)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestWatchRegistrations));
	FAutoConsoleCommandWithWorldAndArgs TestOnArrivalCommand(TEXT("aps.Test.OnArrival"),
		TEXT("Test runs (star approach harness): aps.Test.OnArrival <s>:<command>[;<s>:<command>...]: queues one stage; the ")
		TEXT("piloted ship's next autopilot arrival (its 'off: arrived' line) runs the stage's commands that many world seconds later."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestOnArrival));
	FAutoConsoleCommandWithWorldAndArgs TestOnLogCommand(TEXT("aps.Test.OnLog"),
		TEXT("Test runs (star approach harness): aps.Test.OnLog <arrived|coursetake|standup> <s>:<command>[;<s>:<command>...]: queues ")
		TEXT("one stage; the next matching log line (arrived: the piloted ship's autopilot arrival; coursetake: the course star's far ")
		TEXT("take; standup: a system stands) runs its commands that many world seconds later. aps.Test.OnArrival is OnLog arrived."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestOnLog));
	FAutoConsoleCommandWithWorldAndArgs TestSteerCommand(TEXT("aps.Test.Steer"),
		TEXT("Test runs (star approach harness): aps.Test.Steer on [period s=0.5] [hand over AU=0] [carry 1|0] [brake periods=0] | off: ")
		TEXT("a pilot without navigation keeps the nose on the traced star's drawn dot (aps.Test.SteerDrawnDir, else its exact place) ")
		TEXT("every period; within the hand-over distance (checked every frame) the drive is released and the autopilot finishes ")
		TEXT("(aps.Test.Autopilot system traced). brake N: every frame the speed is held to the way left over N periods, so the ship ")
		TEXT("closes on the star instead of flying through the hand-over sphere, and near it a drawn dot that misses the sphere ")
		TEXT("gives way to the exact place."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestSteer));

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
			// Rio 06.10 (packaged build): IsNaniteEnabled reads the editor-only build settings; a cooked game asks whether the
			// mesh carries Nanite data.
			if (const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
#if WITH_EDITORONLY_DATA
				Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->IsNaniteEnabled())
#else
				Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->HasValidNaniteData())
#endif
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
