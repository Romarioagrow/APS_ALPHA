#include "APSWorldOriginSubsystem.h"
#include "APSRealScale.h"
#include "APSWorldShiftEvents.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyGpuStars.h"
#include "APS_ALPHA/Core/Rendering/APSRenderSafety.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
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
#include "ProfilingDebugging/CpuProfilerTrace.h"
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
	// Rio 06.10 ("the star material smears at high speed"): on a flow or pay frame the view is told it moved only with the
	// ship's own step, so the renderer moves every proxy's previous transform with the world and a star a few radii off
	// gets no motion although it slides pixels on screen; TSR then drags its surface into streaks. Within a star's 1000
	// radii no still ship takes over (aps.RealScale.DeferBodyClearRadii), so close approaches are all flow frames.
	TAutoConsoleVariable<int32> CVarFlowBodyParallax(
		TEXT("aps.RealScale.FlowBodyParallax"), 1,
		TEXT("1: on flow and pay frames a nearby star's photosphere gets its real previous place relative to the still view, ")
		TEXT("so TSR reprojects it instead of smearing it. 0: as before (no motion vectors for the world on those frames)."));
	/** The view's own step on this flow or pay frame (a nearby star moved by minus it relative to the still view). */
	TWeakObjectPtr<const UWorld> FlowViewStepWorld;
	FVector FlowViewStep{FVector::ZeroVector};
	void SetFlowViewStep(const UWorld* World, const FVector& Step)
	{
		FlowViewStepWorld = World;
		FlowViewStep = Step;
	}
	TAutoConsoleVariable<int32> CVarFlowViewStill(
		TEXT("aps.RealScale.FlowViewStill"), 1,
		TEXT("Rio 05.10 evening (\"the edges ripple at speed, aboard too\"): 1 tells the renderer that a view riding a fast ")
		TEXT("REAL SCALE ship moved only with the ship's own small step when the world flows past it (not by the whole ")
		TEXT("shift: its previous view a light year away left TSR, Lumen and the motion vectors nothing but float noise, and ")
		TEXT("no frame kept any history), and resets the riders' velocity state the scene shift moved. 0: as before."));
	/**
	 * Rio 06.10 (walking aboard a fast ship: 120 -> 40-60 fps, back in the seat 120 at once): a walker has the detailed
	 * hull's own body in the physics scene (aps.Ship.WalkOnShellAtSpeed 0), and at drive speed every frame's flow moved the
	 * ship by its rest after the shift: a kinematic move of all 13.5k shapes (~1.5 ms), their overlaps (~1.25 ms), the
	 * waits of every query near the hull and ~2.2 ms of physics-thread wait. With the switch on, a flow frame with the
	 * player walking aboard leaves the ship (and its body) where it is: the world still shifts by the whole grains, the
	 * sub-grain rest (at most a grain, 10.5 km; 168 km beyond 1.5 kpc) is added to the ship's next step (paid by a plain move
	 * if no step takes it the next frame, and before a settle), and the ship's and its riders' render state, which the
	 * scene-wide shift moved, is sent again where they are. The view did not move, so it tells the renderer so (Grains,
	 * not Grains + Rest). Seated flight is untouched.
	 */
	TAutoConsoleVariable<int32> CVarFlowKeepWalkerShip(
		TEXT("aps.RealScale.FlowKeepWalkerShip"), 1, // Rio 06.10 night: on after w-stand A/B (walking aboard at speed 70 -> 96 fps); 0 if exterior effects lag
		TEXT("Rio 06.10 (walking aboard at speed): 1: on a world flow frame with the player walking aboard (not seated) and the ")
		TEXT("view riding the ship, the ship and its body stay where they are; the step's sub-grain rest (at most one grain, ")
		TEXT("~10.5 km) is added to its next step, and its render state is re-sent past r.SkipRedundantTransformUpdate (no ")
		TEXT("keep when that is set from the console). 0: the ship moves by the rest after every flow shift (as before)."));
	/** The ship a kept flow frame left where it was, the rest it owes and the frame it was kept. */
	TWeakObjectPtr<AActor> FlowKeptShip;
	FVector FlowKeptRest{FVector::ZeroVector};
	uint64 FlowKeptFrame{0};
	/** The kept ship whose riders' render state FinishFlowMove sends again (after their previous transforms are given). */
	TWeakObjectPtr<AActor> FlowKeptResend;

	IConsoleVariable* SkipRedundantTransformUpdateVariable()
	{
		static IConsoleVariable* const SkipRedundant =
			IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkipRedundantTransformUpdate"));
		return SkipRedundant;
	}

	/** Code may switch r.SkipRedundantTransformUpdate off around a resend (a console-set value outranks code), or it is off. */
	bool CanForceRenderResend()
	{
		const IConsoleVariable* SkipRedundant = SkipRedundantTransformUpdateVariable();
		return !SkipRedundant || SkipRedundant->GetInt() == 0
			|| (uint32(SkipRedundant->GetFlags()) & ECVF_SetByMask) <= uint32(ECVF_SetByCode);
	}

	/**
	 * Rio 06.10 (aps.RealScale.FlowKeepWalkerShip): the scene-wide shift moved every render proxy, the kept ship's and its
	 * riders' too, while their components stayed. A plain MarkRenderTransformDirty would be judged redundant on the game
	 * thread (UE 5.4 RendererScene.cpp ~2142 compares with the proxy, which the render thread shifts later) and leave them a
	 * whole shift away, so the check is off while they are sent, without the sink call (as for the stars' parallax in
	 * FinishFlowMove). Lights and decals have no such check; they are simply sent again. No body moves.
	 */
	int32 ForceRenderResend(const TArray<AActor*>& Parts)
	{
		IConsoleVariable* SkipRedundant = SkipRedundantTransformUpdateVariable();
		const bool bToggle = SkipRedundant && SkipRedundant->GetInt() != 0
			&& (uint32(SkipRedundant->GetFlags()) & ECVF_SetByMask) <= uint32(ECVF_SetByCode);
		const EConsoleVariableFlags QuietCode = EConsoleVariableFlags(ECVF_SetByCode | ECVF_Set_NoSinkCall_Unsafe);
		const int32 SavedSkip = bToggle ? SkipRedundant->GetInt() : 0;
		if (bToggle)
		{
			SkipRedundant->Set(0, QuietCode);
		}
		int32 Sent = 0;
		for (AActor* Part : Parts)
		{
			if (!IsValid(Part))
			{
				continue;
			}
			Part->ForEachComponent<USceneComponent>(false, [&Sent](USceneComponent* Component)
			{
				// Rio 06.10 (review): a component whose render state is dirty is re-created at the end of the frame where it
				// is now (nothing to resend); re-created here it would run mid-tick, beside the walker's parallel animation.
				if (!IsValid(Component) || !Component->IsRegistered() || !Component->IsRenderStateCreated()
					|| Component->IsRenderStateDirty())
				{
					return;
				}
				if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
				{
					// Rio 06.10 (review): a primitive's transform only (it did not move, its bounds are current). Through the
					// deferred updates its pending dynamic data would go out too, the walker's bones while their animation
					// may still be evaluating on a worker; that and its instances follow at the end of the frame as always.
					UWorld* PrimitiveWorld = Primitive->GetWorld();
					if (Primitive->SceneProxy && PrimitiveWorld && PrimitiveWorld->Scene)
					{
						PrimitiveWorld->Scene->UpdatePrimitiveTransform(Primitive);
						++Sent;
					}
					return;
				}
				// Lights, decals, fog volumes, captures: their own transform send.
				Component->MarkRenderTransformDirty();
				Component->DoDeferredRenderUpdates_Concurrent();
				++Sent;
			});
		}
		if (bToggle)
		{
			SkipRedundant->Set(SavedSkip, QuietCode);
		}
		return Sent;
	}

	/** Rio 06.10 (aps.RealScale.FlowKeepWalkerShip): a kept ship's rest that no step took is paid with a plain move. */
	void PayKeptFlowRest(UWorld* World, const TCHAR* Reason)
	{
		AActor* Kept = FlowKeptShip.Get();
		const FVector KeptRest = FlowKeptRest;
		FlowKeptShip.Reset();
		FlowKeptRest = FVector::ZeroVector;
		if (!IsValid(Kept) || KeptRest.IsZero())
		{
			return;
		}
		Kept->AddActorWorldOffset(KeptRest, false, nullptr, ETeleportType::None);
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] flow keep: %s pays its kept rest %.1f m (%s)"), *Kept->GetName(),
			KeptRest.Size() / 100.0, Reason);
		// As after a settle: the view riding the ship is taken again from where it is now.
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
		const AActor* ViewTarget = Camera ? Camera->GetViewTarget() : nullptr;
		const USceneComponent* ViewRoot = ViewTarget ? ViewTarget->GetRootComponent() : nullptr;
		if (Camera && ViewRoot && ViewRoot->GetAttachmentRootActor() == Kept)
		{
			Camera->UpdateCamera(0.0f);
		}
	}
	TAutoConsoleVariable<float> CVarRealFloatFastKmPerS(
		TEXT("aps.RealScale.FloatFastKmPerS"), 1000.0f,
		TEXT("Rio 05.10 (at drive speeds the hull and the walking pilot jumped all over the screen; slowed down all was ")
		TEXT("well): in a REAL SCALE world a pawn faster than this (its own speed, or that of the ship it walks aboard) keeps ")
		TEXT("the world centred on itself every frame it is farther than aps.RealScale.FloatFastDriftKm from 0,0,0. 0: off."));
	TAutoConsoleVariable<float> CVarRealFloatFastDriftKm(
		TEXT("aps.RealScale.FloatFastDriftKm"), 100000.0f,
		TEXT("Rio 05.10: how far from 0,0,0 a fast pawn of a REAL SCALE world gets before the world shifts onto it. The ")
		TEXT("renderer's double-float error is about this distance x 3.5e-15 (1e10 cm: 0.04 mm; at 17 ly it was ~50 m)."));
	TAutoConsoleVariable<float> CVarFlowRideKm(
		TEXT("aps.RealScale.FlowRideKm"), 250.0f,
		TEXT("Rio 05.10 night (\"the jerking starts at about 1 c and stops at some point higher\"): between the world flows a ")
		TEXT("fast REAL SCALE ship still flies its own way out to aps.RealScale.FloatFastDriftKm, 500-100 000 km a frame; a ")
		TEXT("view riding it then tells the renderer it moved only with the ship (as on a flow frame) once a step is longer ")
		TEXT("than this, km, so TSR keeps its history instead of cutting or smearing it on those frames. Keep it at most half ")
		TEXT("of aps.Ship.TsrHistoryMaxStepKm. 0: off."));
	TAutoConsoleVariable<int32> CVarFlowDefer(
		TEXT("aps.RealScale.FlowDefer"), 1,
		TEXT("Rio 06.10 (still ship: \"move our whole high-speed travel onto this system\"): 1 keeps the world still far from ")
		TEXT("every star system, the fast ship and its view standing where they are, and moves only the sky (the star ")
		TEXT("catalogue and the systems' places) by the travel, owed in exact grains and paid with one flow shift at a ")
		TEXT("system's bubble, below flow speed, under the map, before a save or a materialization. 0: the world flows past ")
		TEXT("the ship every frame (05.10)."));
	TAutoConsoleVariable<float> CVarDeferClearLy(
		TEXT("aps.RealScale.DeferClearLy"), 0.5f,
		TEXT("Rio 06.10 (still ship): the travel is owed only while no star system (and no drawn catalogue star) is nearer ")
		TEXT("the ship than this, light years, plus two of its steps ahead. Keep it above aps.RealScale.MaterializeLy."));
	TAutoConsoleVariable<float> CVarDeferBodyClearRadii(
		TEXT("aps.RealScale.DeferBodyClearRadii"), 1000.0f,
		TEXT("Rio 06.10 (still ship while braking): inside the bubble of a star system that rides with the sky (one stood up ")
		TEXT("away from home) the travel stays owed until the ship is this many radii from one of its bodies (a star, a ")
		TEXT("planet, a moon), or aps.RealScale.DeferBodyClearKm, whichever is more; there the debt is paid and the world is ")
		TEXT("used as it is (physics, surfaces). 0: such a system ends the debt like any other (0.5 ly out)."));
	TAutoConsoleVariable<float> CVarDeferBodyClearKm(
		TEXT("aps.RealScale.DeferBodyClearKm"), 1000000.0f,
		TEXT("Rio 06.10 (still ship while braking): the least distance from a body of a system riding with the sky at which ")
		TEXT("the travel stays owed, km."));
	TAutoConsoleVariable<float> CVarDeferShipClearKm(
		TEXT("aps.RealScale.DeferShipClearKm"), 1000000.0f,
		TEXT("Rio 06.10 (\"no visual regression for the other objects: a ship flying ahead of me must be seen where it is\"): ")
		TEXT("another ship or a station this near the still ship (km, as the ship truly is) ends the debt, and the world flows ")
		TEXT("past as before, so it is drawn where it is and nothing about it changes. 0: other ships do not matter."));
	TAutoConsoleVariable<int32> CVarRealFlowWorld(
		TEXT("aps.RealScale.FlowWorld"), 1,
		TEXT("Rio 05.10 (at billions of c the hull, the camera and the walking pilot jumped by metres): 1 puts the whole ")
		TEXT("grains of a fast REAL SCALE ship's step straight into the world shift, the ship and its passengers staying ")
		TEXT("put, so they never stand at the step's 1e18 cm (double steps of metres). 0: the ship moves its whole step and ")
		TEXT("the world shifts back after the frame's ticks (the 05.10 05:00 way)."));
	TAutoConsoleVariable<int32> CVarFlowCarrySky(
		TEXT("aps.Origin.FlowCarrySky"), 1,
		TEXT("Rio 06.10 (flight FPS 20-40 on the way out of a system): 1 carries the gameplay star catalogue (the AstroGenerator ")
		TEXT("with its galaxy and cluster, only while nothing else hangs under it) through a flow or a pay shift and moves it ")
		TEXT("after by the same offset, so its two instanced catalogues (~61k stars) keep their render state instead of being ")
		TEXT("re-created on every flow frame (~2.5 ms); they end up where the shift would have put them. 0: the catalogue shifts ")
		TEXT("with the world and is re-created on every shift (as before)."));
	TAutoConsoleVariable<int32> CVarSkyMovesCatalogue(
		TEXT("aps.Origin.SkyMovesCatalogue"), 1,
		TEXT("Rio 06.10 (approaching a star its point vanished and came back elsewhere): 1 puts the gameplay star catalogue ")
		TEXT("(the stellar view's AstroGenerator) at its sky place, the home system + the sky offset, inside the call that ")
		TEXT("changes the sky offset (an owed step, a pay, a settle) and before OnSkyOffsetChanged is told, so every reader of ")
		TEXT("the frame (the star systems registering stars, the anchors, the GPU points, the key light) finds the catalogue ")
		TEXT("where it is drawn. 0: only the stellar view's snap moves it, later in the frame (as before)."));
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

	/** Rio 06.10 (the review): the colony terminal (Tab/K) places the home's assets among the catalogue's systems by their
	 * raw places; the still ship keeps no debt while it is open, as under the strategic map. */
	bool IsColonyTerminalOpen(const UWorld* World)
	{
		const UAPSColonyTerminalSubsystem* Terminal = World ? World->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
		return Terminal && Terminal->IsTerminalOpen();
	}

	/** Rio 06.10 (flight FPS, aps.Origin.FlowCarrySky): the sky a flow or a pay shift carries, the gameplay AstroGenerator
	 * (the one REAL SCALE and the stellar view use), or null. Only while its whole tree is the star catalogue (the galaxy
	 * and its clusters): anything else under it (an authored home system, a planet, a ship) needs the world's own shift
	 * (WorldScape, physics, the level's steps), and the sky then shifts with the world as before. */
	AActor* CarriedSky(const UWorld* World)
	{
		if (CVarFlowCarrySky.GetValueOnGameThread() == 0)
		{
			return nullptr;
		}
		AAstroGenerator* Sky = APSRealScale::FindGameplayGenerator(World);
		if (!IsValid(Sky) || Sky->GetAttachParentActor() || Sky->bIgnoresOriginShifting)
		{
			return nullptr;
		}
		TArray<AActor*> Tree;
		Sky->GetAttachedActors(Tree, true, true);
		for (AActor* Part : Tree)
		{
			if (IsValid(Part) && (Part->bIgnoresOriginShifting || !(Part->IsA<AGalaxy>() || Part->IsA<AStarCluster>())))
			{
				static TWeakObjectPtr<AActor> LoggedBlocker;
				if (LoggedBlocker.Get() != Part)
				{
					LoggedBlocker = Part;
					UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] flow carry sky: off, %s (%s) hangs under %s; the sky shifts with the world"),
						*Part->GetName(), *Part->GetClass()->GetName(), *Sky->GetName());
				}
				return nullptr;
			}
		}
		return Sky;
	}

	/** Rio 06.10 (aps.Origin.FlowCarrySky): the carried sky's own move this frame is no motion to draw (as PayDebt's
	 * members): each of its primitives takes its present transform as the previous one. The last call of a frame wins. */
	void KeepSkyMotionStill(AActor& Sky)
	{
		FMotionVectorSimulation& Vectors = FMotionVectorSimulation::Get();
		TArray<AActor*> Tree;
		Sky.GetAttachedActors(Tree, true, true);
		Tree.Add(&Sky);
		for (AActor* Part : Tree)
		{
			if (!IsValid(Part))
			{
				continue;
			}
			Part->ForEachComponent<UPrimitiveComponent>(false, [&Vectors](UPrimitiveComponent* Primitive)
			{
				if (Primitive->IsRegistered() && Primitive->SceneProxy)
				{
					Vectors.SetPreviousTransform(Primitive, Primitive->GetComponentTransform());
				}
			});
		}
	}

	/** Rio 06.10 (aps.Origin.SkyMovesCatalogue): the stellar view's generator, chosen by its own rule
	 * (UAPSStellarVisualSubsystem::UpdateGameplayStellarView: no menu preview, a final catalogue with its galaxy layer, a
	 * home system), kept weakly; the world's actors are searched at most once a second, and only while there is none. */
	struct FSkyCatalogueCache
	{
		TWeakObjectPtr<const UWorld> World;
		TWeakObjectPtr<AAstroGenerator> Generator;
		double NextScanSeconds{0.0};
		/** What was said last (placed, or why it is left to the snap), so each is logged once. */
		TWeakObjectPtr<const AAstroGenerator> LoggedGenerator;
		int32 LoggedState{-1};
		double LastBehindLogSeconds{0.0};
	};
	FSkyCatalogueCache GSkyCatalogue;

	bool IsStellarViewGenerator(const AAstroGenerator& Generator)
	{
		const FAPSCanonicalStellarProjectionDescriptor& Descriptor = Generator.GetCanonicalStellarProjectionDescriptor();
		return !Generator.ActorHasTag(TEXT("WorldGenerationPreview")) && !Generator.UsesContinuousPreviewFrame()
			&& Descriptor.bFinalized && Descriptor.Galaxy.bEnabled && IsValid(Generator.GetPreviewHomeSystem());
	}

	AAstroGenerator* SkyCatalogueGenerator(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		FSkyCatalogueCache& Cache = GSkyCatalogue;
		if (Cache.World.Get() != World)
		{
			Cache = FSkyCatalogueCache();
			Cache.World = World;
		}
		if (AAstroGenerator* Known = Cache.Generator.Get(); IsValid(Known) && Known->GetWorld() == World)
		{
			if (IsStellarViewGenerator(*Known))
			{
				return Known;
			}
			// No longer the one the stellar view would pick (a new generation): looked for again below.
			Cache.Generator.Reset();
		}
		const double Now = FPlatformTime::Seconds();
		if (Now < Cache.NextScanSeconds)
		{
			return nullptr;
		}
		Cache.NextScanSeconds = Now + 1.0;
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsStellarViewGenerator(**It))
			{
				Cache.Generator = *It;
				return *It;
			}
		}
		return nullptr;
	}

	/**
	 * Rio 06.10 (approaching a star, its point vanished, came back elsewhere and "went down"; aps.Origin.SkyMovesCatalogue):
	 * the stellar view's generator (the galaxy and cluster catalogues and their GPU points) is put at its sky place, the
	 * home system + the sky offset (the stellar view's own snap expression), inside the call that changed the sky offset
	 * and before OnSkyOffsetChanged is told. Every listener and every later reader of the frame (the star systems
	 * registering stars through the catalogue, the anchors, the GPU points, the key light, CanDeferTravel) then finds the
	 * sky where it is drawn, whatever their order, also on a settle from the world-less ticker after the world's tick. Rio
	 * 06.10 (audit: dead sky mover): the stellar view's own catalogue mover, which never bound, was removed; this function
	 * is the only catalogue mover (aps.Origin.SkyMovesCatalogue 1). Before it the catalogue moved only at the stellar tick,
	 * and stars the neighbour scan registered during owed flight stood one owed step (0.03-1 ly) off their drawn point;
	 * with aps.Origin.SkyMovesCatalogue 0 that is still so. The snap now finds it in
	 * place (Equals 0.01: no second move). ExpectedMove is the move it takes when it stood in place before (the owed step;
	 * the owed travel back on a pay); more than that is logged (it had been left behind).
	 */
	void PlaceSkyCatalogue(const UAPSWorldOriginSubsystem& Origin, UWorld* World, const FVector& ExpectedMove,
		const TCHAR* Why, const bool bLogMove)
	{
		if (CVarSkyMovesCatalogue.GetValueOnGameThread() == 0 || !World)
		{
			return;
		}
		AAstroGenerator* Sky = SkyCatalogueGenerator(World);
		const AStarSystem* Home = Sky ? Sky->GetPreviewHomeSystem() : nullptr;
		// The stellar view snaps a consumed catalogue only (the generated and REAL SCALE games); a legacy one is left alone.
		if (!Sky || !IsValid(Home) || !Sky->GetCanonicalStellarProjectionDescriptor().bConsumedFinalizedDataset)
		{
			return;
		}
		// Left to the snap: a generator riding with the sky as a member moves with the members already, and one carrying the
		// home system (an authored map) would carry it along, its target chasing itself on every owed step.
		const int32 State = Origin.IsSkyMember(Sky) ? 1 : (Home->IsAttachedTo(Sky) ? 2 : 0);
		FSkyCatalogueCache& Cache = GSkyCatalogue;
		if (Cache.LoggedGenerator.Get() != Sky || Cache.LoggedState != State)
		{
			Cache.LoggedGenerator = Sky;
			Cache.LoggedState = State;
			if (State == 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] sky catalogue: %s moves to its sky place (home system + sky offset) with every owed step and pay, before the sky's listeners are told (aps.Origin.SkyMovesCatalogue 1)"),
					*Sky->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] sky catalogue: %s is left to the stellar view's snap (%s)"),
					*Sky->GetName(), State == 1 ? TEXT("it rides with the sky as a member") : TEXT("the home system hangs under it"));
			}
		}
		if (State != 0)
		{
			return;
		}
		const FVector Target = Home->GetActorLocation() + Origin.GetSkyOffset();
		const FVector Before = Sky->GetActorLocation();
		if (Before.Equals(Target, 0.01))
		{
			return;
		}
		constexpr double AstronomicalUnitCm = 1.495978707e13;
		const double BehindCm = FVector::Dist(Target - Before, ExpectedMove);
		const double Now = FPlatformTime::Seconds();
		if (BehindCm > 1.0e9 && Now - Cache.LastBehindLogSeconds >= 1.0)
		{
			Cache.LastBehindLogSeconds = Now;
			UE_LOG(LogTemp, Warning, TEXT("[APS.WorldOrigin] f=%llu sky catalogue left behind: %s was %.4g AU off its sky place before this %s (placed now)"),
				static_cast<unsigned long long>(GFrameCounter), *Sky->GetName(), BehindCm / AstronomicalUnitCm, Why);
		}
		Sky->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
		if (bLogMove)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] f=%llu sky catalogue: %s placed on the %s, moved %.4f ly"),
				static_cast<unsigned long long>(GFrameCounter), *Sky->GetName(), Why, (Target - Before).Size() / LightYearCm);
		}
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

	// Rio 06.10 (audit: static-destruction order): a namespace-static FTickableGameObject would unregister from
	// FTickableStatics at exit after that singleton is gone in a monolithic exe; it lives until the process ends instead.
	FFloatingOriginTicker* GFloatingOriginTicker = nullptr; // never freed: FTickableStatics' singleton dies before namespace statics in a monolithic exe

	FDelayedAutoRegisterHelper GFloatingOriginRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		if (!GFloatingOriginTicker)
		{
			GFloatingOriginTicker = new FFloatingOriginTicker();
		}
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
	AActor* Carrier, const TArray<AActor*>* AlsoCarried)
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
		// Rio 06.10 (still ship pays its debt): the systems riding with the sky keep their place through the shift too and
		// take their own small move after it (PayDebt), so they never move a light-month and back within one frame.
		if (AlsoCarried)
		{
			TArray<AActor*> Tree;
			for (AActor* Root : *AlsoCarried)
			{
				if (IsValid(Root))
				{
					Tree.Reset();
					Root->GetAttachedActors(Tree, true, true);
					Carried.Add(Root);
					Carried.Append(Tree);
				}
			}
		}
	}
	const auto IsCarried = [&Carried](const UActorComponent* Component)
	{
		return !Carried.IsEmpty() && Carried.Contains(Component->GetOwner());
	};
	// Rio 06.10 (flight FPS, aps.Origin.FlowCarrySky): the star catalogue a flow or a pay carries (FlowPastShip and PayDebt
	// put APSWorldOrigin::CarriedSky among AlsoCarried; the systems riding with the sky are never generators). Carried, it
	// always takes the shift as its own move after the levels' steps, never left a whole shift behind.
	AActor* CarriedSkyRoot = nullptr;
	if (!Carried.IsEmpty() && AlsoCarried)
	{
		for (AActor* Root : *AlsoCarried)
		{
			if (IsValid(Root) && Root->IsA<AAstroGenerator>())
			{
				CarriedSkyRoot = Root;
				break;
			}
		}
	}

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
	// Rio 06.10 (flight FPS 20-40 on the way out of a system, aps.Origin.FlowCarrySky): the star catalogue rode through the
	// levels' steps above and takes the shift now as an ordinary move, so its two instanced catalogues (the galaxy's 25k and
	// the cluster's 36k stars) keep their render state (not re-created below: two CreateSceneProxy, ~2.5 ms, every flow
	// frame) and their instance data takes the new primitive transform the way any move sends it. It ends exactly where the
	// shift would have put it (the stellar view keeps it on the home system, which shifted). The scene-wide shift already
	// moved its proxies there, so the move draws no motion of its own, and everything after this (the listeners of
	// OnPostDoubleShift below) finds the sky in place.
	if (CarriedSkyRoot)
	{
		CarriedSkyRoot->AddActorWorldOffset(Offset, false, nullptr, ETeleportType::TeleportPhysics);
		APSWorldOrigin::KeepSkyMotionStill(*CarriedSkyRoot);
		static TWeakObjectPtr<AActor> LoggedSky;
		if (LoggedSky.Get() != CarriedSkyRoot)
		{
			LoggedSky = CarriedSkyRoot;
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] flow carry sky: %s rides through the flow and pay shifts, moved after them (aps.Origin.FlowCarrySky 1)"),
				*CarriedSkyRoot->GetName());
		}
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

