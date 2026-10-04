#include "APSWorldOriginSubsystem.h"
#include "APSWorldShiftEvents.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Rendering/APSRenderSafety.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Pawns/Spectator/APSSpectatorPawn.h"
#include "AI/NavigationSystemBase.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DelayedAutoRegister.h"
#include "PhysicsField/PhysicsFieldComponent.h"
#include "SceneInterface.h"
#include "Tickable.h"
#include "UObject/UObjectHash.h"
#include "WorldScapeRoot.h"

namespace APSWorldOrigin
{
	TAutoConsoleVariable<int32> CVarRebaseOnSpawn(
		TEXT("aps.WorldOrigin.RebaseOnSpawn"), 1,
		TEXT("1 shifts the whole world so the player's spawn point becomes 0,0,0 (engine world origin). ")
		TEXT("0 keeps the generation frame with the headquarters at 0,0,0."));

	TAutoConsoleVariable<float> CVarMinShiftMeters(
		TEXT("aps.WorldOrigin.MinShiftMeters"), 100.0f,
		TEXT("No shift when the target is already closer to 0,0,0 than this, m."));

	/** UWorld::OriginLocation is an FIntVector in cm: about 21 474 km per axis. */
	constexpr double MaximumOriginCm = 2147000000.0;

	TAutoConsoleVariable<int32> CVarFloat(
		TEXT("aps.WorldOrigin.Float"), 1,
		TEXT("1: far from 0,0,0 the world shifts back under the player at a calm moment, in double precision past the ")
		TEXT("engine origin's 21 000 km, so physics holds anywhere (Rio 03.10). 0: off."));
	TAutoConsoleVariable<float> CVarFloatShipKm(
		TEXT("aps.WorldOrigin.FloatShipKm"), 50.0f,
		TEXT("A piloted ship slower than aps.WorldOrigin.FloatCalmKmPerS this far from 0,0,0 shifts the world, km."));
	TAutoConsoleVariable<float> CVarFloatWalkKm(
		TEXT("aps.WorldOrigin.FloatWalkKm"), 20.0f,
		TEXT("On foot or in a ground vehicle, this far from 0,0,0 shifts the world, km."));
	TAutoConsoleVariable<float> CVarFloatCalmKmPerS(
		TEXT("aps.WorldOrigin.FloatCalmKmPerS"), 3.0f,
		TEXT("A ship counts as calm (a shift is invisible and worth it) below this speed, km/s."));
	TAutoConsoleVariable<float> CVarFloatLeadSeconds(
		TEXT("aps.WorldOrigin.FloatLeadSeconds"), 4.0f,
		TEXT("Rio 03.10 (another star's planets flickered, their air came and went): a ship faster than calm shifts the ")
		TEXT("world too once it is farther from 0,0,0 than it flies in this many seconds (and aps.WorldOrigin.FloatShipKm), ")
		TEXT("so it never reaches a planet tens of AU out. 0: a fast ship never shifts (the old rule)."));
	TAutoConsoleVariable<int32> CVarMapShift(
		TEXT("aps.WorldOrigin.MapShift"), 1,
		TEXT("Rio 04.10 (from ~900 AU the home planet's layers slid apart on the map): 1 brings the strategic map's close ")
		TEXT("view of something far (beyond 50 view distances and 1 AU from 0,0,0) near the origin, while the pilot flies a ")
		TEXT("ship in open space; the pilot is the origin again as the map closes. 0: the map never shifts the world."));

	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World())
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	APawn* PlayerPawn(const UWorld* World)
	{
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		return PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	FAutoConsoleCommandWithWorld RebaseHereCommand(
		TEXT("aps.WorldOrigin.RebaseHere"),
		TEXT("Shifts the world so the player pawn becomes 0,0,0."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UWorld* GameWorld = FindGameWorld(World);
			APawn* Pawn = PlayerPawn(GameWorld);
			UAPSWorldOriginSubsystem* Origin = GameWorld ? GameWorld->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
			if (Pawn && Origin)
			{
				Origin->RebaseOnto(Pawn->GetActorLocation(), TEXT("console"));
			}
		}));

	/** Runs the floating origin once a frame for the game or PIE world, after the actors moved and before it is drawn. */
	class FFloatingOriginTicker final : public FTickableGameObject
	{
	public:
		virtual TStatId GetStatId() const override
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAPSFloatingOrigin, STATGROUP_Tickables);
		}
		virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
		// PIE worlds live in the editor process: tick there too (only game and PIE worlds are looked at).
		virtual bool IsTickableInEditor() const override { return true; }

		virtual void Tick(const float DeltaTime) override
		{
			if (!GEngine)
			{
				return;
			}
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
				{
					if (UAPSWorldOriginSubsystem* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>())
					{
						Origin->UpdateFloatingOrigin(DeltaTime);
					}
				}
			}
		}
	};

	TUniquePtr<FFloatingOriginTicker> GFloatingOriginTicker;

	FDelayedAutoRegisterHelper GFloatingOriginRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		GFloatingOriginTicker = MakeUnique<FFloatingOriginTicker>();
	});

	FAutoConsoleCommandWithWorld ReportCommand(
		TEXT("aps.WorldOrigin.Report"),
		TEXT("Logs the engine world origin and how far the player pawn is from 0,0,0."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UWorld* GameWorld = FindGameWorld(World);
			const APawn* Pawn = PlayerPawn(GameWorld);
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] report origin=%s pawn=%s (%.3f km from 0,0,0)"),
				GameWorld ? *GameWorld->OriginLocation.ToString() : TEXT("none"),
				Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("none"),
				Pawn ? Pawn->GetActorLocation().Size() / 100000.0 : -1.0);
		}));
}

