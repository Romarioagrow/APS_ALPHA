#include "APSWorldOriginSubsystem.h"
#include "APSRealScale.h"
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
#include "Camera/PlayerCameraManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/ModelComponent.h"
#include "ContentStreaming.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DelayedAutoRegister.h"
#include "PhysicsField/PhysicsFieldComponent.h"
#include "PlanetaryAtmosphere.h"
#include "SceneInterface.h"
#include "Rendering/MotionVectorSimulation.h"
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
	TAutoConsoleVariable<float> CVarRealFloatDriftLy(
		TEXT("aps.RealScale.FloatDriftLy"), 20.0f,
		TEXT("Rio 05.10 (REAL SCALE, stage 2): a ship crossing a REAL SCALE world at light years a second shifts the world as ")
		TEXT("soon as it is this many light years from 0,0,0, without the two-second wait, so the camera stays well inside the ")
		TEXT("renderer's ~42 ly precision range (a shift costs ~7 ms: at 100 ly/s one every 0.2 s). 0: the legacy rule."));
	// Rio 05.10 (the hull still shimmers at drive speeds, already at a couple of c): test switches for what a shift does
	// besides moving things, to find which part shows on screen. 1 (default) keeps each.
	TAutoConsoleVariable<int32> CVarShiftRefileNanite(
		TEXT("aps.WorldOrigin.ShiftRefileNanite"), 1,
		TEXT("Test: 1 re-creates every Nanite mesh's render state on a shift (re-files it in the culling grid); 0 skips it."));
	TAutoConsoleVariable<int32> CVarShiftCameraJump(
		TEXT("aps.WorldOrigin.ShiftCameraJump"), 1,
		TEXT("Test: 1 tells the renderer the camera jumped on a shift (virtual shadow map panning off); 0 does not."));
	TAutoConsoleVariable<int32> CVarFlowViewStill(
		TEXT("aps.RealScale.FlowViewStill"), 1,
		TEXT("Rio 05.10 evening (\"the edges ripple at speed, aboard too\"): 1 tells the renderer that a view riding a fast ")
		TEXT("REAL SCALE ship moved only with the ship's own small step when the world flows past it (not by the whole ")
		TEXT("shift: its previous view a light year away left TSR, Lumen and the motion vectors nothing but float noise, and ")
		TEXT("no frame kept any history), and resets the riders' velocity state the scene shift moved. 0: as before."));
	TAutoConsoleVariable<float> CVarRealFloatFastKmPerS(
		TEXT("aps.RealScale.FloatFastKmPerS"), 1000.0f,
		TEXT("Rio 05.10 (at drive speeds the hull and the walking pilot jumped all over the screen; slowed down all was ")
		TEXT("well): in a REAL SCALE world a pawn faster than this (its own speed, or that of the ship it walks aboard) keeps ")
		TEXT("the world centred on itself every frame it is farther than aps.RealScale.FloatFastDriftKm from 0,0,0. 0: off."));
	TAutoConsoleVariable<float> CVarRealFloatFastDriftKm(
		TEXT("aps.RealScale.FloatFastDriftKm"), 100000.0f,
		TEXT("Rio 05.10: how far from 0,0,0 a fast pawn of a REAL SCALE world gets before the world shifts onto it. The ")
		TEXT("renderer's double-float error is about this distance x 3.5e-15 (1e10 cm: 0.04 mm; at 17 ly it was ~50 m)."));
	TAutoConsoleVariable<int32> CVarRealFlowWorld(
		TEXT("aps.RealScale.FlowWorld"), 1,
		TEXT("Rio 05.10 (at billions of c the hull, the camera and the walking pilot jumped by metres): 1 puts the whole ")
		TEXT("grains of a fast REAL SCALE ship's step straight into the world shift, the ship and its passengers staying ")
		TEXT("put, so they never stand at the step's 1e18 cm (double steps of metres). 0: the ship moves its whole step and ")
		TEXT("the world shifts back after the frame's ticks (the 05.10 05:00 way)."));
	constexpr double LightYearCm = 9.4607304725808e17;
	/** Fast REAL SCALE shifts move by whole multiples of this (cm), beyond ~1.5 kpc of the generation origin by the larger
	 * one: the far actors and the float origin then add every shift exactly instead of drifting by a rounding each time. */
	constexpr double RealShiftGrainCm = 1048576.0;
	constexpr double RealShiftFarGrainCm = 16777216.0;
	constexpr double RealShiftFarCm = 4.7e21;
	/** Atmospheres nearer the pawn than this (cm, ~7 AU) re-take their materials' light position right after a shift. */
	constexpr double AtmosphereRefreshCm = 1.0e14;
	/** A flow shift re-files the Nanite meshes this near the pawn (cm, 100 000 km); the rest once the flow ends. */
	constexpr double NaniteNearCm = 1.0e10;

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

	// Rio 05.10 (night optimisation): what a world shift has to move, by actor class: physics bodies (each one teleported
	// on every shift), Nanite meshes (re-filed), instanced meshes, ticking spring arms (a camera each, even unpossessed).
	FAutoConsoleCommandWithWorld ShiftCostReportCommand(
		TEXT("aps.WorldOrigin.ShiftCostReport"),
		TEXT("Logs, per actor class, the physics bodies, Nanite meshes, instanced meshes and ticking spring arms a world shift moves."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UWorld* GameWorld = FindGameWorld(World);
			if (!GameWorld)
			{
				return;
			}
			struct FRow { int32 Actors = 0; int32 Bodies = 0; int32 Nanite = 0; int32 Instanced = 0; int32 Arms = 0; };
			TMap<FString, FRow> Rows;
			FRow Total;
			for (TActorIterator<AActor> It(GameWorld); It; ++It)
			{
				FRow Row;
				Row.Actors = 1;
				It->ForEachComponent<UActorComponent>(false, [&Row](UActorComponent* Component)
				{
					if (const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
					{
						Row.Bodies += Primitive->IsPhysicsStateCreated() && Primitive->GetBodyInstance()
							&& Primitive->GetBodyInstance()->IsValidBodyInstance() ? 1 : 0;
						const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Primitive);
						Row.Nanite += Mesh && Mesh->IsRenderStateCreated() && !Mesh->bDisallowNanite
							&& !Mesh->bForceDisableNanite && Mesh->HasValidNaniteData() ? 1 : 0;
						Row.Instanced += Cast<UInstancedStaticMeshComponent>(Primitive) ? 1 : 0;
					}
					Row.Arms += Component->IsA<USpringArmComponent>() && Component->IsComponentTickEnabled() ? 1 : 0;
				});
				if (Row.Bodies + Row.Nanite + Row.Instanced + Row.Arms == 0)
				{
					continue;
				}
				FRow& Sum = Rows.FindOrAdd(It->GetClass()->GetName());
				Sum.Actors += Row.Actors; Sum.Bodies += Row.Bodies; Sum.Nanite += Row.Nanite;
				Sum.Instanced += Row.Instanced; Sum.Arms += Row.Arms;
				Total.Actors += Row.Actors; Total.Bodies += Row.Bodies; Total.Nanite += Row.Nanite;
				Total.Instanced += Row.Instanced; Total.Arms += Row.Arms;
			}
			Rows.ValueSort([](const FRow& A, const FRow& B) { return A.Bodies + A.Nanite + A.Arms > B.Bodies + B.Nanite + B.Arms; });
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] shift cost: %d actors, %d physics bodies, %d Nanite meshes, %d instanced meshes, %d ticking spring arms"),
				Total.Actors, Total.Bodies, Total.Nanite, Total.Instanced, Total.Arms);
			for (const TPair<FString, FRow>& Pair : Rows)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin]   %-48s actors %3d bodies %4d Nanite %4d instanced %3d arms %2d"),
					*Pair.Key, Pair.Value.Actors, Pair.Value.Bodies, Pair.Value.Nanite, Pair.Value.Instanced, Pair.Value.Arms);
			}
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

