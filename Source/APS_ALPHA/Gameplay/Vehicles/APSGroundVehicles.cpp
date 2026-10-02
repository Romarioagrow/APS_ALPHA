#include "APSGroundVehicles.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModule.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Engine/AssetManager.h"
#include "Engine/Level.h"
#include "Engine/OverlapResult.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

namespace APSGroundVehiclesLocal
{
	TAutoConsoleVariable<int32> CVarSpawn(
		TEXT("aps.Vehicles.Spawn"), 1,
		TEXT("1: once the colony base stands, a rover, a hover and a drone park beside it (missing ones only; existing ones stay). 0: none."));

	constexpr EAPSGroundVehicleKind Kinds[] = {
		EAPSGroundVehicleKind::Rover, EAPSGroundVehicleKind::Hover, EAPSGroundVehicleKind::Drone};
	constexpr int32 KindCount = UE_ARRAY_COUNT(Kinds);
	/** The motor pool: the three in a row this far apart (centre to centre), noses away from the base. */
	constexpr double SlotSpacingCm = 700.0;
	/** Footprints for the spawn service, X toward the base and Y across: the whole pool, and one vehicle. */
	const FVector PoolSizeCm(650.0, 2.0 * SlotSpacingCm + 450.0, 400.0);
	const FVector SingleSizeCm(650.0, 450.0, 400.0);
	constexpr double PoolSpacingCm = 350.0;
	constexpr double PoolReachCm = 2500.0;
	/** The spawn service's direction order from 157.5 degrees: behind the base, away from its landing pad and from the
	 * sides where the colony's modules go first. */
	constexpr int32 PoolSeed = 9;
	constexpr double CheckSeconds = 2.0;
	constexpr double SettleSeconds = 0.5;
	constexpr double ClearSeconds = 3.0;
	constexpr double MoveAsideSeconds = 30.0;
	constexpr double RetrySpotDelaySeconds = 30.0;
	/** The vehicles' meshes, loaded in the background before the first spawn so it does not hitch. */
	const TCHAR* const PreloadPaths[] = {
		TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Body.SM_Offroad_Body"),
		TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Tire.SM_Offroad_Tire"),
		TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_P1_06/SM_Spaceship_P1_06.SM_Spaceship_P1_06"),
		TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_P1_05/SM_Spaceship_P1_05.SM_Spaceship_P1_05")};
	constexpr double PreloadTimeoutSeconds = 30.0;

	/** One game world's vehicles and timers. */
	struct FWorldVehicles
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AActor> Base;
		TWeakObjectPtr<APlanetaryBody> Body;
		TWeakObjectPtr<ASpaceship> Vehicles[KindCount];
		bool bSettled[KindCount]{false, false, false};
		/** World seconds of each vehicle's last move aside: at most one a MoveAsideSeconds, so nothing ping-pongs. */
		double MovedSeconds[KindCount]{-1000.0, -1000.0, -1000.0};
		/** After a search found no free spot (the surface loaded), the next one waits until then (world seconds). */
		double RetrySpotSeconds{-1.0};
		double CheckClock{CheckSeconds};
		double SettleClock{0.0};
		double ClearClock{0.0};
		TSharedPtr<FStreamableHandle> Preload;
		double PreloadStartSeconds{-1.0};
		bool bPreloaded{false};
		bool bWaitLogged{false};
		bool bFailureLogged{false};
	};

	TArray<FWorldVehicles>& Worlds()
	{
		static TArray<FWorldVehicles> Entries;
		return Entries;
	}

	/** A world being torn down takes its entry (and any preload handle) with it, long before static destruction. */
	void ForgetWorld(UWorld* World, bool /*bSessionEnded*/, bool /*bCleanupResources*/)
	{
		Worlds().RemoveAll([World](const FWorldVehicles& Entry)
		{
			return !Entry.World.IsValid() || Entry.World.Get() == World;
		});
	}