bool UAPSWorldOriginSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FVector UAPSWorldOriginSubsystem::GetOriginOffset() const
{
	const UWorld* World = GetWorld();
	return World ? FVector(World->OriginLocation) + FloatOrigin : FloatOrigin;
}

bool UAPSWorldOriginSubsystem::ShiftWorldBy(const FVector& Offset, const TCHAR* Reason)
{
	UWorld* World = GetWorld();
	if (!World || !World->Scene || Offset.IsNearlyZero() || World->IsVisibilityRequestPending())
	{
		return false;
	}
	const APawn* Pawn = APSWorldOrigin::PlayerPawn(World);
	const FVector PawnBefore = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const double StartSeconds = FPlatformTime::Seconds();
	// The virtual shadow map cache cannot pan by such a jump (an int32 page offset): off from this very frame.
	APSRenderSafety::MarkCameraJump(TEXT("floating origin shift"));

	// UWorld::SetNewWorldOrigin's own steps with a double offset; its FIntVector origin is left as it is. The physics
	// scene follows its bodies' components (Chaos does not shift itself: FPhysScene::SupportsOriginShifting is false).
	World->OriginOffsetThisFrame = Offset;
	World->Scene->ApplyWorldOffset(Offset);
	for (ULevel* Level : World->GetLevels())
	{
		if (Level && (Level->bIsVisible || Level->IsPersistentLevel()))
		{
			Level->ApplyWorldOffset(Offset, true);
		}
	}
	if (UNavigationSystemBase* Navigation = World->GetNavigationSystem())
	{
		Navigation->ApplyWorldOffset(Offset, true);
	}
	TArray<UObject*> Children;
	GetObjectsWithOuter(World, Children, false);
	for (UObject* Child : Children)
	{
		UActorComponent* Component = Cast<UActorComponent>(Child);
		if (Component && !Component->GetOwner())
		{
			Component->ApplyWorldOffset(Offset, true);
		}
	}
	if (World->PhysicsField)
	{
		World->PhysicsField->ApplyWorldOffset(Offset, true);
	}
	// Rio 03.10 (a 1.6 s freeze on the first shift of every session): the scene moves each proxy's transform, but an
	// instanced mesh keeps its instance buffer's primitive transform until the component tells its instance manager,
	// which a world shift skips (ApplyWorldOffset bypasses OnUpdateTransform). The next GPUScene upload then failed
	// "InstanceSceneDataBuffers->GetPrimitiveToRelativeWorld().Equals(PrimitiveToWorld)" (GPUScene.cpp:367). Resend them
	// the way a normal move does; physics already followed the components above.
	// Rio 03.10 (still one GPUScene.cpp:367 ensure, a 1.2 s freeze, and two frames of mismatch on the first large shift,
	// s2-stars run): a hierarchical one (the star catalogues) runs UE 5.4's legacy instance path, whose proxy only takes a
	// new primitive transform from a rebuilt render state; it is re-created (a catalogue of 25k stars, ~1 ms).
	int32 InstancedMeshes = 0;
	for (TObjectIterator<UInstancedStaticMeshComponent> It; It; ++It)
	{
		UInstancedStaticMeshComponent* Instanced = *It;
		if (IsValid(Instanced) && Instanced->GetWorld() == World && Instanced->IsRegistered()
			&& Instanced->GetInstanceCount() > 0)
		{
			Instanced->UpdateComponentTransform(EUpdateTransformFlags::SkipPhysicsUpdate, ETeleportType::TeleportPhysics);
			if (Instanced->IsA<UHierarchicalInstancedStaticMeshComponent>() && Instanced->IsRenderStateCreated())
			{
				Instanced->MarkRenderStateDirty();
			}
			++InstancedMeshes;
		}
	}
	// Rio 03.10 (the same GPUScene.cpp:367 mismatch on some shifts, primitives other than instanced meshes): UE 5.4's spline
	// mesh proxies (the orbit lines, BP_OrbiteSpline) keep an instance buffer too. Re-created, there are only a few dozen.
	for (TObjectIterator<USplineMeshComponent> It; It; ++It)
	{
		USplineMeshComponent* Spline = *It;
		if (IsValid(Spline) && Spline->GetWorld() == World && Spline->IsRegistered() && Spline->IsRenderStateCreated())
		{
			Spline->MarkRenderStateDirty();
			++InstancedMeshes;
		}
	}
	// Rio 04.10 ("jumped out of the ship and it vanished", only its glass, ramp and props stayed): UE 5.4's scene culling
	// files a Nanite primitive into its grid cell when it is added or moves. A shift moves the proxies without either (the
	// resent transform is skipped as redundant), so a parked Nanite hull stayed culled by its old cell 38 km away. Its
	// render state is re-created, which files it again; the non-Nanite parts never used the cells.
	int32 NaniteMeshes = 0;
	for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
	{
		UStaticMeshComponent* Mesh = *It;
		if (IsValid(Mesh) && Mesh->GetWorld() == World && Mesh->IsRegistered() && Mesh->IsRenderStateCreated()
			&& !Mesh->bDisallowNanite && !Mesh->bForceDisableNanite && Mesh->HasValidNaniteData())
		{
			Mesh->MarkRenderStateDirty();
			++NaniteMeshes;
		}
	}
	// Absolute world positions kept outside the components: WorldScape's observer override (re-read every tick, but a
	// stale frame would rebuild the surface) and the flight model's fixed star centres.
	for (TActorIterator<AWorldScapeRoot> It(World); It; ++It)
	{
		It->OverridedPlayerPosition += Offset;
	}
	for (TActorIterator<ASpaceship> It(World); It; ++It)
	{
		if (It->FlightModel)
		{
			It->FlightModel->ApplyWorldShift(Offset);
		}
	}
	FloatOrigin -= Offset;
	// ApplyWorldOffset bypasses TransformUpdated; the engine's integer-origin
	// event does not run on this double-precision path. Refresh material/cloud
	// uniforms only after every actor and the camera have reached the new frame.
	APSWorldShiftEvents::OnPostDoubleShift().Broadcast(World, Offset);
	SinceFloatShiftSeconds = 0.0;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldOrigin] float shift reason=%s by %.3f km | pawn %.3f km -> %.2f m from 0,0,0 | generation offset %s | %d instanced and spline meshes resent, %d Nanite meshes refiled | %.1f ms"),
		Reason, Offset.Size() / 100000.0, PawnBefore.Size() / 100000.0,
		Pawn ? Pawn->GetActorLocation().Size() / 100.0 : -1.0, *GetOriginOffset().ToCompactString(), InstancedMeshes,
		NaniteMeshes, (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
	return true;
}