bool UAPSWorldOriginSubsystem::ShiftWorldBy(const FVector& Offset, const TCHAR* Reason, const bool bQuiet,
	AActor* Carrier)
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
	if (APSWorldOrigin::CVarShiftCameraJump.GetValueOnGameThread() != 0)
	{
		APSRenderSafety::MarkCameraJump(TEXT("floating origin shift"));
	}
	// Rio 05.10 (FlowPastShip): a carrier and everything riding it stay where they are, left out of the shift altogether
	// (not even a zero offset: UE 5.4's UNiagaraComponent::ApplyWorldOffset re-starts its effect on any shift, and the
	// ship's bodies would all be teleported again). Their render proxies, which the scene-wide shift below moves with
	// everything else, come back with the carrier's own move that follows (FlowPastShip never leaves it unmoved).
	TArray<AActor*> Carried;
	if (IsValid(Carrier))
	{
		Carrier->GetAttachedActors(Carried, true, true);
		Carried.Insert(Carrier, 0);
		// The camera manager caches the view of a carried camera (read by every tick until UpdateCameraManager, e.g. the
		// atmospheres' inside/outside choice): moved with the world it would point a whole shift away for the frame.
		const APlayerController* Player = World->GetFirstPlayerController();
		if (APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
			Camera && Carried.Contains(Camera->GetViewTarget()))
		{
			Carried.Add(Camera);
		}
	}
	const auto IsCarried = [&Carried](const UActorComponent* Component)
	{
		return !Carried.IsEmpty() && Carried.Contains(Component->GetOwner());
	};

	// UWorld::SetNewWorldOrigin's own steps with a double offset; its FIntVector origin is left as it is. The physics
	// scene follows its bodies' components (Chaos does not shift itself: FPhysScene::SupportsOriginShifting is false).
	// Accumulated: the renderer moves its previous view by the frame's whole shift, and a frame can shift twice.
	World->OriginOffsetThisFrame += Offset;
	// Rio 05.10: where a shift's time goes (actors, the rest, instanced, splines, Nanite, listeners), for the logs.
	double Sections[6] = {};
	double Mark = FPlatformTime::Seconds();
	const auto Lap = [&Sections, &Mark](const int32 Section)
	{
		const double Now = FPlatformTime::Seconds();
		Sections[Section] += (Now - Mark) * 1000.0;
		Mark = Now;
	};
	World->Scene->ApplyWorldOffset(Offset);
	for (ULevel* Level : World->GetLevels())
	{
		if (!Level || !(Level->bIsVisible || Level->IsPersistentLevel()))
		{
			continue;
		}
		if (Carried.IsEmpty())
		{
			Level->ApplyWorldOffset(Offset, true);
			continue;
		}
		// ULevel::ApplyWorldOffset's steps for a world shift (its precomputed lighting moves with the scene), the carried
		// actors skipped.
		for (AActor* Actor : Level->Actors)
		{
			if (Actor && !Carried.Contains(Actor))
			{
				Actor->ApplyWorldOffset(Actor->bIgnoresOriginShifting ? FVector::ZeroVector : Offset, true);
			}
		}
		for (UModelComponent* Model : Level->ModelComponents)
		{
			if (Model)
			{
				Model->ApplyWorldOffset(Offset, true);
			}
		}
		IStreamingManager::Get().NotifyLevelOffset(Level, Offset);
		FWorldDelegates::PostApplyLevelOffset.Broadcast(Level, World, Offset, true);
	}
	Lap(0);
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
	Lap(1);
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
			if (Instanced->IsA<UHierarchicalInstancedStaticMeshComponent>() && Instanced->IsRenderStateCreated()
				&& !IsCarried(Instanced))
			{
				Instanced->MarkRenderStateDirty();
			}
			++InstancedMeshes;
		}
	}
	Lap(2);
	// Rio 03.10 (the same GPUScene.cpp:367 mismatch on some shifts, primitives other than instanced meshes): UE 5.4's spline
	// mesh proxies (the orbit lines, BP_OrbiteSpline) keep an instance buffer too. Re-created, there are only a few dozen.
	for (TObjectIterator<USplineMeshComponent> It; It; ++It)
	{
		USplineMeshComponent* Spline = *It;
		if (IsValid(Spline) && Spline->GetWorld() == World && Spline->IsRegistered() && Spline->IsRenderStateCreated()
			&& !IsCarried(Spline))
		{
			Spline->MarkRenderStateDirty();
			++InstancedMeshes;
		}
	}
	Lap(3);
	// Rio 04.10 ("jumped out of the ship and it vanished", only its glass, ramp and props stayed): UE 5.4's scene culling
	// files a Nanite primitive into its grid cell when it is added or moves. A shift moves the proxies without either (the
	// resent transform is skipped as redundant), so a parked Nanite hull stayed culled by its old cell 38 km away. Its
	// render state is re-created, which files it again; the non-Nanite parts never used the cells. A carrier's meshes did
	// not move (their re-sent transform files them again) and are left alone: no hull re-created every frame of a flow.
	// Rio 05.10 afternoon (flight FPS at speed): a flow shift (every frame at drive speed) re-files only the meshes near the
	// pawn; the far ones owe their re-file, paid once as the flow ends (UpdateFloatingOrigin).
	const FVector NearPawn = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const int32 NaniteMeshes = RefileNanite(Carried.IsEmpty() ? nullptr : &NearPawn, Carried);
	Lap(4);
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
	// Rio 05.10 (atmospheres flickered at speed): AtmoScape writes its star's world position into its materials in its
	// own tick (LightPosition); one that ticked before this shift lit the air from where the star was a shift ago for the
	// rest of the frame (a flow shifts every frame at speed, the old fast path after every tick). The near ones re-take
	// their parameters now; far ones are invisible and catch up on their next tick.
	const FVector Viewer = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	for (TActorIterator<AAtmoScape> It(World); It; ++It)
	{
		if (FVector::DistSquared(It->GetActorLocation(), Viewer) < FMath::Square(APSWorldOrigin::AtmosphereRefreshCm))
		{
			It->UpdatePresentationScale();
		}
	}
	FloatOrigin -= Offset;
	// ApplyWorldOffset bypasses TransformUpdated; the engine's integer-origin
	// event does not run on this double-precision path. Refresh material/cloud
	// uniforms only after every actor and the camera have reached the new frame.
	APSWorldShiftEvents::OnPostDoubleShift().Broadcast(World, Offset);
	Lap(5);
	SinceFloatShiftSeconds = 0.0;
	const double ShiftMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	if (bQuiet)
	{
		// Rio 05.10: the per-frame shifts of a fast REAL SCALE pawn log one summary a second, not 60 lines.
		static double QuietSince = 0.0;
		static int32 QuietCount = 0;
		static double QuietMs = 0.0;
		static double QuietSections[6] = {};
		++QuietCount;
		QuietMs += ShiftMs;
		for (int32 Section = 0; Section < 6; ++Section)
		{
			QuietSections[Section] += Sections[Section];
		}
		if (StartSeconds - QuietSince >= 1.0)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[APS.WorldOrigin] fast shifts reason=%s: %d in %.1f s, %.2f ms each (actors %.2f, other %.2f, instanced %.2f, splines %.2f, Nanite %.2f, listeners %.2f) | last by %.3f km, pawn -> %.2f m from 0,0,0 | generation offset %s"),
				Reason, QuietCount, QuietSince > 0.0 ? StartSeconds - QuietSince : 0.0, QuietMs / QuietCount,
				QuietSections[0] / QuietCount, QuietSections[1] / QuietCount, QuietSections[2] / QuietCount,
				QuietSections[3] / QuietCount, QuietSections[4] / QuietCount, QuietSections[5] / QuietCount,
				Offset.Size() / 100000.0, Pawn ? Pawn->GetActorLocation().Size() / 100.0 : -1.0,
				*GetOriginOffset().ToCompactString());
			QuietSince = StartSeconds;
			QuietCount = 0;
			QuietMs = 0.0;
			for (double& Section : QuietSections)
			{
				Section = 0.0;
			}
		}
		return true;
	}
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldOrigin] float shift reason=%s by %.3f km | pawn %.3f km -> %.2f m from 0,0,0 | generation offset %s | %d instanced and spline meshes resent, %d Nanite meshes refiled | %.1f ms"),
		Reason, Offset.Size() / 100000.0, PawnBefore.Size() / 100000.0,
		Pawn ? Pawn->GetActorLocation().Size() / 100.0 : -1.0, *GetOriginOffset().ToCompactString(), InstancedMeshes,
		NaniteMeshes, ShiftMs);
	return true;
}