FVector UAPSWorldOriginSubsystem::FlowPastShip(AActor& Ship, const FVector& InDelta, const double SpeedCmPerS)
{
	UWorld* World = GetWorld();
	// Rio 06.10 (aps.RealScale.FlowKeepWalkerShip): the rest a kept flow frame left this ship owing comes with its next step
	// (whatever path takes it below: a flow, a plain move, an owed step). Another ship's step leaves it alone.
	FVector Delta = InDelta;
	APSWorldOrigin::FlowKeptResend.Reset();
	if (APSWorldOrigin::FlowKeptShip.Get() == &Ship)
	{
		Delta += APSWorldOrigin::FlowKeptRest;
		APSWorldOrigin::FlowKeptShip.Reset();
		APSWorldOrigin::FlowKeptRest = FVector::ZeroVector;
	}
	// Rio 06.10 (still ship): a ship owing travel pays it back as soon as the world may not stay still any more; while the
	// world cannot shift at all it keeps owing (and stays put).
	const bool bOwes = bTravelDeferred && DeferredCarrier.Get() == &Ship;
	const auto PayOrMove = [this, &Ship, &Delta, bOwes](const TCHAR* Reason)
	{
		FVector Rest = Delta;
		if (bOwes && !PayDebt(Ship, Delta, Rest, Reason))
		{
			OweTravel(Ship, Delta);
			return FVector::ZeroVector;
		}
		return Rest;
	};
	if (!World || APSWorldOrigin::CVarRealFlowWorld.GetValueOnGameThread() == 0
		|| APSWorldOrigin::CVarFloat.GetValueOnGameThread() == 0 || bRebaseDeferred || bMapShifted
		|| !APSRealScale::IsActive(World))
	{
		return PayOrMove(TEXT("the flow is off"));
	}
	const double FastCmPerS = FMath::Max(APSWorldOrigin::CVarRealFloatFastKmPerS.GetValueOnGameThread(), 0.0f) * 1.0e5;
	const double FastDriftCm = FMath::Max(APSWorldOrigin::CVarRealFloatFastDriftKm.GetValueOnGameThread(), 1.0f) * 1.0e5;
	if (FastCmPerS <= 0.0 || SpeedCmPerS <= FastCmPerS)
	{
		return PayOrMove(TEXT("below flow speed"));
	}
	// Only the ship that carries the player: piloted, or with the player walking aboard (attached to it).
	const APawn* Pawn = APSWorldOrigin::PlayerPawn(World);
	const USceneComponent* PawnRoot = Pawn ? Pawn->GetRootComponent() : nullptr;
	if (!PawnRoot || PawnRoot->GetAttachmentRootActor() != &Ship)
	{
		return PayOrMove(TEXT("the player is not aboard"));
	}
	// The map's own far view (TryMapShift) and a settling star catalogue keep the world still, as for every shift.
	const AGravityPlayerController* Controller = Cast<AGravityPlayerController>(Pawn->GetController());
	if (Controller && Controller->IsStrategicMapOpen())
	{
		return PayOrMove(TEXT("the map"));
	}
	if (bOwes && APSWorldOrigin::IsColonyTerminalOpen(World))
	{
		return PayOrMove(TEXT("the colony terminal"));
	}
	if (IsStellarCatalogueSettling())
	{
		if (bOwes)
		{
			OweTravel(Ship, Delta);
			return FVector::ZeroVector;
		}
		return Delta;
	}
	// Rio 06.10 (still ship): far from every star system the world stays where it is and the step is owed; only the sky
	// moves (OweTravel). Near one (the catalogue as the sky shows it) the debt is paid with this step's flow below.
	if ((bOwes || (Ship.GetActorLocation() + Delta).SizeSquared() > FMath::Square(FastDriftCm))
		&& CanDeferTravel(Ship, Delta))
	{
		OweTravel(Ship, Delta);
		return FVector::ZeroVector;
	}
	if (bOwes)
	{
		return PayOrMove(APSWorldOrigin::CVarFlowDefer.GetValueOnGameThread() != 0 ? TEXT("a star system ahead")
			: TEXT("aps.RealScale.FlowDefer 0"));
	}
	// The view rides this ship (a camera on it, or on the pilot walking its decks).
	const APlayerController* Player = World->GetFirstPlayerController();
	const AActor* ViewTarget = Player && Player->PlayerCameraManager ? Player->PlayerCameraManager->GetViewTarget() : nullptr;
	const USceneComponent* ViewRoot = ViewTarget ? ViewTarget->GetRootComponent() : nullptr;
	const bool bViewRides = APSWorldOrigin::CVarFlowViewStill.GetValueOnGameThread() != 0 && ViewRoot
		&& ViewRoot->GetAttachmentRootActor() == &Ship;
	// Where the step would end (rounded at its own size, only to pick the grains); a short way out it simply moves.
	const FVector Target = Ship.GetActorLocation() + Delta;
	if (Target.SizeSquared() <= FMath::Square(FastDriftCm))
	{
		// Rio 05.10 night ("the ship jerks from about 1 c up to some 10 c, then it stops"): on these frames the ship and its
		// view fly their own step, hundreds to 100 000 km, between flows a few times a second. The renderer took that as a
		// camera jump (a cut, or with the history kept a smear across it), alternating with the exact flow frames. A long
		// step now reads like a flow frame's: the view moved with the ship, and the ship's own move (MoveShipKinematic)
		// gives its parts and riders their previous transforms where they are (FinishFlowMove).
		const double RideCm = FMath::Max(APSWorldOrigin::CVarFlowRideKm.GetValueOnGameThread(), 0.0f) * 1.0e5;
		if (bViewRides && RideCm > 0.0 && Delta.SizeSquared() > FMath::Square(RideCm))
		{
			World->OriginOffsetThisFrame += Delta;
			bFlowViewStill = true;
			APSWorldOrigin::SetFlowViewStep(World, Delta);
			if (const double Now = FPlatformTime::Seconds(); Now - LastFlowViewLogSeconds >= 1.0)
			{
				LastFlowViewLogSeconds = Now;
				UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] flow ride: view=%s step %.1f km, %.0f km from 0,0,0"),
					*GetNameSafe(ViewTarget), Delta.Size() / 100000.0, Target.Size() / 100000.0);
			}
		}
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
	// Rio 06.10 (flight FPS, aps.Origin.FlowCarrySky): the star catalogue rides through the flow and is moved after it by
	// the same offset (ShiftWorldBy), instead of re-creating its two instanced catalogues on every flow frame.
	TArray<AActor*> Sky;
	if (AActor* Catalogue = APSWorldOrigin::CarriedSky(World))
	{
		Sky.Add(Catalogue);
	}
	if (Grains.IsZero() || Rest.IsNearlyZero(1.0e-3)
		|| !ShiftWorldBy(-Grains, TEXT("the world flows past a fast REAL SCALE ship"), true, &Ship,
			Sky.IsEmpty() ? nullptr : &Sky))
	{
		return Delta;
	}
	SinceFlowShiftSeconds = 0.0;
	// Rio 06.10 (walking aboard at speed, aps.RealScale.FlowKeepWalkerShip): the player walks aboard (the pawn is not the
	// ship) and the view rides it: the ship and its body stay where they are, the rest waits for its next step, and
	// FinishFlowMove sends the riders' render state again after giving them their previous transforms.
	// Rio 06.10 (review): only a ship within a grain of where this step would leave it. One that stands farther from 0,0,0
	// (it flew its own steps out to FloatFastDriftKm before this shift) would owe up to that whole way on every kept frame
	// and never be centred again; it takes this frame's move (centred within half a grain) and is kept from the next one,
	// so a kept rest never exceeds one grain on any axis.
	const bool bKeepShip = APSWorldOrigin::CVarFlowKeepWalkerShip.GetValueOnGameThread() != 0 && Pawn != &Ship
		&& bViewRides && Rest.GetAbsMax() <= Grain && APSWorldOrigin::CanForceRenderResend();
	// Rio 05.10 evening (edge ripple at speed): the view rides the ship, so between the frames it moved only by the ship's
	// Rest. The renderer moves its previous view by OriginOffsetThisFrame (UE 5.4 SceneVisibility.cpp): by the whole shift
	// it lay light years off, and in float maths nothing kept any TSR history (r.TSR.Visualize 0: nothing accumulated,
	// flying or aboard). Moved by Rest it is where the view is; the riders' velocity state, which the scene-wide shift
	// moved too, is reset after the ship's own move (FinishFlowMove). The world around gets no motion of its own from the
	// shift: far away it hardly moves on screen, and near things at these speeds pass in a frame or two.
	// Rio 06.10 (aps.RealScale.FlowKeepWalkerShip): a kept ship did not move at all, so neither did the view (Grains only).
	bFlowViewStill = bViewRides;
	const FVector ViewStep = bKeepShip ? Grains : Grains + Rest;
	if (bFlowViewStill)
	{
		World->OriginOffsetThisFrame += ViewStep;
		APSWorldOrigin::SetFlowViewStep(World, ViewStep);
	}
	if (const double Now = FPlatformTime::Seconds(); Now - LastFlowViewLogSeconds >= 1.0)
	{
		LastFlowViewLogSeconds = Now;
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] flow view: still=%d kept=%d view=%s rest %.1f m, shift %.3e cm, frame offset %s"),
			bFlowViewStill ? 1 : 0, bKeepShip ? 1 : 0, *GetNameSafe(ViewTarget), Rest.Size() / 100.0, Grains.Size(),
			*World->OriginOffsetThisFrame.ToCompactString());
	}
	if (bKeepShip)
	{
		APSWorldOrigin::FlowKeptShip = &Ship;
		APSWorldOrigin::FlowKeptRest = Rest;
		APSWorldOrigin::FlowKeptFrame = GFrameCounter;
		APSWorldOrigin::FlowKeptResend = &Ship;
		return FVector::ZeroVector;
	}
	return Rest;
}