void UAPSWorldOriginSubsystem::UpdateFloatingOrigin(const float DeltaSeconds)
{
	SinceFloatShiftSeconds += DeltaSeconds;
	UWorld* World = GetWorld();
	APawn* Pawn = World ? APSWorldOrigin::PlayerPawn(World) : nullptr;
	// Rio 04.10 (the ship's walls vanished as the pilot got up, the planet showing through them): the pilot getting up
	// dropped the limit from FloatShipKm to FloatWalkKm, so a ship 20-50 km out shifted the world on the very frame the
	// camera moved inside the hull, and that view reused the outside view's occlusion and history. A new pawn waits the
	// same two seconds as after a shift.
	if (Pawn != FloatAnchorPawn.Get())
	{
		FloatAnchorPawn = Pawn;
		SinceFloatShiftSeconds = 0.0;
	}
	// Rio 04.10 ("from ~900 AU the home planet's layers slide apart on the map"): the map's view of something far from
	// 0,0,0 brings it near the origin (RequestMapView), and once the map closes the pilot is the anchor again at once.
	const AGravityPlayerController* MapController = Pawn ? Cast<AGravityPlayerController>(Pawn->GetController()) : nullptr;
	const bool bMapOpen = MapController && MapController->IsStrategicMapOpen();
	const bool bMapRequested = bMapRequest;
	bMapRequest = false;
	if (World && Pawn && APSWorldOrigin::CVarFloat.GetValueOnGameThread() != 0 && !bRebaseDeferred)
	{
		if (bMapOpen && bMapRequested)
		{
			TryMapShift();
		}
		else if (!bMapOpen && bMapShifted)
		{
			const FVector PilotLocation = Pawn->GetActorLocation();
			if (PilotLocation.IsNearlyZero(100.0) || ShiftWorldBy(-PilotLocation, TEXT("back to the pilot after the map")))
			{
				bMapShifted = false;
				ReleaseMapHold();
				return;
			}
		}
	}
	if (!World || APSWorldOrigin::CVarFloat.GetValueOnGameThread() == 0 || bRebaseDeferred || SinceFloatShiftSeconds < 2.0)
	{
		return;
	}
	if (!Pawn)
	{
		return;
	}
	const FVector Location = Pawn->GetActorLocation();
	const ASpaceship* Ship = Cast<ASpaceship>(Pawn);
	// Rio 03.10: the Generate Space free-flight camera (AU/s and faster) shifts like a ship, only when calm.
	const bool bFreeFlight = Pawn->IsA<AAPSSpectatorPawn>();
	const bool bFlying = (Ship && !Ship->IsGroundVehicle()) || bFreeFlight;
	// Someone walking a ship's decks keeps the ship's limit: the ship does not move under them, nothing grows far.
	const AActor* Carrier = Pawn->GetAttachParentActor();
	const bool bAboard = !bFlying && Carrier && Carrier->IsA<ASpaceship>();
	const double LimitCm = (bFlying || bAboard ? APSWorldOrigin::CVarFloatShipKm : APSWorldOrigin::CVarFloatWalkKm)
		.GetValueOnGameThread() * 1.0e5;
	if (Location.SizeSquared() <= FMath::Square(LimitCm))
	{
		return;
	}
	// A calm ship shifts past FloatShipKm. A fast one used never to shift: Rio's 03.10 flight to another star's planet
	// stayed above 3 km/s down to 44 km over the surface, so its WorldScape surface, air and clouds were drawn 4.5e14 cm
	// (30 AU) from 0,0,0 for half a minute, where float materials and planet centres lose kilometres (the flicker).
	// Now a fast ship shifts once it is farther out than it flies in FloatLeadSeconds: every few seconds at most.
	const double SpeedCm = Ship ? FMath::Max(Ship->GetKinematicVelocity().Size(), Ship->GetVelocity().Size())
		: Pawn->GetVelocity().Size();
	const bool bFast = bFlying && SpeedCm > APSWorldOrigin::CVarFloatCalmKmPerS.GetValueOnGameThread() * 1.0e5;
	if (bFast)
	{
		const double LeadSeconds = APSWorldOrigin::CVarFloatLeadSeconds.GetValueOnGameThread();
		if (LeadSeconds <= 0.0 || Location.SizeSquared() <= FMath::Square(FMath::Max(LimitCm, SpeedCm * LeadSeconds)))
		{
			return;
		}
	}
	// Not under the strategic map's own view, and not before the star catalogue has settled in its frame.
	const AGravityPlayerController* Controller = Cast<AGravityPlayerController>(Pawn->GetController());
	if ((Controller && Controller->IsStrategicMapOpen()) || IsStellarCatalogueSettling())
	{
		return;
	}
	ShiftWorldBy(-Location, bFreeFlight ? (bFast ? TEXT("a fast free-flight camera far out") : TEXT("a calm free-flight camera far out"))
		: bFlying ? (bFast ? TEXT("a fast ship far out") : TEXT("a slow ship far out")) : TEXT("on foot far out"));
}