int32 UAPSWorldOriginSubsystem::RefileNanite(const FVector* NearOnly, const TArray<AActor*>& Carried)
{
	UWorld* World = GetWorld();
	if (!World || APSWorldOrigin::CVarShiftRefileNanite.GetValueOnGameThread() == 0)
	{
		return 0;
	}
	int32 Refiled = 0;
	for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
	{
		UStaticMeshComponent* Mesh = *It;
		if (!IsValid(Mesh) || Mesh->GetWorld() != World || !Mesh->IsRegistered() || !Mesh->IsRenderStateCreated()
			|| Mesh->bDisallowNanite || Mesh->bForceDisableNanite || !Mesh->HasValidNaniteData()
			|| (!Carried.IsEmpty() && Carried.Contains(Mesh->GetOwner())))
		{
			continue;
		}
		if (NearOnly && FVector::DistSquared(Mesh->Bounds.Origin, *NearOnly)
			> FMath::Square(APSWorldOrigin::NaniteNearCm + Mesh->Bounds.SphereRadius))
		{
			bNaniteRefileOwed = true;
			continue;
		}
		Mesh->MarkRenderStateDirty();
		++Refiled;
	}
	if (!NearOnly)
	{
		bNaniteRefileOwed = false;
	}
	return Refiled;
}