UAPSWorldOriginSubsystem::FSkyOffsetChanged& UAPSWorldOriginSubsystem::OnSkyOffsetChanged()
{
	static FSkyOffsetChanged Delegate;
	return Delegate;
}

FVector UAPSWorldOriginSubsystem::SkyOffsetOf(const UWorld* World)
{
	const UAPSWorldOriginSubsystem* Origin = World ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
	return Origin ? Origin->GetSkyOffset() : FVector::ZeroVector;
}

void UAPSWorldOriginSubsystem::AddSkyMember(AActor* Root)
{
	if (!IsValid(Root))
	{
		return;
	}
	SkyMembers.RemoveAll([](const TWeakObjectPtr<AActor>& Member) { return !Member.IsValid(); });
	if (!SkyMembers.Contains(Root))
	{
		SkyMembers.Add(Root);
		DeferCheckMarginCm = -1.0;
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] still ship: %s rides with the sky (sky offset %s)"), *Root->GetName(),
			*GetSkyOffset().ToCompactString());
	}
}

void UAPSWorldOriginSubsystem::RemoveSkyMember(AActor* Root)
{
	SkyMembers.RemoveAll([Root](const TWeakObjectPtr<AActor>& Member) { return !Member.IsValid() || Member.Get() == Root; });
	DeferCheckMarginCm = -1.0;
}