void UAPSWorldOriginSubsystem::RequestMapView(const FVector& FocusLocation, const double ViewDistanceCm)
{
	bMapRequest = true;
	MapFocusLocation = FocusLocation;
	MapViewDistanceCm = ViewDistanceCm;
}

bool UAPSWorldOriginSubsystem::TryMapShift()
{
	UWorld* World = GetWorld();
	constexpr double AstronomicalUnitCm = 1.495978707e13;
	// Near enough already (the view is wide, or close to 0,0,0), or a shift a moment ago.
	if (!World || APSWorldOrigin::CVarMapShift.GetValueOnGameThread() == 0 || SinceFloatShiftSeconds < 1.0
		|| MapFocusLocation.SizeSquared() <= FMath::Square(FMath::Max(MapViewDistanceCm * 50.0, AstronomicalUnitCm))
		|| IsStellarCatalogueSettling())
	{
		return false;
	}
	// The world keeps running under the map, and the pilot ends up as far from 0,0,0 as the view was, beyond the engine's
	// large-world range. A ship in open space flies on (nothing near to collide with or stand on: its autopilot keeps
	// going); a pilot on foot, landed, or near a world, moon or station holds still until the map closes (Rio 04.10:
	// "the map opens, no problems, no switches"). The keys go to the map meanwhile anyway.
	APawn* Pawn = APSWorldOrigin::PlayerPawn(World);
	const ASpaceship* Ship = Cast<ASpaceship>(Pawn);
	bool bPilotClear = Ship && !Ship->IsGroundVehicle() && !Ship->GetAttachParentActor();
	if (bPilotClear)
	{
		const FVector ShipLocation = Ship->GetActorLocation();
		for (TActorIterator<APlanetaryBody> It(World); It && bPilotClear; ++It)
		{
			bPilotClear = !IsValid(*It) || FVector::Dist(ShipLocation, It->GetActorLocation()) - It->GetWorldScapeBodyRadiusCm() >= 1.0e10;
		}
		for (TActorIterator<ASpaceStation> It(World); It && bPilotClear; ++It)
		{
			bPilotClear = !IsValid(*It) || FVector::DistSquared(ShipLocation, It->GetActorLocation()) >= FMath::Square(1.0e8);
		}
	}
	if (!ShiftWorldBy(-MapFocusLocation, TEXT("the strategic map's view far out")))
	{
		return false;
	}
	bMapShifted = true;
	if (!bPilotClear && Pawn && MapHeldTicks.IsEmpty())
	{
		// Every ticking part of the pilot stops (movement, gravity, flight), exactly those, restored as the map closes.
		MapHeldPawn = Pawn;
		if (Pawn->IsActorTickEnabled())
		{
			MapHeldTicks.Add(Pawn);
			Pawn->SetActorTickEnabled(false);
		}
		for (UActorComponent* Component : Pawn->GetComponents())
		{
			if (IsValid(Component) && Component->IsComponentTickEnabled())
			{
				MapHeldTicks.Add(Component);
				Component->SetComponentTickEnabled(false);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] %s holds still while the map looks far out (%d ticks paused)"),
			*Pawn->GetName(), MapHeldTicks.Num());
	}
	return true;
}