FVector UAPSWorldOriginSubsystem::FlowPastShip(AActor& Ship, const FVector& Delta, const double SpeedCmPerS)
{
	UWorld* World = GetWorld();
	if (!World || APSWorldOrigin::CVarRealFlowWorld.GetValueOnGameThread() == 0
		|| APSWorldOrigin::CVarFloat.GetValueOnGameThread() == 0 || bRebaseDeferred || bMapShifted
		|| !APSRealScale::IsActive(World))
	{
		return Delta;
	}
	const double FastCmPerS = FMath::Max(APSWorldOrigin::CVarRealFloatFastKmPerS.GetValueOnGameThread(), 0.0f) * 1.0e5;
	const double FastDriftCm = FMath::Max(APSWorldOrigin::CVarRealFloatFastDriftKm.GetValueOnGameThread(), 1.0f) * 1.0e5;
	if (FastCmPerS <= 0.0 || SpeedCmPerS <= FastCmPerS)
	{
		return Delta;
	}
	// Only the ship that carries the player: piloted, or with the player walking aboard (attached to it).
	const APawn* Pawn = APSWorldOrigin::PlayerPawn(World);
	const USceneComponent* PawnRoot = Pawn ? Pawn->GetRootComponent() : nullptr;
	if (!PawnRoot || PawnRoot->GetAttachmentRootActor() != &Ship)
	{
		return Delta;
	}
	// The map's own far view (TryMapShift) and a settling star catalogue keep the world still, as for every shift.
	const AGravityPlayerController* Controller = Cast<AGravityPlayerController>(Pawn->GetController());
	if ((Controller && Controller->IsStrategicMapOpen()) || IsStellarCatalogueSettling())
	{
		return Delta;
	}
	// Where the step would end (rounded at its own size, only to pick the grains); a short way out it simply moves.
	const FVector Target = Ship.GetActorLocation() + Delta;
	if (Target.SizeSquared() <= FMath::Square(FastDriftCm))
	{
		return Delta;
	}
	const double Grain = GetOriginOffset().GetAbsMax() > APSWorldOrigin::RealShiftFarCm
		? APSWorldOrigin::RealShiftFarGrainCm : APSWorldOrigin::RealShiftGrainCm;
	const FVector Grains(FMath::RoundToDouble(Target.X / Grain) * Grain, FMath::RoundToDouble(Target.Y / Grain) * Grain,
		FMath::RoundToDouble(Target.Z / Grain) * Grain);
	// Exact: the step and its grains differ by about the ship's own small offset from 0,0,0 (Sterbenz), so the ship
	// ends within half a grain of 0,0,0 and nothing it carries is ever placed at the step's size. The ship must still
	// move after the shift: that move re-sends its proxies, which the scene shift moved (an unmoved one would be
	// skipped as a redundant update and stay a whole shift away), so a step of exact grains moves as before.
	const FVector Rest = Delta - Grains;
	if (Grains.IsZero() || Rest.IsNearlyZero(1.0e-3)
		|| !ShiftWorldBy(-Grains, TEXT("the world flows past a fast REAL SCALE ship"), true, &Ship))
	{
		return Delta;
	}
	SinceFlowShiftSeconds = 0.0;
	// Rio 05.10 evening (edge ripple at speed): the view rides the ship, so between the frames it moved only by the ship's
	// Rest. The renderer moves its previous view by OriginOffsetThisFrame (UE 5.4 SceneVisibility.cpp): by the whole shift
	// it lay light years off, and in float maths nothing kept any TSR history (r.TSR.Visualize 0: nothing accumulated,
	// flying or aboard). Moved by Rest it is where the view is; the riders' velocity state, which the scene-wide shift
	// moved too, is reset after the ship's own move (FinishFlowMove). The world around gets no motion of its own from the
	// shift: far away it hardly moves on screen, and near things at these speeds pass in a frame or two.
	const APlayerController* Player = World->GetFirstPlayerController();
	const AActor* ViewTarget = Player && Player->PlayerCameraManager ? Player->PlayerCameraManager->GetViewTarget() : nullptr;
	const USceneComponent* ViewRoot = ViewTarget ? ViewTarget->GetRootComponent() : nullptr;
	bFlowViewStill = APSWorldOrigin::CVarFlowViewStill.GetValueOnGameThread() != 0 && ViewRoot
		&& ViewRoot->GetAttachmentRootActor() == &Ship;
	if (bFlowViewStill)
	{
		World->OriginOffsetThisFrame += Grains + Rest;
	}
	if (const double Now = FPlatformTime::Seconds(); Now - LastFlowViewLogSeconds >= 1.0)
	{
		LastFlowViewLogSeconds = Now;
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] flow view: still=%d view=%s rest %.1f m, shift %.3e cm, frame offset %s"),
			bFlowViewStill ? 1 : 0, *GetNameSafe(ViewTarget), Rest.Size() / 100.0, Grains.Size(),
			*World->OriginOffsetThisFrame.ToCompactString());
	}
	return Rest;
}