bool UAPSWorldOriginSubsystem::IsSkyMember(const AActor* Actor) const
{
	if (!Actor || SkyMembers.IsEmpty())
	{
		return false;
	}
	const USceneComponent* Root = Actor->GetRootComponent();
	const AActor* Top = Root ? Root->GetAttachmentRootActor() : Actor;
	for (const TWeakObjectPtr<AActor>& Member : SkyMembers)
	{
		if (Member.Get() == Top || Member.Get() == Actor)
		{
			return true;
		}
	}
	return false;
}

bool UAPSWorldOriginSubsystem::IsInStillFrame(const AActor* Actor) const
{
	if (!Actor || !bTravelDeferred)
	{
		return false;
	}
	// One rule for every consumer (the 06.10 review): what the still frame already holds stands where it is for the still
	// ship and the sky offset away from its world place for anyone else: the still ship and what rides it, the systems
	// riding with the sky, the catalogue's anchors (FAPSStarSystems keeps them at the sky's places) and the sky itself
	// (whatever hangs under the gameplay AstroGenerator), each with whatever is attached to it.
	const USceneComponent* Root = Actor->GetRootComponent();
	const AActor* Top = Root && Root->GetAttachmentRootActor() ? Root->GetAttachmentRootActor() : Actor;
	FGuid AnchorId;
	return Top == DeferredCarrier.Get() || IsSkyMember(Top) || Top->IsA<AAstroGenerator>()
		|| FAPSStarSystems::AnchorSystem(Top, AnchorId);
}