void UAPSWorldOriginSubsystem::ReleaseMapHold()
{
	for (const TWeakObjectPtr<UObject>& Held : MapHeldTicks)
	{
		if (AActor* Actor = Cast<AActor>(Held.Get()))
		{
			Actor->SetActorTickEnabled(true);
		}
		else if (UActorComponent* Component = Cast<UActorComponent>(Held.Get()))
		{
			Component->SetComponentTickEnabled(true);
		}
	}
	if (MapHeldTicks.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] %s moves again after the map (%d ticks)"), *GetNameSafe(MapHeldPawn.Get()),
			MapHeldTicks.Num());
	}
	MapHeldTicks.Reset();
	MapHeldPawn.Reset();
}

bool UAPSWorldOriginSubsystem::IsStellarCatalogueSettling() const
{
	for (TActorIterator<AAstroGenerator> It(GetWorld()); It; ++It)
	{
		if (!It->GetCanonicalStellarProjectionDescriptor().bFinalized)
		{
			return true;
		}
	}
	return false;
}

bool UAPSWorldOriginSubsystem::RebaseOnto(const FVector& WorldLocation, const TCHAR* Reason)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	if (IsStellarCatalogueSettling())
	{
		// The canonical star catalogue validates its proxy bounds in the frame it was generated in, at the end of level
		// initialisation. The spawn shift used to come first; a surface start moves ~2 500 km, failed those bounds and
		// left the catalogue unused: no stars after a surface spawn (regression of b21445f9, 29.09). The shift now waits
		// for the catalogue (the same frame's initialisation) and then centres on the player where they stand.
		if (!bRebaseDeferred)
		{
			bRebaseDeferred = true;
			DeferredRebaseTicks = 0;
			DeferredRebaseReason = FString::Printf(TEXT("%s, after the star catalogue"), Reason);
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UAPSWorldOriginSubsystem::RetryDeferredRebase));
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] defer reason=%s until the star catalogue is final"), Reason);
		}
		return false;
	}
	return RebaseNow(WorldLocation, Reason);
}