void UAPSWorldOriginSubsystem::FinishFlowMove(AActor& Ship)
{
	if (!bFlowViewStill)
	{
		return;
	}
	bFlowViewStill = false;
	// The ship and its riders after the ship's own step. The scene-wide shift moved their proxies' previous transform a
	// whole shift away, and a velocity reset in the same frame does not help: the transform update that follows takes the
	// shifted proxy matrix as the previous one again (UE 5.4 RendererScene.cpp ~6210; also on the Epic forum, "How to
	// properly manage motion vectors for primitives in render scene"). The engine's own override
	// (FMotionVectorSimulation, applied after that update, RendererScene.cpp ~2126/6238) gives each its previous
	// transform here: where it is now, as the view moved with it. A rider that moves again this frame (a pilot walking
	// aboard) keeps that motion of its own.
	static IConsoleVariable* const Simulation = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionVectorSimulation"));
	if (Simulation && Simulation->GetInt() == 0)
	{
		Simulation->Set(1, ECVF_SetByCode);
	}
	FMotionVectorSimulation& Vectors = FMotionVectorSimulation::Get();
	TArray<AActor*> Riders;
	Ship.GetAttachedActors(Riders, true, true);
	Riders.Add(&Ship);
	for (AActor* Rider : Riders)
	{
		if (IsValid(Rider))
		{
			Rider->ForEachComponent<UPrimitiveComponent>(false, [&Vectors](UPrimitiveComponent* Primitive)
			{
				if (Primitive->IsRegistered() && Primitive->SceneProxy)
				{
					Vectors.SetPreviousTransform(Primitive, Primitive->GetComponentTransform());
				}
			});
		}
	}
}