FVector UAPSWorldOriginSubsystem::SkyPlaceOf(const AActor& Actor) const
{
	const FVector Place = Actor.GetActorLocation();
	return bTravelDeferred && !IsInStillFrame(&Actor) ? Place + GetSkyOffset() : Place;
}

FVector UAPSWorldOriginSubsystem::SkyPlace(const AActor& Actor)
{
	const UWorld* World = Actor.GetWorld();
	const UAPSWorldOriginSubsystem* Origin = World ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
	return Origin ? Origin->SkyPlaceOf(Actor) : Actor.GetActorLocation();
}

bool UAPSWorldOriginSubsystem::IsStillObserver(const AActor* Observer)
{
	const UWorld* World = Observer ? Observer->GetWorld() : nullptr;
	const UAPSWorldOriginSubsystem* Origin = World ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
	const AActor* Carrier = Origin && Origin->bTravelDeferred ? Origin->DeferredCarrier.Get() : nullptr;
	if (!Carrier)
	{
		return false;
	}
	const USceneComponent* Root = Observer->GetRootComponent();
	return Observer == Carrier || (Root && Root->GetAttachmentRootActor() == Carrier);
}

FVector UAPSWorldOriginSubsystem::SkyOffsetFor(const AActor* Observer)
{
	return IsStillObserver(Observer) ? SkyOffsetOf(Observer->GetWorld()) : FVector::ZeroVector;
}

FVector UAPSWorldOriginSubsystem::WorldPlace(const AActor& Target)
{
	const UWorld* World = Target.GetWorld();
	const UAPSWorldOriginSubsystem* Origin = World ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
	const FVector Place = Target.GetActorLocation();
	if (!Origin || !Origin->bTravelDeferred)
	{
		return Place;
	}
	// What the still frame holds stands the sky offset away from its world place (IsInStillFrame).
	return Origin->IsInStillFrame(&Target) ? Place - Origin->GetSkyOffset() : Place;
}