	FWorldVehicles& StateFor(UWorld* World)
	{
		static bool bCleanupBound = false;
		if (!bCleanupBound)
		{
			bCleanupBound = true;
			FWorldDelegates::OnWorldCleanup.AddStatic(&ForgetWorld);
		}
		TArray<FWorldVehicles>& Entries = Worlds();
		for (FWorldVehicles& Entry : Entries)
		{
			if (Entry.World.Get() == World)
			{
				return Entry;
			}
		}
		Entries.RemoveAll([](const FWorldVehicles& Entry) { return !Entry.World.IsValid(); });
		FWorldVehicles& Added = Entries.AddDefaulted_GetRef();
		Added.World = World;
		return Added;
	}

	/** C19 (Rio 02.10): the colony's vehicles chosen in the generation menu, bits in Kinds order (1 rover, 2 hover, 4 drone). */
	bool IsWanted(const FWorldVehicles& State, const int32 Index)
	{
		const UWorld* World = State.World.Get();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
		const int32 Mask = Gameplay && IsValid(Gameplay->SpawnParameters) ? Gameplay->SpawnParameters->GroundVehicleMask : 7;
		return (Mask & (1 << Index)) != 0;
	}

	bool HasMissing(const FWorldVehicles& State)
	{
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			if (!State.Vehicles[Index].IsValid() && IsWanted(State, Index))
			{
				return true;
			}
		}
		return false;
	}

	/** The body's own terrain and foliage collision: owned by the body through its WorldScape root. */
	bool IsOwnedByWorld(const AActor* Actor, const AActor* Body)
	{
		for (const AActor* Current = Actor; IsValid(Current); Current = Current->GetOwner())
		{
			if (Current == Body)
			{
				return true;
			}
		}
		return false;
	}

	/** The colony base (the construction service's surface site, else the manifest's base by name) and its world. */
	bool FindColony(UWorld* World, FWorldVehicles& State)
	{
		if (State.Base.IsValid() && State.Body.IsValid())
		{
			return true;
		}
		AActor* Base = nullptr;
		if (const UAPSColonyConstructionSubsystem* Construction = World->GetSubsystem<UAPSColonyConstructionSubsystem>())
		{
			Base = Construction->GetSiteAnchor(EAPSSpawnSite::Surface);
		}
		if (!IsValid(Base) && World->PersistentLevel)
		{
			const UAPSCivilizationMaterializationSubsystem* Colony = World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>();
			const FAPSCivilizationManifestEntity* Entity = Colony && Colony->IsMaterializationComplete()
				? Colony->GetRuntimeManifest().FindEntity(EAPSCivilizationEntityRole::BaseModule) : nullptr;
			if (Entity)
			{
				Base = FindObject<AActor>(World->PersistentLevel, *Entity->MakeDeterministicActorName().ToString());
			}
		}
		if (!IsValid(Base) || !Base->ActorHasTag(TEXT("APS.Civilization.Materialized")))
		{
			return false;
		}
		const UAPSCivilizationIdentityComponent* Identity = Base->FindComponentByClass<UAPSCivilizationIdentityComponent>();
		APlanetaryBody* Body = Cast<APlanetaryBody>(Base->GetAttachParentActor());
		if (!Identity || Identity->Role != EAPSCivilizationEntityRole::BaseModule || !Body)
		{
			return false;
		}
		State.Base = Base;
		State.Body = Body;
		return true;
	}

	/** The vehicles that exist (after a load, or spawned earlier in this world): the world's ships, only while one is missing. */
	void RefreshVehicles(UWorld* World, FWorldVehicles& State)
	{
		if (!HasMissing(State))
		{
			return;
		}
		for (TActorIterator<ASpaceship> It(World); It; ++It)
		{
			ASpaceship* Ship = *It;
			if (!IsValid(Ship) || !Ship->IsGroundVehicle())
			{
				continue;
			}
			for (int32 Index = 0; Index < KindCount; ++Index)
			{
				if (Ship->GetGroundVehicleKind() == Kinds[Index] && !State.Vehicles[Index].IsValid())
				{
					State.Vehicles[Index] = Ship;
					State.bSettled[Index] = false;
				}
			}
		}
	}

	/** The meshes are loaded in the background first; after the timeout the spawn loads what is left itself. */
	bool AssetsReady(FWorldVehicles& State)
	{
		if (State.bPreloaded)
		{
			return true;
		}
		const double Now = FPlatformTime::Seconds();
		if (State.PreloadStartSeconds < 0.0)
		{
			State.PreloadStartSeconds = Now;
			if (UAssetManager::GetIfInitialized())
			{
				TArray<FSoftObjectPath> Paths;
				for (const TCHAR* Path : PreloadPaths)
				{
					Paths.Emplace(Path);
				}
				State.Preload = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate(),
					FStreamableManager::DefaultAsyncLoadPriority, false, false, TEXT("APS.GroundVehicles"));
			}
		}
		State.bPreloaded = !State.Preload.IsValid() || State.Preload->HasLoadCompleted() || State.Preload->WasCanceled()
			|| Now - State.PreloadStartSeconds > PreloadTimeoutSeconds;
		return State.bPreloaded;
	}

	/** What a new spot keeps clear of: the colony's pad and parked ship, its built modules, the other vehicles. */
	void CollectObstacles(UWorld* World, const FWorldVehicles& State, FAPSSpawnRequest& Request, const AActor* Except)
	{
		if (const UAPSColonyConstructionSubsystem* Construction = World->GetSubsystem<UAPSColonyConstructionSubsystem>())
		{
			TArray<AAPSColonyModule*> Modules;
			Construction->GetBuiltModules(EAPSSpawnSite::Surface, Modules);
			for (AAPSColonyModule* Module : Modules)
			{
				Request.Obstacles.Add(Module);
			}
		}
		// Once per placement (not per frame): the colony's materialized actors on the body, the pad's walk kept free.
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor) || Actor == Request.Anchor || !Actor->ActorHasTag(TEXT("APS.Civilization.Materialized"))
				|| Actor->GetAttachParentActor() != State.Body.Get())
			{
				continue;
			}
			Request.Obstacles.Add(Actor);
			const UAPSCivilizationIdentityComponent* Identity = Actor->FindComponentByClass<UAPSCivilizationIdentityComponent>();
			if (Identity && Identity->Role == EAPSCivilizationEntityRole::LandingPad)
			{
				Request.RouteTargets.Add(Actor);
			}
		}
		for (const TWeakObjectPtr<ASpaceship>& Vehicle : State.Vehicles)
		{
			if (Vehicle.IsValid() && Vehicle.Get() != Except)
			{
				Request.Obstacles.Add(Vehicle.Get());
			}
		}
	}

	/** The world's WorldScape surface is loaded and current: only then can a spot beside the base be judged. */
	bool IsSurfaceLoaded(const APlanetaryBody* Body)
	{
		const APlanetarySurfaceGenerator* Surface = Body ? Body->PlanetaryEnvironmentGenerator : nullptr;
		const AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		return IsValid(Root) && IsValid(Root->WorldScapeNoise) && Root->PlanetScale > 0.0
			&& Surface->IsSurfaceProfileCurrent(Body);
	}

	/** A dry, gentle spot of this size beside the base; a wider, steeper search if the first rings are full. */
	bool ResolveSite(UWorld* World, FWorldVehicles& State, const FVector& SizeCm, FAPSSpawnPlacement& OutPlacement,
		const AActor* Except = nullptr, AActor* ExtraObstacle = nullptr)
	{
		const UAPSSpawnPlacementSubsystem* Spawner = World->GetSubsystem<UAPSSpawnPlacementSubsystem>();
		if (!Spawner || !State.Base.IsValid() || !State.Body.IsValid())
		{
			return false;
		}
		if (!IsSurfaceLoaded(State.Body.Get()))
		{
			// The surface beside the colony is not loaded (the pilot is far from it): try again later.
			if (!State.bWaitLogged)
			{
				State.bWaitLogged = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] waiting for the surface beside %s to load"), *GetNameSafe(State.Base.Get()));
			}
			return false;
		}
		FAPSSpawnRequest Request;
		Request.Site = EAPSSpawnSite::Surface;
		Request.Anchor = State.Base.Get();
		Request.Body = State.Body.Get();
		Request.SizeCm = SizeCm;
		Request.SpacingCm = PoolSpacingCm;
		Request.MaximumReachCm = PoolReachCm;
		Request.MaximumSlope = 0.2;
		Request.MinimumDryMarginCm = 300.0;
		Request.Seed = PoolSeed;
		CollectObstacles(World, State, Request, Except);
		if (ExtraObstacle)
		{
			Request.Obstacles.Add(ExtraObstacle);
		}
		if (Spawner->ResolvePlacement(Request, OutPlacement))
		{
			return true;
		}
		if (!OutPlacement.bSiteReady)
		{
			// The surface beside the colony is not loaded (the pilot is far from it): try again later.
			if (!State.bWaitLogged)
			{
				State.bWaitLogged = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] waiting for the surface beside %s to load"), *GetNameSafe(State.Base.Get()));
			}
			return false;
		}
		Request.MaximumSlope = 0.35;
		Request.SpacingCm = 250.0;
		Request.MaximumReachCm = 6000.0;
		if (Spawner->ResolvePlacement(Request, OutPlacement))
		{
			return true;
		}
		// Nothing fits now (a crowded or steep site): the next search waits a while; the colony may change meanwhile.
		State.RetrySpotSeconds = World->GetTimeSeconds() + RetrySpotDelaySeconds;
		if (!State.bFailureLogged)
		{
			State.bFailureLogged = true;
			UE_LOG(LogTemp, Warning, TEXT("[APS.Vehicles] no free spot beside %s for the vehicles (%s); trying again later"),
				*GetNameSafe(State.Base.Get()), *OutPlacement.FailureCode.ToString());
		}
		return false;
	}

	/** The gravity up at a point of the colony's world. */
	FVector UpAt(const FWorldVehicles& State, const FVector& Location)
	{
		const APlanetaryBody* Body = State.Body.Get();
		const FVector Up = Body ? (Location - Body->GetActorLocation()).GetSafeNormal() : FVector::ZeroVector;
		return Up.IsNearlyZero() ? FVector::UpVector : Up;
	}

	/** Spawns one kind at a spot, nose along Heading, and sets it on the ground (collision where it exists). */
	ASpaceship* SpawnAt(UWorld* World, FWorldVehicles& State, const int32 Index, const FVector& Location,
		const FVector& Heading)
	{
		const FVector Up = UpAt(State, Location);
		const FVector Nose = FVector::VectorPlaneProject(Heading, Up).GetSafeNormal();
		ASpaceship* Vehicle = APSGroundVehicles::SpawnVehicle(World, Kinds[Index], Location,
			Nose.IsNearlyZero() ? FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal() : Nose, Up,
			State.Body.Get());
		if (!Vehicle)
		{
			return nullptr;
		}
		State.Vehicles[Index] = Vehicle;
		State.bSettled[Index] = Vehicle->FlightModel && Vehicle->FlightModel->SettleVehicle(false);
		return Vehicle;
	}

	/** Parks the missing vehicles: the whole pool in one spot when all three are missing, else each in its own spot. */
	void SpawnMissing(UWorld* World, FWorldVehicles& State)
	{
		int32 Missing = 0;
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			Missing += State.Vehicles[Index].IsValid() || !IsWanted(State, Index) ? 0 : 1;
		}
		if (Missing == 0)
		{
			return;
		}
		const FVector BaseLocation = State.Base->GetActorLocation();
		if (Missing == KindCount)
		{
			FAPSSpawnPlacement Pool;
			if (!ResolveSite(World, State, PoolSizeCm, Pool))
			{
				return;
			}
			// The pool's X faces the base: the vehicles stand along its Y, noses out, ready to drive away.
			const FVector Along = Pool.Transform.GetUnitAxis(EAxis::Y);
			const FVector Out = -Pool.Transform.GetUnitAxis(EAxis::X);
			for (int32 Index = 0; Index < KindCount; ++Index)
			{
				SpawnAt(World, State, Index, Pool.Transform.GetLocation() + Along * ((Index - 1) * SlotSpacingCm), Out);
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] the colony's ROVER, HOVER and DRONE park beside %s, %.0f m from it (slope %.2f)"),
				*GetNameSafe(State.Base.Get()), FVector::Distance(Pool.Transform.GetLocation(), BaseLocation) / 100.0, Pool.Slope);
		}
		else
		{
			for (int32 Index = 0; Index < KindCount; ++Index)
			{
				if (State.Vehicles[Index].IsValid() || !IsWanted(State, Index))
				{
					continue;
				}
				FAPSSpawnPlacement Spot;
				if (!ResolveSite(World, State, SingleSizeCm, Spot))
				{
					return;
				}
				SpawnAt(World, State, Index, Spot.Transform.GetLocation(), -Spot.Transform.GetUnitAxis(EAxis::X));
				UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] the colony's %s parks beside %s, %.0f m from it"),
					APSGroundVehicle::KindName(Kinds[Index]), *GetNameSafe(State.Base.Get()),
					FVector::Distance(Spot.Transform.GetLocation(), BaseLocation) / 100.0);
			}
		}
		if (!HasMissing(State))
		{
			// The vehicles' meshes now hold the assets.
			State.Preload.Reset();
		}
	}

	/** Parked vehicles placed on the WorldScape height settle on the collision once it exists near them. */
	void SettleParked(FWorldVehicles& State)
	{
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			ASpaceship* Vehicle = State.Vehicles[Index].Get();
			if (!Vehicle || State.bSettled[Index])
			{
				continue;
			}
			if (IsValid(Vehicle->Pilot) || Vehicle->IsActorTickEnabled())
			{
				// Driven: it comes to rest where its driver leaves it.
				State.bSettled[Index] = true;
				continue;
			}
			if (Vehicle->FlightModel && Vehicle->FlightModel->SettleVehicle(true))
			{
				State.bSettled[Index] = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] %s settled on the ground"), *Vehicle->GetGroundVehicleName());
			}
		}
	}

	/** What stands in a parked vehicle now (a module built over it, a ship landed on it), besides its own ground. */
	AActor* FindBlocker(UWorld* World, const ASpaceship* Vehicle, const APlanetaryBody* Body)
	{
		const FBox Local = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Vehicle);
		if (!Local.IsValid)
		{
			return nullptr;
		}
		const FTransform Transform = Vehicle->GetActorTransform();
		// The upper part of its body, a little inside it: what it rests on or brushes against does not count.
		const FVector Extent = Local.GetExtent() * Transform.GetScale3D().GetAbs();
		const FVector QueryExtent(FMath::Max(Extent.X * 0.8, 10.0), FMath::Max(Extent.Y * 0.8, 10.0),
			FMath::Max(Extent.Z * 0.45, 10.0));
		const FVector QueryCenter = Transform.TransformPosition(Local.GetCenter() + FVector(0.0, 0.0, Local.GetExtent().Z * 0.35));
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSVehicleClearance), false, Vehicle);
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(Overlaps, QueryCenter, Transform.GetRotation(), ECC_WorldDynamic,
			FCollisionShape::MakeBox(QueryExtent), Params);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Actor = Overlap.GetActor();
			// Its world's terrain and foliage, and people walking by, do not count.
			if (Overlap.bBlockingHit && IsValid(Actor) && Actor != Vehicle && !IsOwnedByWorld(Actor, Body)
				&& !Actor->IsA<ACharacter>() && !Actor->IsA<AWorldScapeRoot>() && !Actor->IsA<APlanetarySurfaceGenerator>())
			{
				return Actor;
			}
		}
		return nullptr;
	}

	/** A parked vehicle something now stands in moves aside to a free spot beside the base. */
	void KeepClear(UWorld* World, FWorldVehicles& State)
	{
		if (!State.Base.IsValid() || !State.Body.IsValid())
		{
			return;
		}
		const double Now = World->GetTimeSeconds();
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			ASpaceship* Vehicle = State.Vehicles[Index].Get();
			if (!Vehicle || IsValid(Vehicle->Pilot) || Vehicle->IsActorTickEnabled()
				|| Now - State.MovedSeconds[Index] < MoveAsideSeconds)
			{
				continue;
			}
			AActor* Blocker = FindBlocker(World, Vehicle, State.Body.Get());
			if (!Blocker)
			{
				continue;
			}
			// One try (and its scan of the colony) per vehicle in MoveAsideSeconds, found or not.
			State.MovedSeconds[Index] = Now;
			FAPSSpawnPlacement Spot;
			if (!ResolveSite(World, State, SingleSizeCm, Spot, Vehicle, Blocker))
			{
				continue;
			}
			const FVector Location = Spot.Transform.GetLocation();
			const FVector Up = UpAt(State, Location);
			const FVector Heading = FVector::VectorPlaneProject(-Spot.Transform.GetUnitAxis(EAxis::X), Up).GetSafeNormal();
			if (Heading.IsNearlyZero())
			{
				continue;
			}
			Vehicle->SetActorLocationAndRotation(Location, Vehicle->GetActorRotationForFlightAxes(Heading, Up), false,
				nullptr, ETeleportType::TeleportPhysics);
			State.bSettled[Index] = Vehicle->FlightModel && Vehicle->FlightModel->SettleVehicle(false);
			UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] %s moved aside: %s stands where it was parked"),
				*Vehicle->GetGroundVehicleName(), *GetNameSafe(Blocker));
		}
	}

	/** aps.Vehicles.Bring: the parked vehicles line up beside the player, wherever on the colony's world (tests). */
	void Bring(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		FWorldVehicles& State = StateFor(World);
		FindColony(World, State);
		RefreshVehicles(World, State);
		const APlayerController* Player = World->GetFirstPlayerController();
		const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (!Pawn || !State.Body.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Vehicles] aps.Vehicles.Bring: no player pawn or no colony world"));
			return;
		}
		const FVector Up = UpAt(State, Pawn->GetActorLocation());
		FVector Forward = FVector::VectorPlaneProject(Pawn->GetActorForwardVector(), Up).GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			Forward = FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal();
		}
		const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
		int32 Moved = 0;
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			ASpaceship* Vehicle = State.Vehicles[Index].Get();
			if (!Vehicle || Vehicle == Pawn || IsValid(Vehicle->Pilot) || Vehicle->IsActorTickEnabled())
			{
				continue;
			}
			const FVector Location = Pawn->GetActorLocation() + Right * 1200.0 + Forward * ((Index - 1) * SlotSpacingCm)
				+ Up * 300.0;
			const FVector SlotUp = UpAt(State, Location);
			Vehicle->SetActorLocationAndRotation(Location, Vehicle->GetActorRotationForFlightAxes(
				FVector::VectorPlaneProject(Forward, SlotUp).GetSafeNormal(), SlotUp), false, nullptr, ETeleportType::TeleportPhysics);
			State.bSettled[Index] = Vehicle->FlightModel && Vehicle->FlightModel->SettleVehicle(false);
			++Moved;
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] aps.Vehicles.Bring: %d vehicle(s) beside the player"), Moved);
	}

	void List(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		FWorldVehicles& State = StateFor(World);
		RefreshVehicles(World, State);
		UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] colony base %s on %s"), *GetNameSafe(State.Base.Get()), *GetNameSafe(State.Body.Get()));
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			const ASpaceship* Vehicle = State.Vehicles[Index].Get();
			UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles]   %s: %s%s%s%s, %.0f m from the base"), APSGroundVehicle::KindName(Kinds[Index]),
				Vehicle ? *Vehicle->GetName() : IsWanted(State, Index) ? TEXT("missing") : TEXT("not chosen in the generation menu"),
				State.bSettled[Index] ? TEXT(", settled") : TEXT(""),
				Vehicle && IsValid(Vehicle->Pilot) ? TEXT(", piloted") : TEXT(""),
				Vehicle && Vehicle->IsActorTickEnabled() ? TEXT(", running") : TEXT(""),
				Vehicle && State.Base.IsValid() ? FVector::Distance(Vehicle->GetActorLocation(), State.Base->GetActorLocation()) / 100.0 : -1.0);
		}
	}

	FAutoConsoleCommandWithWorld BringCommand(
		TEXT("aps.Vehicles.Bring"),
		TEXT("Lines the colony's parked rover, hover and drone up beside the player (tests; same world as the colony)."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Bring));
	FAutoConsoleCommandWithWorld ListCommand(
		TEXT("aps.Vehicles.List"),
		TEXT("Logs the colony's ground vehicles: where they are, settled, driven."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&List));
}