void UAPSWorldOriginSubsystem::UpdateFloatingOrigin(const float DeltaSeconds)
{
	SinceFloatShiftSeconds += DeltaSeconds;
	SinceFlowShiftSeconds += DeltaSeconds;
	// Rio 05.10 afternoon: the far Nanite meshes the flow shifts skipped are re-filed once it has stopped for a moment.
	if (bNaniteRefileOwed && SinceFlowShiftSeconds > 0.25)
	{
		RefileNanite(nullptr, TArray<AActor*>());
	}
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
	// Rio 05.10 ("at tens of millions of c the hull jumps all over the screen, a hundred and fifty times a second, and the
	// pilot walking aboard too; slowed down all is fine, however far we flew"): between the shifts below a drive-speed ship
	// got 17 ly from 0,0,0, where the renderer's double-float transforms are only good to ~50 m, a new error every frame.
	// In a REAL SCALE world a fast pawn (its own speed, or the ship it walks aboard) now keeps the world centred on itself
	// every frame, by whole grains. This runs from the tickable, after the actors moved and before the frame is drawn, so
	// the hull and the camera are drawn at ~0 in the same frame.
	if (World && Pawn && APSRealScale::IsActive(World) && APSWorldOrigin::CVarFloat.GetValueOnGameThread() != 0
		&& !bRebaseDeferred && !bMapOpen)
	{
		const double FastCmPerS = FMath::Max(APSWorldOrigin::CVarRealFloatFastKmPerS.GetValueOnGameThread(), 0.0f) * 1.0e5;
		const double FastDriftCm = FMath::Max(APSWorldOrigin::CVarRealFloatFastDriftKm.GetValueOnGameThread(), 1.0f) * 1.0e5;
		const ASpaceship* Mover = Cast<ASpaceship>(Pawn);
		if (!Mover)
		{
			Mover = Cast<ASpaceship>(Pawn->GetAttachParentActor());
		}
		const double MoverSpeedCm = Mover ? FMath::Max(Mover->GetKinematicVelocity().Size(), Mover->GetVelocity().Size())
			: Pawn->GetVelocity().Size();
		const FVector Here = Pawn->GetActorLocation();
		if (FastCmPerS > 0.0 && MoverSpeedCm > FastCmPerS && Here.SizeSquared() > FMath::Square(FastDriftCm)
			&& !IsStellarCatalogueSettling())
		{
			const double Grain = GetOriginOffset().GetAbsMax() > APSWorldOrigin::RealShiftFarCm
				? APSWorldOrigin::RealShiftFarGrainCm : APSWorldOrigin::RealShiftGrainCm;
			const FVector Offset(FMath::RoundToDouble(-Here.X / Grain) * Grain, FMath::RoundToDouble(-Here.Y / Grain) * Grain,
				FMath::RoundToDouble(-Here.Z / Grain) * Grain);
			if (!Offset.IsNearlyZero())
			{
				ShiftWorldBy(Offset, TEXT("a fast pawn of a REAL SCALE world"), true);
				return;
			}
		}
	}
	// Rio 05.10 (real scale, stage 2): between the stars of a REAL SCALE world a ship flies light years a second; past
	// aps.RealScale.FloatDriftLy from 0,0,0 the world shifts now, whatever the two-second wait (still never under the map).
	const double RealDriftCm = APSRealScale::IsActive(World)
		? FMath::Max(static_cast<double>(APSWorldOrigin::CVarRealFloatDriftLy.GetValueOnGameThread()), 0.0)
			* APSWorldOrigin::LightYearCm
		: 0.0;
	const bool bRealOverdue = RealDriftCm > 0.0 && Pawn && Pawn->GetActorLocation().SizeSquared() > FMath::Square(RealDriftCm);
	if (!World || APSWorldOrigin::CVarFloat.GetValueOnGameThread() == 0 || bRebaseDeferred
		|| (SinceFloatShiftSeconds < 2.0 && !bRealOverdue))
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
		// Rio 05.10 (real scale, stage 2): a REAL SCALE ship's lead never reaches past aps.RealScale.FloatDriftLy.
		const double LeadCm = RealDriftCm > 0.0 ? FMath::Min(SpeedCm * LeadSeconds, RealDriftCm) : SpeedCm * LeadSeconds;
		if (LeadSeconds <= 0.0 || Location.SizeSquared() <= FMath::Square(FMath::Max(LimitCm, LeadCm)))
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
	FVector Offset = -Location;
	if (RealDriftCm > 0.0 && bFast)
	{
		// Rio 05.10 (real scale, stage 2): a fast REAL SCALE shift moves by whole grains, so the float origin and the far
		// actors (the home system, parsecs out) take it exactly; the pilot stays within half a grain (~5 km) of 0,0,0.
		const double Grain = GetOriginOffset().GetAbsMax() > APSWorldOrigin::RealShiftFarCm
			? APSWorldOrigin::RealShiftFarGrainCm : APSWorldOrigin::RealShiftGrainCm;
		Offset = FVector(FMath::RoundToDouble(Offset.X / Grain) * Grain, FMath::RoundToDouble(Offset.Y / Grain) * Grain,
			FMath::RoundToDouble(Offset.Z / Grain) * Grain);
	}
	ShiftWorldBy(Offset, bFreeFlight ? (bFast ? TEXT("a fast free-flight camera far out") : TEXT("a calm free-flight camera far out"))
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
		// Rio 05.10 (real scale, stage 2): a REAL SCALE world reaches far past the int32 origin (a start or a load parsecs
		// out): the double-precision shift makes the spot 0,0,0 instead.
		if (APSRealScale::IsActive(World))
		{
			return ShiftWorldBy(-WorldLocation, Reason);
		}
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