FVector UAPSWorldOriginSubsystem::PlaceFor(const AActor* Observer, const AActor& Target)
{
	return IsStillObserver(Observer) ? SkyPlace(Target) : WorldPlace(Target);
}

FVector UAPSWorldOriginSubsystem::CataloguePlaceFor(const AActor* Observer, const FVector& SkyPlace)
{
	const UWorld* World = Observer ? Observer->GetWorld() : nullptr;
	return IsStillObserver(Observer) ? SkyPlace : SkyPlace - SkyOffsetOf(World);
}

bool UAPSWorldOriginSubsystem::CanDeferTravel(const AActor& Ship, const FVector& Delta)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_StillShip_Check);
	if (APSWorldOrigin::CVarFlowDefer.GetValueOnGameThread() == 0 || APSWorldOrigin::IsColonyTerminalOpen(GetWorld()))
	{
		DeferCheckMarginCm = -1.0;
		return false;
	}
	// The ship stands where it is; the catalogue, as the sky shows it (at world + sky offset), is what it flies among.
	// A system or a drawn star nearer than the clear distance plus two steps ahead ends the debt before the ship gets in.
	const double BaseClearCm = FMath::Max(APSWorldOrigin::CVarDeferClearLy.GetValueOnGameThread(), 0.0f)
		* APSWorldOrigin::LightYearCm;
	const double StepsCm = 2.0 * Delta.Size();
	// Rio 06.10 ("a ship flying ahead of me must be seen where it is"): the world's own movers (other ships, stations) are
	// drawn at their world place, which a still ship would see the debt away from where they truly are: near one of them
	// the world flows past as before. They move on their own, so this is checked every step (a few dozen actors at most).
	if (const double ShipClearCm = FMath::Max(APSWorldOrigin::CVarDeferShipClearKm.GetValueOnGameThread(), 0.0f) * 1.0e5;
		ShipClearCm > 0.0)
	{
		const FVector Truly = Ship.GetActorLocation() - GetSkyOffset();
		const double ClearSquared = FMath::Square(ShipClearCm + StepsCm);
		const auto Near = [&Ship, &Truly, ClearSquared](const AActor* Other)
		{
			const USceneComponent* Root = Other ? Other->GetRootComponent() : nullptr;
			return Other && Other != &Ship && !(Root && Root->GetAttachmentRootActor() == &Ship)
				&& FVector::DistSquared(Other->GetActorLocation(), Truly) < ClearSquared;
		};
		for (TActorIterator<ASpaceship> It(GetWorld()); It; ++It)
		{
			if (Near(*It))
			{
				DeferCheckMarginCm = -1.0;
				return false;
			}
		}
		for (TActorIterator<ASpaceStation> It(GetWorld()); It; ++It)
		{
			if (Near(*It))
			{
				DeferCheckMarginCm = -1.0;
				return false;
			}
		}
	}
	const UWorld* World = GetWorld();
	const FVector Here = Ship.GetActorLocation();
	const double ClearCm = BaseClearCm + StepsCm;
	// Rio 06.10 (still ship while braking): a system riding with the sky does not end the debt at the clear distance;
	// its bodies do, a number of their radii out, and never inside the arrival forecast's reach of a planet's surface
	// (aps.Surface.ArrivalReachFactor x its activation radius: no surface starts while the debt lasts). Its catalogue entry
	// and drawn star stand where its root is. Checked on every step: its planets stand up over the frames after its star.
	const double BodyRadii = FMath::Max(APSWorldOrigin::CVarDeferBodyClearRadii.GetValueOnGameThread(), 0.0f);
	const double BodyFloorCm = FMath::Max(APSWorldOrigin::CVarDeferBodyClearKm.GetValueOnGameThread(), 0.0f) * 1.0e5;
	static IConsoleVariable* const ArrivalReach = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.ArrivalReachFactor"));
	const double ReachFactor = ArrivalReach ? FMath::Max(ArrivalReach->GetFloat(), 1.0f) : 16.0;
	TArray<FVector, TInlineAllocator<4>> Standing;
	if (BodyRadii > 0.0)
	{
		TArray<AActor*> Bodies;
		for (const TWeakObjectPtr<AActor>& Member : SkyMembers)
		{
			AActor* Root = Member.Get();
			if (!IsValid(Root))
			{
				continue;
			}
			Standing.Add(Root->GetActorLocation());
			Bodies.Reset();
			Root->GetAttachedActors(Bodies, true, true);
			for (const AActor* Body : Bodies)
			{
				double NeedCm = 0.0;
				if (const APlanetaryBody* Planet = Cast<APlanetaryBody>(Body))
				{
					NeedCm = FMath::Max(BodyRadii * Planet->GetWorldScapeBodyRadiusCm(),
						ReachFactor * Planet->GetWorldScapeActivationRadiusCm());
				}
				else if (const AStar* Star = Cast<AStar>(Body))
				{
					NeedCm = BodyRadii * (Star->RadiusKM > 0.0 ? Star->RadiusKM : static_cast<double>(Star->StarRadiusKM)) * 1.0e5;
				}
				else
				{
					continue;
				}
				if (FVector::Dist(Body->GetActorLocation(), Here) - FMath::Max(NeedCm, BodyFloorCm) < StepsCm)
				{
					DeferCheckMarginCm = -1.0;
					return false;
				}
			}
		}
	}
	// Rio 06.10 (still ship, flight FPS): the catalogue (tens of thousands of systems) is searched again only once the
	// ship has flown the margin the last search found; until then nothing can have come within the clear distance.
	const FVector Owed = OwedGrains + OwedRest;
	if (bTravelDeferred && DeferCheckMarginCm > 0.0
		&& FVector::Dist(Owed, DeferCheckOwed) + StepsCm < DeferCheckMarginCm)
	{
		return true;
	}
	// Rio 06.10 (flight FPS: APS_StillShip_Check took 2.6 ms a frame on the fast way out, inside 0.5 ly of home): the home
	// system always counts in the search below (it never stands aside), so while it is nearer than the clear distance the
	// answer is no whatever else is near. The catalogue's search is skipped; the result is the same.
	const FAPSStarSystems* Systems = APSStarSystemsFind(World);
	if (const FAPSStarSystemInfo* Home = Systems ? Systems->GetHome() : nullptr;
		Home && FVector::Dist(Home->Location, Here) < ClearCm)
	{
		DeferCheckMarginCm = -1.0;
		return false;
	}
	const auto IsStanding = [&Standing](const FVector& Location)
	{
		return Standing.ContainsByPredicate([&Location](const FVector& Root)
		{
			return FVector::DistSquared(Root, Location) < FMath::Square(1.0e9);
		});
	};
	// The nearest system or drawn star within a few clear distances: how far the ship may fly before the next search.
	double NearestCm = 4.0 * BaseClearCm + StepsCm;
	if (Systems)
	{
		TArray<int32> Nearest;
		Systems->FindNearest(Here, 4, Nearest);
		for (const int32 Index : Nearest)
		{
			if (const FAPSStarSystemInfo* Near = Systems->Get(Index); Near && !IsStanding(Near->Location))
			{
				NearestCm = FMath::Min(NearestCm, FVector::Dist(Near->Location, Here));
				break;
			}
		}
		if (const FAPSStarSystemInfo* Home = Systems->GetHome())
		{
			NearestCm = FMath::Min(NearestCm, FVector::Dist(Home->Location, Here));
		}
	}
	TArray<APSGalaxyGpuStars::FNearStar> Stars;
	if (APSGalaxyGpuStars::FindNearStars(World, Here, 4, NearestCm, Stars))
	{
		for (const APSGalaxyGpuStars::FNearStar& Star : Stars)
		{
			if (!IsStanding(Star.WorldLocation))
			{
				NearestCm = FMath::Min(NearestCm, Star.DistanceCm);
				break;
			}
		}
	}
	if (NearestCm < ClearCm)
	{
		DeferCheckMarginCm = -1.0;
		return false;
	}
	DeferCheckOwed = Owed;
	DeferCheckMarginCm = NearestCm - BaseClearCm;
	return true;
}