void UAPSWorldOriginSubsystem::RetryDeferredRebase()
{
	UWorld* World = GetWorld();
	if (!World || !bRebaseDeferred)
	{
		return;
	}
	// A catalogue that never finalizes (rejected dataset) must not keep the player off-centre: give up after ~2 s.
	if (IsStellarCatalogueSettling() && ++DeferredRebaseTicks < 120)
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UAPSWorldOriginSubsystem::RetryDeferredRebase));
		return;
	}
	bRebaseDeferred = false;
	if (const APawn* Pawn = APSWorldOrigin::PlayerPawn(World))
	{
		RebaseNow(Pawn->GetActorLocation(), *DeferredRebaseReason);
	}
}

bool UAPSWorldOriginSubsystem::RebaseNow(const FVector& WorldLocation, const TCHAR* Reason)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const double DistanceCm = WorldLocation.Size();
	if (APSWorldOrigin::CVarRebaseOnSpawn.GetValueOnGameThread() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] skip reason=%s: aps.WorldOrigin.RebaseOnSpawn 0, target %.1f km from 0,0,0"),
			Reason, DistanceCm / 100000.0);
		return false;
	}
	if (DistanceCm < APSWorldOrigin::CVarMinShiftMeters.GetValueOnGameThread() * 100.0)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] skip reason=%s: target already %.1f m from 0,0,0"),
			Reason, DistanceCm / 100.0);
		return false;
	}

	const FIntVector OldOrigin = World->OriginLocation;
	const FVector Target = FVector(OldOrigin) + WorldLocation;
	if (FMath::Abs(Target.X) > APSWorldOrigin::MaximumOriginCm || FMath::Abs(Target.Y) > APSWorldOrigin::MaximumOriginCm
		|| FMath::Abs(Target.Z) > APSWorldOrigin::MaximumOriginCm)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.WorldOrigin] skip reason=%s: new origin %s is outside the engine's int32 range"),
			Reason, *Target.ToCompactString());
		return false;
	}
	const FIntVector NewOrigin(
		static_cast<int32>(FMath::RoundToDouble(Target.X)),
		static_cast<int32>(FMath::RoundToDouble(Target.Y)),
		static_cast<int32>(FMath::RoundToDouble(Target.Z)));

	const APawn* Pawn = APSWorldOrigin::PlayerPawn(World);
	const FVector PawnBefore = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const double StartSeconds = FPlatformTime::Seconds();
	if (!World->SetNewWorldOrigin(NewOrigin))
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.WorldOrigin] engine refused the shift reason=%s (level visibility request pending)"),
			Reason);
		return false;
	}
	const double ShiftMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	const FVector PawnAfter = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldOrigin] rebase reason=%s shift=%.3f km origin %s -> %s | pawn %.3f km -> %.2f m from 0,0,0 (%s) | %.1f ms"),
		Reason, DistanceCm / 100000.0, *OldOrigin.ToString(), *NewOrigin.ToString(), PawnBefore.Size() / 100000.0,
		PawnAfter.Size() / 100.0, *PawnAfter.ToCompactString(), ShiftMs);
	return true;
}