ASpaceship* APSGroundVehicles::SpawnVehicle(UWorld* World, const EAPSGroundVehicleKind Kind, const FVector& Location,
	const FVector& Heading, const FVector& Up, APlanetaryBody* Body)
{
	if (!World || Kind == EAPSGroundVehicleKind::None)
	{
		return nullptr;
	}
	// Named ROVER, HOVER, DRONE: the boarding prompt shows the actor's name, and a save made in the driver's seat finds
	// the vehicle again by it (another one of the kind gets a suffix).
	FActorSpawnParameters Params;
	Params.Name = FName(APSGroundVehicle::KindName(Kind));
	Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	ASpaceship* Vehicle = World->SpawnActor<ASpaceship>(ASpaceship::StaticClass(), FTransform(Location), Params);
	if (!Vehicle)
	{
		return nullptr;
	}
	Vehicle->ConfigureAsGroundVehicle(Kind);
	Vehicle->SetGroundVehicleHomeBody(Body);
	Vehicle->FinishSpawning(FTransform(Vehicle->GetActorRotationForFlightAxes(Heading, Up), Location));
	if (IsValid(Body))
	{
		// Like the colony's base and pad: attached to its world.
		Vehicle->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
	}
	return Vehicle;
}

bool APSGroundVehicles::AreParked(UWorld* World)
{
	using namespace APSGroundVehiclesLocal;
	if (!World || CVarSpawn.GetValueOnGameThread() == 0)
	{
		return true;
	}
	const FWorldVehicles& State = StateFor(World);
	if (!State.Base.IsValid())
	{
		return false;
	}
	for (int32 Index = 0; Index < KindCount; ++Index)
	{
		if (IsWanted(State, Index) && (!State.Vehicles[Index].IsValid() || !State.bSettled[Index]))
		{
			return false;
		}
	}
	return true;
}

void APSGroundVehicles::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSGroundVehiclesLocal;
	if (!World || CVarSpawn.GetValueOnGameThread() == 0)
	{
		return;
	}
	FWorldVehicles& State = StateFor(World);
	State.CheckClock += DeltaSeconds;
	State.SettleClock += DeltaSeconds;
	State.ClearClock += DeltaSeconds;
	if (State.CheckClock >= CheckSeconds)
	{
		State.CheckClock = 0.0;
		if (FindColony(World, State))
		{
			RefreshVehicles(World, State);
			if (HasMissing(State) && World->GetTimeSeconds() >= State.RetrySpotSeconds && AssetsReady(State))
			{
				SpawnMissing(World, State);
			}
		}
	}
	if (State.SettleClock >= SettleSeconds)
	{
		State.SettleClock = 0.0;
		SettleParked(State);
	}
	if (State.ClearClock >= ClearSeconds)
	{
		State.ClearClock = 0.0;
		KeepClear(World, State);
	}
}