void UAPSWorldOriginSubsystem::OweTravel(AActor& Ship, const FVector& Delta)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_StillShip_Owe);
	// Exact: whole grains add up without rounding, the rest stays within half a grain.
	const double Grain = GetOriginOffset().GetAbsMax() > APSWorldOrigin::RealShiftFarCm
		? APSWorldOrigin::RealShiftFarGrainCm : APSWorldOrigin::RealShiftGrainCm;
	const FVector Total = OwedRest + Delta;
	const FVector Grains(FMath::RoundToDouble(Total.X / Grain) * Grain, FMath::RoundToDouble(Total.Y / Grain) * Grain,
		FMath::RoundToDouble(Total.Z / Grain) * Grain);
	OwedGrains += Grains;
	OwedRest = Total - Grains;
	if (!bTravelDeferred)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] still ship: the world stays from here, %s carries the view, the sky moves"),
			*Ship.GetName());
	}
	bTravelDeferred = true;
	DeferredCarrier = &Ship;
	++DeferredSteps;
	// Still the fast flight for everything that waits for it to end (the frozen technology, the Nanite re-file).
	SinceFlowShiftSeconds = 0.0;
	MoveSky(-Delta, Ship);
	if (const double Now = FPlatformTime::Seconds(); Now - LastDeferLogSeconds >= 1.0)
	{
		LastDeferLogSeconds = Now;
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] still ship: owed %.4f ly over %d steps (last %.4f ly), sky offset %s"),
			(OwedGrains + OwedRest).Size() / APSWorldOrigin::LightYearCm, DeferredSteps,
			Delta.Size() / APSWorldOrigin::LightYearCm, *GetSkyOffset().ToCompactString());
	}
}

void UAPSWorldOriginSubsystem::MoveSky(const FVector& Change, AActor& StillShip)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_StillShip_MoveSky);
	UWorld* World = GetWorld();
	if (!World || Change.IsZero())
	{
		return;
	}
	// The still ship's flight model keeps the generated stars' centres as it sees them (catalogue places): they move with
	// the sky. Every other ship lives in the world's frame, where nothing moved (Rio 06.10: "the other ships must not
	// break on their flights").
	if (const ASpaceship* Still = Cast<ASpaceship>(&StillShip); Still && Still->FlightModel)
	{
		Still->FlightModel->ApplyWorldShift(Change);
	}
	// Rio 06.10 (still ship while braking): the star systems riding with the sky, each moved as one tree.
	for (const TWeakObjectPtr<AActor>& Member : SkyMembers)
	{
		if (AActor* Root = Member.Get(); IsValid(Root))
		{
			Root->AddActorWorldOffset(Change, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	// Rio 06.10 (aps.Origin.SkyMovesCatalogue): the star catalogue stands at its sky place before anyone is told.
	APSWorldOrigin::PlaceSkyCatalogue(*this, World, Change, TEXT("owed step"), false);
	OnSkyOffsetChanged().Broadcast(World, Change);
}

bool UAPSWorldOriginSubsystem::PayDebt(AActor& Carrier, const FVector& Delta, FVector& OutRest, const TCHAR* Reason)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_StillShip_Pay);
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const double Grain = GetOriginOffset().GetAbsMax() > APSWorldOrigin::RealShiftFarCm
		? APSWorldOrigin::RealShiftFarGrainCm : APSWorldOrigin::RealShiftGrainCm;
	// The owed whole grains, plus the grains of where the carrier's own small rest and this step would take it: the
	// world shifts by all of them, the carrier moves by what is left (within half a grain of 0,0,0), exactly as a flow.
	const FVector Local = Carrier.GetActorLocation() + Delta + OwedRest;
	FVector LocalGrains(FMath::RoundToDouble(Local.X / Grain) * Grain, FMath::RoundToDouble(Local.Y / Grain) * Grain,
		FMath::RoundToDouble(Local.Z / Grain) * Grain);
	FVector Rest = Delta + OwedRest - LocalGrains;
	// The carrier must move after the shift (its move re-sends the proxies the scene shift moved): never by nothing.
	if (Rest.IsNearlyZero(1.0))
	{
		LocalGrains.X -= Grain;
		Rest.X += Grain;
	}
	const FVector Grains = OwedGrains + LocalGrains;
	const FVector Owed = OwedGrains + OwedRest;
	// The systems riding with the sky stand at their world place less the owed travel; the shift leaves them where they
	// are and they then move by OwedRest - LocalGrains (owed - grains, exactly) to their world place: a few km, no
	// light-month there and back within the frame.
	const FVector MemberMove = OwedRest - LocalGrains;
	TArray<AActor*> Members;
	for (const TWeakObjectPtr<AActor>& Member : SkyMembers)
	{
		if (AActor* Root = Member.Get(); IsValid(Root))
		{
			Members.Add(Root);
		}
	}
	// Rio 06.10 (the review: the key light pointed the whole debt away after a pay): the debt is cleared before the
	// shift, so whatever its listeners read (the key light, the clouds) is in the one frame already; restored when the
	// world cannot shift now.
	const FVector SavedGrains = OwedGrains;
	const FVector SavedRest = OwedRest;
	const TWeakObjectPtr<AActor> SavedCarrier = DeferredCarrier;
	const int32 Steps = DeferredSteps;
	OwedGrains = FVector::ZeroVector;
	OwedRest = FVector::ZeroVector;
	bTravelDeferred = false;
	DeferredCarrier.Reset();
	DeferredSteps = 0;
	DeferCheckMarginCm = -1.0;
	// Rio 06.10 (flight FPS, aps.Origin.FlowCarrySky): the star catalogue rides through the pay shift as through a flow
	// (ShiftWorldBy moves it after the levels' steps) and comes back by the owed travel below (OnSkyOffsetChanged), its render
	// state kept. Only while the engine takes the previous transforms given (r.MotionVectorSimulation, switched on by the
	// flow frames before any debt): its second move of the frame would otherwise be drawn as a light-years' motion; without
	// it the catalogue shifts with the world and is re-created, as before.
	TArray<AActor*> Riding = Members;
	AActor* const Sky = FMotionVectorSimulation::IsEnabled() ? APSWorldOrigin::CarriedSky(World) : nullptr;
	if (Sky)
	{
		Riding.Add(Sky);
	}
	if (!ShiftWorldBy(-Grains, TEXT("the still ship pays its owed travel"), false, &Carrier, &Riding))
	{
		OwedGrains = SavedGrains;
		OwedRest = SavedRest;
		bTravelDeferred = true;
		DeferredCarrier = SavedCarrier;
		DeferredSteps = Steps;
		return false;
	}
	SinceFlowShiftSeconds = 0.0;
	if (!MemberMove.IsZero())
	{
		FMotionVectorSimulation& Vectors = FMotionVectorSimulation::Get();
		TArray<AActor*> Tree;
		for (AActor* Root : Members)
		{
			Root->AddActorWorldOffset(MemberMove, false, nullptr, ETeleportType::TeleportPhysics);
			// Their own small move this frame is no motion to draw (they were carried through the scene's shift).
			Tree.Reset();
			Root->GetAttachedActors(Tree, true, true);
			Tree.Add(Root);
			for (AActor* Part : Tree)
			{
				Part->ForEachComponent<UPrimitiveComponent>(false, [&Vectors](UPrimitiveComponent* Primitive)
				{
					if (Primitive->IsRegistered() && Primitive->SceneProxy)
					{
						Vectors.SetPreviousTransform(Primitive, Primitive->GetComponentTransform());
					}
				});
			}
		}
	}
	// The still ship's flight model kept the stars' centres as it saw them (world - owed); the shift took the grains.
	if (const ASpaceship* Still = Cast<ASpaceship>(&Carrier); Still && Still->FlightModel)
	{
		Still->FlightModel->ApplyWorldShift(Owed);
	}
	// Rio 06.10 (aps.Origin.SkyMovesCatalogue): the stellar view's generator comes back by the owed travel here, before the
	// broadcast, so its listeners and the rest of the frame (a settle's drawn frame too) find the sky in place; the carried
	// sky's KeepSkyMotionStill below then covers this move as well.
	APSWorldOrigin::PlaceSkyCatalogue(*this, World, Owed, TEXT("pay"), true);
	// The sky (the stellar view's generator, the GPU points, the anchors, the key light) comes back by the owed travel,
	// and the shift's listeners (clouds, materials) read the members' final places too.
	OnSkyOffsetChanged().Broadcast(World, Owed);
	// The carried catalogue's two moves this frame (the shift's grains, the owed travel back) are no motion to draw, as
	// when it was re-created; given again after the second one (the last given in a frame wins).
	if (Sky)
	{
		APSWorldOrigin::KeepSkyMotionStill(*Sky);
	}
	APSWorldShiftEvents::OnPostDoubleShift().Broadcast(World, MemberMove);
	// The view stays with the carrier, as on a flow frame (FinishFlowMove after the carrier's move).
	const APlayerController* Player = World->GetFirstPlayerController();
	const AActor* ViewTarget = Player && Player->PlayerCameraManager ? Player->PlayerCameraManager->GetViewTarget() : nullptr;
	const USceneComponent* ViewRoot = ViewTarget ? ViewTarget->GetRootComponent() : nullptr;
	bFlowViewStill = APSWorldOrigin::CVarFlowViewStill.GetValueOnGameThread() != 0 && ViewRoot
		&& ViewRoot->GetAttachmentRootActor() == &Carrier;
	if (bFlowViewStill)
	{
		World->OriginOffsetThisFrame += Grains + Rest;
		APSWorldOrigin::SetFlowViewStep(World, Delta);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] still ship: paid %.4f ly owed over %d steps (%s), %s moves %.1f m"),
		Owed.Size() / APSWorldOrigin::LightYearCm, Steps, Reason, *Carrier.GetName(), Rest.Size() / 100.0);
	OutRest = Rest;
	return true;
}

void UAPSWorldOriginSubsystem::SettleDeferredTravel(const TCHAR* Reason)
{
	// Rio 06.10 (aps.RealScale.FlowKeepWalkerShip): a save, a load or a rebase finds the kept ship where it truly is.
	if (APSWorldOrigin::FlowKeptShip.IsValid() || !APSWorldOrigin::FlowKeptRest.IsZero())
	{
		APSWorldOrigin::PayKeptFlowRest(GetWorld(), Reason);
	}
	if (!bTravelDeferred)
	{
		return;
	}
	UWorld* World = GetWorld();
	AActor* Carrier = DeferredCarrier.Get();
	if (!IsValid(Carrier))
	{
		// The ship is gone: whoever rode it (the player) stands for it.
		Carrier = APSWorldOrigin::PlayerPawn(World);
	}
	FVector Rest = FVector::ZeroVector;
	if (IsValid(Carrier) && PayDebt(*Carrier, FVector::ZeroVector, Rest, Reason))
	{
		Carrier->AddActorWorldOffset(Rest, false, nullptr, ETeleportType::None);
		FinishFlowMove(*Carrier);
		// Rio 06.10 (the review): a settle can come after this frame's camera update (from the tickable, a save, an exit);
		// the view riding the carrier is taken again from where it is now.
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
		const AActor* ViewTarget = Camera ? Camera->GetViewTarget() : nullptr;
		const USceneComponent* ViewRoot = ViewTarget ? ViewTarget->GetRootComponent() : nullptr;
		if (Camera && ViewRoot && ViewRoot->GetAttachmentRootActor() == Carrier)
		{
			Camera->UpdateCamera(0.0f);
		}
	}
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
	// Rio 06.10 (aps.RealScale.FlowKeepWalkerShip): the kept ship made no move that would send its riders' transforms; they
	// are sent now, where they are, with the previous transforms just given (no motion drawn, as after a ship's own step).
	if (APSWorldOrigin::FlowKeptResend.Get() == &Ship)
	{
		APSWorldOrigin::FlowKeptResend.Reset();
		APSWorldOrigin::ForceRenderResend(Riders);
	}
	// Rio 06.10 (aps.RealScale.FlowBodyParallax): a nearby star's photosphere stood one view step further along a frame
	// ago, as seen from the still view (ride: S -> S + Step; shift: S - Grains -> S - Grains + Step; a pay alike). Its
	// corona is additive and writes no velocity. A star that hardly moves on screen, or one that passes in a frame, is left.
	const FVector Step = APSWorldOrigin::FlowViewStepWorld.Get() == GetWorld() ? APSWorldOrigin::FlowViewStep : FVector::ZeroVector;
	APSWorldOrigin::SetFlowViewStep(nullptr, FVector::ZeroVector);
	if (!Step.IsZero() && APSWorldOrigin::CVarFlowBodyParallax.GetValueOnGameThread() != 0)
	{
		// An unmoved or shift-moved star is a "redundant" transform update in UE 5.4 (RendererScene.cpp ~2142) and would
		// never read the simulated previous transform: the check is off while these are sent.
		// Rio 06.10 (audit: every Set ran all console variable sinks next engine tick, on every near-system flow frame): it is
		// switched off only once a star is actually sent, without the sink call (no sink reads it; the renderer reads the
		// plain int in FScene::UpdatePrimitiveTransform on the game thread), and only when code may set it at all (a
		// console-set value outranks SetByCode: it was ignored with a warning before, it is left alone now).
		static IConsoleVariable* const SkipRedundant = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkipRedundantTransformUpdate"));
		bool bSkipOff = false;
		const bool bMayToggle = SkipRedundant && SkipRedundant->GetInt() != 0
			&& (uint32(SkipRedundant->GetFlags()) & ECVF_SetByMask) <= uint32(ECVF_SetByCode);
		const EConsoleVariableFlags QuietCode = EConsoleVariableFlags(ECVF_SetByCode | ECVF_Set_NoSinkCall_Unsafe);
		const int32 SavedSkip = bMayToggle ? SkipRedundant->GetInt() : 0;
		const FVector View = Ship.GetActorLocation();
		for (TActorIterator<AStar> It(GetWorld()); It; ++It)
		{
			UStaticMeshComponent* Body = It->StarMesh;
			if (!IsValid(Body) || !Body->IsRegistered() || !Body->SceneProxy || !Body->IsVisible())
			{
				continue;
			}
			const double Gap = FMath::Max(FVector::Dist(Body->Bounds.Origin, View) - Body->Bounds.SphereRadius, 1.0e5);
			const double Parallax = Step.Size() / Gap;
			if (Parallax < 2.0e-5 || Parallax > 0.5)
			{
				continue;
			}
			if (bMayToggle && !bSkipOff)
			{
				SkipRedundant->Set(0, QuietCode);
				bSkipOff = true;
			}
			FTransform Previous = Body->GetComponentTransform();
			Previous.AddToTranslation(Step);
			Vectors.SetPreviousTransform(Body, Previous);
			Body->MarkRenderTransformDirty();
			Body->DoDeferredRenderUpdates_Concurrent();
		}
		if (bSkipOff)
		{
			SkipRedundant->Set(SavedSkip, QuietCode);
		}
	}
}

void UAPSWorldOriginSubsystem::UpdateFloatingOrigin(const float DeltaSeconds)
{
	SinceFloatShiftSeconds += DeltaSeconds;
	SinceFlowShiftSeconds += DeltaSeconds;
	// Rio 06.10 (still ship): a debt nobody adds to any more (the carrier is gone or stopped flying fast without a move
	// through FlowPastShip) is paid now; so is one under the strategic map or with the experiment switched off.
	if (bTravelDeferred)
	{
		const APawn* Player = APSWorldOrigin::PlayerPawn(GetWorld());
		const AGravityPlayerController* Owner = Player ? Cast<AGravityPlayerController>(Player->GetController()) : nullptr;
		if (!DeferredCarrier.IsValid() || SinceFlowShiftSeconds > 0.25 || (Owner && Owner->IsStrategicMapOpen())
			|| APSWorldOrigin::IsColonyTerminalOpen(GetWorld()) || APSWorldOrigin::CVarFlowDefer.GetValueOnGameThread() == 0)
		{
			SettleDeferredTravel(TEXT("the owed travel stopped growing"));
		}
	}
	// Rio 06.10 (aps.RealScale.FlowKeepWalkerShip): a kept rest that no step of its ship took by the next frame (the ship
	// stopped, a swept move, a world without flow) is paid now with a plain move; a paused world keeps it for the next step.
	if ((APSWorldOrigin::FlowKeptShip.IsValid() || !APSWorldOrigin::FlowKeptRest.IsZero())
		&& GFrameCounter > APSWorldOrigin::FlowKeptFrame && !(GetWorld() && GetWorld()->IsPaused()))
	{
		APSWorldOrigin::PayKeptFlowRest(GetWorld(), TEXT("no step took it"));
	}
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
	// Rio 06.10 (still ship): any other shift starts from a world that owes nothing. The settle moves the player's pawn,
	// whose place the caller read before it: the target follows that move (the review).
	const APawn* SettledPawn = APSWorldOrigin::PlayerPawn(World);
	const FVector PawnBeforeSettle = SettledPawn ? SettledPawn->GetActorLocation() : FVector::ZeroVector;
	SettleDeferredTravel(TEXT("a rebase"));
	const FVector RebaseLocation = WorldLocation
		+ (SettledPawn ? SettledPawn->GetActorLocation() - PawnBeforeSettle : FVector::ZeroVector);
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
	return RebaseNow(RebaseLocation, Reason);
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
