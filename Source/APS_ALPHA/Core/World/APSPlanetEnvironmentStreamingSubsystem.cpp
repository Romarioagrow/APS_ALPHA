#include "APSPlanetEnvironmentStreamingSubsystem.h"
#include "APS_ALPHA/Core/World/APSPlaceholderGlobe.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetReliefRuntime.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeStreamingPolicy.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
// Rio 06.10 (takeoff hitches): WorldScapeCore public header; CurrentBytes() is WORLDSCAPECORE_API (aps.Surface.PublishHoldFrames).
#include "WorldScapePreparedMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSWorldScapeStreaming, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarMaxStandbyRoots(
		TEXT("aps.Surface.MaxStandbyRoots"), 4,
		TEXT("Maximum hidden, unpublished sibling roots prepared ahead of travel (0..32). ")
		TEXT("Never evicts the selected body or an already published surface. Not a total GPU memory cap."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarMapCameraObserver(
		TEXT("aps.Map.WorldScapeObserver"), 1,
		TEXT("1: while the F10 strategic map is open, its camera is the resident planet's WorldScape visual observer, so ")
		TEXT("the planet builds for the map's view (collision stays with the pawn). 0: the terrain stays around the pawn."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarMapObserverRadii(
		TEXT("aps.Map.WorldScapeObserverRadii"), 30.0f,
		TEXT("Rio 03.10 (F10 near a planet: ~250 ms render hitches every few seconds while WorldScape rebuilt its LODs for a map ")
		TEXT("camera far out at cluster scale): the map camera becomes the terrain's observer only within this many planet radii; ")
		TEXT("farther out the globe is a few pixels and stays built around the pawn. 0: at any distance (the old rule)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarFarFreezeRadii(
		TEXT("aps.Surface.FarFreezeRadii"), 10.0f,
		TEXT("Rio 03.10 (flight stutter: over 98% of in-flight hitch time was WorldScape rebuilding the active planet's LOD ")
		TEXT("batches): beyond this many planet radii the terrain's visual observer only moves once the view from the planet's ")
		TEXT("centre has turned by aps.Surface.FarFreezeTurnDeg (WorldScape's own far rule, switched off by our ")
		TEXT("DistanceToFreezeGeneration = 0). 0: the observer follows every frame."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarFarFreezeTurnDeg(
		TEXT("aps.Surface.FarFreezeTurnDeg"), 18.0f,
		TEXT("See aps.Surface.FarFreezeRadii: the turn, in degrees, that moves a far observer."),
		ECVF_Default);

	bool IsExplicitMenuPreviewBody(const AActor* Actor)
	{
		for (const AActor* Parent = Actor; IsValid(Parent); Parent = Parent->GetAttachParentActor())
		{
			if (Parent->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return true;
			}
		}
		return false;
	}
}

// Rio 06.10 (planet freezes): the surface reveal is spread over frames by the body itself (PlanetaryBodyStreaming.cpp);
// this subsystem only advances it every frame.
namespace APSWorldScapeReveal
{
	void AdvanceStagedReveals(UWorld* World);
}

// Rio 06.10 (planet freezes on foot, on arrival and on departure): fewer LOD batches and fewer surface switches. Each
// rule has its own console variable; 0 restores the 05.10 behaviour of that rule.
namespace APSSurfacePolicyPrivate
{
	TAutoConsoleVariable<float> CVarNearObserverStepCm(
		TEXT("aps.Surface.NearObserverStepCm"), 800.0f,
		TEXT("Rio 06.10 (stutter on foot): near the ground the terrain's visual observer moves only once the pawn is this far ")
		TEXT("from it. WorldScape re-centres its finest ring every 1.2 m, so walking published ~3.4 LOD batches a second, each ")
		TEXT("followed by render stalls; in 8 m steps it is about one a second. Collision follows the pawn on its own. ")
		TEXT("0: the observer follows every frame (the old way)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarNearObserverAltitudeKm(
		TEXT("aps.Surface.NearObserverAltitudeKm"), 2.0f,
		TEXT("See aps.Surface.NearObserverStepCm: the step applies only this close to the ground (km above the terrain)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarActiveSwitchMinSeconds(
		TEXT("aps.Surface.ActiveSwitchMinSeconds"), 3.0f,
		TEXT("Rio 06.10 (A->B->A surface thrash): another body of the family takes over the active surface only once it has been ")
		TEXT("the nearest for this many seconds, unless the observer is within aps.Surface.ActiveSwitchNearRadii of it or flies ")
		TEXT("at it. Each switch rebuilt a surface (50-84 ms hitches; 4 switches in 14 s while leaving a planet). 0: at once."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarActiveSwitchNearRadii(
		TEXT("aps.Surface.ActiveSwitchNearRadii"), 4.0f,
		TEXT("Within this many of its own radii from its centre a body takes over the active surface at once ")
		TEXT("(aps.Surface.ActiveSwitchMinSeconds, aps.Surface.PassByKmPerS)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarPassByKmPerS(
		TEXT("aps.Surface.PassByKmPerS"), 20.0f,
		TEXT("Rio 06.10 (activations of bodies only flown past): a body the observer passes faster than this (km/s), on a line ")
		TEXT("missing its centre by more than aps.Surface.PassByMissRadii, is neither activated nor started ahead of arrival. ")
		TEXT("0: off (the old way)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarPassByMissRadii(
		TEXT("aps.Surface.PassByMissRadii"), 6.0f,
		TEXT("See aps.Surface.PassByKmPerS: a line passing within this many body radii of the centre is an approach."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarLeaveFreezeKmPerS(
		TEXT("aps.Surface.LeaveFreezeKmPerS"), 20.0f,
		TEXT("Rio 06.10 (freezes on departure): while the observer recedes from the active body faster than this (km/s), the ")
		TEXT("terrain keeps its observer (as the far freeze does), so a planet being left is not rebuilt: WorldScape rebuilt ")
		TEXT("every LOD at each doubling of altitude, and again when a frozen surface resumed on the way out. Only once the kept ")
		TEXT("observer is high enough that WorldScape's altitude scale no longer grows; resumes when slower or approaching. 0: off."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarLeaveFreezeHoldTurnDeg(
		TEXT("aps.Surface.LeaveFreezeHoldTurnDeg"), 0.0f,
		TEXT("Rio 06.10 (takeoff: the planet was rebuilt again once the leave freeze let go at 14 km/s, hitches 54/43/312 ms): ")
		TEXT("a leave freeze already on also holds while the observer is still above the kept observer, is not approaching ")
		TEXT("faster than a tenth of aps.Surface.LeaveFreezeKmPerS, and the view from the planet's centre has turned less than ")
		TEXT("this many degrees from the kept observer. Never while the F10 map camera is the observer. 0: off (lets go below ")
		TEXT("0.8x aps.Surface.LeaveFreezeKmPerS, as before)."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarPublishHoldFrames(
		TEXT("aps.Surface.PublishHoldFrames"), 0,
		TEXT("Rio 06.10 (takeoff from an ocean world, 59-80 fps, 1-2 hitches a second): one land+ocean LOD wave nearly fills ")
		TEXT("WorldScape's prepared-mesh budget, and its reservations live on in render commands while the next wave reserves, ")
		TEXT("so that wave falls back to the slow legacy upload. Above 300 m, while prepared reservations are live, the visual ")
		TEXT("observer is held during a wave's generation and for up to this many frames after it (at most 30). 0: off (as before)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarFastObserverKmPerS(
		TEXT("aps.Surface.FastObserverKmPerS"), 0.0f,
		TEXT("Rio 09.10 (4K, autopilot 80-650 km over a planet at 300-800 km/s: a 90-105 ms render-thread and a 40-55 ms ")
		TEXT("game-thread hitch about every second, the LOD waves of an observer moved every frame): above ")
		TEXT("aps.Surface.NearObserverAltitudeKm an observer faster than this (km/s; its ship's speed when aboard) moves at most ")
		TEXT("every aps.Surface.FastObserverSeconds. 0: off, it follows every frame (as before)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarFastObserverSeconds(
		TEXT("aps.Surface.FastObserverSeconds"), 0.5f,
		TEXT("See aps.Surface.FastObserverKmPerS: the shortest time between two moves of a fast observer."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarMapKeepObserverOnPass(
		TEXT("aps.Map.KeepObserverOnPass"), 1,
		TEXT("Rio 06.10 review: while the F10 map camera is the WorldScape observer (aps.Map.WorldScapeObserver/Radii), the ")
		TEXT("half-second pass keeps it instead of writing the pawn for one frame (two full LOD waves and collision resets each ")
		TEXT("cycle). 0: the old behaviour."),
		ECVF_Default);

	/** The observer's motion relative to one streamed body between two selection passes. */
	struct FPassMotion
	{
		double SpeedCmPerS{0.0};
		/** Positive while the observer recedes from the body's centre. */
		double RadialCmPerS{0.0};
		APSPlanetArrivalForecast::FResult Line;
	};

	/** Per-subsystem state of the rules above (the subsystem's header is shared; this stays private to this file). */
	struct FPolicyState
	{
		float DeltaSeconds{0.0f};
		// Leave freeze: the observer's recession from the anchored root, measured every frame.
		TWeakObjectPtr<AWorldScapeRoot> MotionRoot;
		uint64 MotionFrame{0};
		double MotionDistanceCm{-1.0};
		double RecedeCmPerS{0.0};
		bool bLeaveFrozen{false};
		// Rio 06.10 (leave freeze hold, publish hold): the F10 map camera is the visual observer on this frame's pass.
		bool bMapObserver{false};
		// Rio 06.10 (audit: F10 near a planet, aps.Map.KeepObserverOnPass): the frame whose per-frame pass made the map camera
		// the observer; MAX_uint64 when it did not.
		uint64 MapObserverFrame{MAX_uint64};
		// Rio 06.10 (audit: the leave freeze kept the map camera's position after F10 closed): the map camera was the observer
		// on the previous per-frame pass.
		bool bMapObserverLastFrame{false};
		// Rio 06.10 (publish hold): frames held since the last wave finished, and the frame/root of this frame's hold.
		int32 PublishHeldFrames{0};
		uint64 PublishHoldFrame{MAX_uint64};
		TWeakObjectPtr<AWorldScapeRoot> PublishHoldRoot;
		// Body motion between selection passes (switch dwell, pass-by, the leave freeze of a rebound root).
		TMap<TWeakObjectPtr<APlanetaryBody>, FVector> PassRelative;
		TWeakObjectPtr<APawn> PassObserver;
		double PassSeconds{0.0};
		TMap<TWeakObjectPtr<AWorldScapeRoot>, double> PassRecedeByRoot;
		// Switch dwell.
		TWeakObjectPtr<APlanetaryBody> SwitchCandidate;
		double SwitchCandidateSince{0.0};
		TWeakObjectPtr<APlanetaryBody> LoggedHold;
		// Near observer step, summarised in the log every 10 s.
		int32 StepKeptFrames{0};
		int32 StepMoves{0};
		double StepLogSeconds{0.0};
		double StepLastCountSeconds{0.0};
		// Rio 09.10 fast observer step (aps.Surface.FastObserverKmPerS): the last move, and the counts logged every 10 s.
		double FastMoveSeconds{0.0};
		int32 FastKeptFrames{0};
		int32 FastMoves{0};
		double FastLogSeconds{0.0};
	};
	TMap<const void*, FPolicyState> GPolicyStates;

	FPolicyState& PolicyState(const void* Owner)
	{
		return GPolicyStates.FindOrAdd(Owner);
	}

	const APlanetaryBody* OwningBody(const AWorldScapeRoot* Root)
	{
		const APlanetaryBody* Body = Cast<APlanetaryBody>(Root->GetOwner());
		if (!Body)
		{
			Body = Cast<APlanetaryBody>(Root->GetAttachParentActor());
		}
		return Body && IsValid(Body->PlanetaryEnvironmentGenerator)
			&& Body->PlanetaryEnvironmentGenerator->WorldScapeRootInstance == Root ? Body : nullptr;
	}

	/**
	 * WorldScape (WorldScapeRoot_Main.cpp, UpdatePosition) scales every LOD by 2^round(log2(altitude / HeightAnchor)), capped
	 * per LOD set at ceil(128 R / (triangle * resolution * 2^MaxLod)) and 999. Once the kept observer is above the altitude
	 * where both sets reach that cap, its coarsest ring spans the whole globe: a farther observer would build the same rings.
	 */
	bool IsAltitudeScaleSaturated(const AWorldScapeRoot* Root)
	{
		const double Altitude = Root->PlayerDistanceToGround;
		if (!Root->init || !(Root->HeightAnchor > 0.0f) || !FMath::IsFinite(Altitude) || Altitude <= Root->HeightAnchor)
		{
			return false;
		}
		const auto Cap = [Root](const double Triangle, const int32 Resolution, const int32 MaxLod)
		{
			const double Span = Triangle * Resolution * FMath::Pow(2.0, static_cast<double>(MaxLod));
			return Span > 0.0 ? FMath::Clamp(FMath::CeilToDouble(128.0 * Root->PlanetScaleCode / Span), 1.0, 999.0) : 999.0;
		};
		double Needed = Cap(Root->TriangleSize, Root->LodResolution, Root->MaxLod);
		if (Root->bOcean)
		{
			Needed = FMath::Max(Needed, Cap(Root->OceanTriangleSize, Root->OceanLodResolution, Root->OceanMaxLod));
		}
		const double Multiplier = FMath::Pow(2.0, FMath::RoundToDouble(FMath::Log2(Altitude / Root->HeightAnchor)));
		return Multiplier >= Needed;
	}

	/** Updates the observer's recession from Root (once per frame) and whether the leave freeze keeps Root's observer. */
	void UpdateLeaveFreeze(FPolicyState& State, AWorldScapeRoot* Root, const FVector& PawnLocation)
	{
		const double Distance = FVector::Distance(PawnLocation, Root->GetActorLocation());
		if (State.MotionRoot.Get() != Root)
		{
			// A rebound root (a surface resumed on the way out) starts from the selection pass's measurement.
			if (State.bLeaveFrozen)
			{
				UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("[APS.Surface] leave freeze off: %s is no longer the anchored surface"),
					*GetNameSafe(State.MotionRoot.Get()));
			}
			State.MotionRoot = Root;
			State.MotionFrame = GFrameCounter;
			const double* Seed = State.PassRecedeByRoot.Find(Root);
			State.RecedeCmPerS = Seed ? *Seed : 0.0;
			State.bLeaveFrozen = false;
		}
		else if (State.MotionFrame != GFrameCounter)
		{
			if (GFrameCounter - State.MotionFrame > 4)
			{
				// Rio 06.10 (review): only (nearly) consecutive frames measure a speed. After a gap (no anchor for a while, a
				// paused world) the distance change spans many frames and read as a burst of recession over one frame's
				// time, which could freeze a slowly receding observer; the selection pass's measurement seeds it instead.
				// A frame or two of slack keeps the measurement across a high-resolution screenshot's extra frame.
				const double* Seed = State.PassRecedeByRoot.Find(Root);
				State.RecedeCmPerS = Seed ? *Seed : 0.0;
			}
			else if (State.DeltaSeconds > 0.0f && State.MotionDistanceCm >= 0.0 && FMath::IsFinite(Distance))
			{
				const double Raw = (Distance - State.MotionDistanceCm) / State.DeltaSeconds;
				const double Blend = 1.0 - FMath::Exp(-static_cast<double>(State.DeltaSeconds) / 0.2);
				State.RecedeCmPerS += (Raw - State.RecedeCmPerS) * Blend;
			}
			State.MotionFrame = GFrameCounter;
		}
		State.MotionDistanceCm = FMath::IsFinite(Distance) ? Distance : -1.0;
		if (!FMath::IsFinite(State.RecedeCmPerS))
		{
			State.RecedeCmPerS = 0.0;
		}

		const double Limit = CVarLeaveFreezeKmPerS.GetValueOnGameThread() * 1.0e5;
		const APlanetaryBody* Body = Limit > 0.0 ? OwningBody(Root) : nullptr;
		const bool bEligible = Body && Body->bWorldScapeSurfaceReady && Root->bOverridePlayerPosition
			&& !Root->OverridedPlayerPosition.ContainsNaN() && IsAltitudeScaleSaturated(Root);
		// Rio 06.10 (takeoff: after the freeze let go at 14 km/s on a turn, the planet was rebuilt again, hitches 54/43/312 ms):
		// with aps.Surface.LeaveFreezeHoldTurnDeg > 0 a freeze already on also holds while the observer is still above the kept
		// observer, not approaching fast, and the view from the centre has turned less than that angle. Not while the F10 map
		// camera is the observer. 0: bTurnHold stays false, so the 0.8x rule below is exactly the previous one.
		const FVector KeptFromCenter = Root->OverridedPlayerPosition - Root->GetActorLocation();
		const FVector HereFromCenter = PawnLocation - Root->GetActorLocation();
		bool bTurnHold = false;
		if (const double HoldTurnDeg = CVarLeaveFreezeHoldTurnDeg.GetValueOnGameThread();
			State.bLeaveFrozen && HoldTurnDeg > 0.0 && !State.bMapObserver)
		{
			bTurnHold = HereFromCenter.SizeSquared() >= KeptFromCenter.SizeSquared() && State.RecedeCmPerS > -0.1 * Limit
				&& FVector::DotProduct(HereFromCenter.GetSafeNormal(), KeptFromCenter.GetSafeNormal())
					> FMath::Cos(FMath::DegreesToRadians(HoldTurnDeg));
		}
		const bool bFreeze = bEligible && (bTurnHold || State.RecedeCmPerS > (State.bLeaveFrozen ? 0.8 : 1.0) * Limit);
		if (bFreeze != State.bLeaveFrozen)
		{
			State.bLeaveFrozen = bFreeze;
			// Rio 06.10 (review of Rio's takeoff log): the "off" line also gives the view's turn from the kept observer.
			const FString TurnNote = bFreeze ? FString() : FString::Printf(TEXT(", view turned %.1f deg from the kept observer"),
				FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
					FVector::DotProduct(HereFromCenter.GetSafeNormal(), KeptFromCenter.GetSafeNormal()), -1.0, 1.0))));
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("[APS.Surface] leave freeze %s: %s observer %.0f km above the ground, receding at %.1f km/s%s"),
				bFreeze ? TEXT("on") : TEXT("off"), *GetNameSafe(Body ? static_cast<const AActor*>(Body) : Root),
				Root->PlayerDistanceToGround / 1.0e5, State.RecedeCmPerS / 1.0e5, *TurnNote);
		}
	}

	/** Near-ground observer step: true while the step rule applies to Root; bOutHold while Candidate is within the step. */
	bool NearStepApplies(const FPolicyState& State, const AWorldScapeRoot* Root, const FVector& Candidate, bool& bOutHold)
	{
		bOutHold = false;
		const double Step = CVarNearObserverStepCm.GetValueOnGameThread();
		// A zero-time manual Tick (automation, tools) asks for the exact observer.
		if (!(Step > 0.0) || !(State.DeltaSeconds > 0.0f) || !Root->init || !Root->bOverridePlayerPosition
			|| Root->OverridedPlayerPosition.ContainsNaN() || Candidate.ContainsNaN()
			|| !(Root->PlayerDistanceToGround <= CVarNearObserverAltitudeKm.GetValueOnGameThread() * 1.0e5))
		{
			return false;
		}
		bOutHold = FVector::DistSquared(Candidate, Root->OverridedPlayerPosition) < FMath::Square(Step);
		return true;
	}

	void CountNearStep(FPolicyState& State, const AWorldScapeRoot* Root, const bool bKept)
	{
		const double Now = FPlatformTime::Seconds();
		// Rio 06.10 (review): a new stretch near the ground opens a new window, so each line is a rate over its own seconds
		// (a window left open while flying reported the counts of a short walk over minutes).
		if (State.StepLogSeconds <= 0.0 || Now - State.StepLastCountSeconds > 1.0)
		{
			State.StepLogSeconds = Now;
			State.StepKeptFrames = 0;
			State.StepMoves = 0;
		}
		State.StepLastCountSeconds = Now;
		bKept ? ++State.StepKeptFrames : ++State.StepMoves;
		if (Now - State.StepLogSeconds >= 10.0)
		{
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("[APS.Surface] near observer step %.0f cm: kept %d frames, moved %d times in %.0f s (%s, %.0f m above the ground)"),
				CVarNearObserverStepCm.GetValueOnGameThread(), State.StepKeptFrames, State.StepMoves, Now - State.StepLogSeconds,
				*GetNameSafe(Root->GetOwner()), Root->PlayerDistanceToGround / 100.0);
			State.StepKeptFrames = 0;
			State.StepMoves = 0;
			State.StepLogSeconds = Now;
		}
	}

	/** Fast observer step (aps.Surface.FastObserverKmPerS): true while a fast observer high over the ground keeps its place. */
	bool FastStepHolds(FPolicyState& State, const AWorldScapeRoot* Root, const APawn* Observer, const FVector& Candidate)
	{
		const double FastCm = CVarFastObserverKmPerS.GetValueOnGameThread() * 1.0e5;
		const double MinSeconds = CVarFastObserverSeconds.GetValueOnGameThread();
		if (!(FastCm > 0.0) || !(MinSeconds > 0.0) || !(State.DeltaSeconds > 0.0f) || !Root->init
			|| !Root->bOverridePlayerPosition || Root->OverridedPlayerPosition.ContainsNaN() || Candidate.ContainsNaN()
			|| Root->PlayerDistanceToGround <= CVarNearObserverAltitudeKm.GetValueOnGameThread() * 1.0e5)
		{
			return false;
		}
		const AActor* Mover = Observer->GetAttachParentActor() ? Observer->GetAttachParentActor() : Observer;
		double SpeedCm = Mover->GetVelocity().Size();
		if (const ASpaceship* Ship = Cast<ASpaceship>(Mover))
		{
			SpeedCm = FMath::Max(SpeedCm, Ship->GetKinematicVelocity().Size());
		}
		if (SpeedCm <= FastCm)
		{
			return false;
		}
		const double Now = FPlatformTime::Seconds();
		const bool bHold = Now - State.FastMoveSeconds < MinSeconds;
		bHold ? ++State.FastKeptFrames : ++State.FastMoves;
		if (!bHold)
		{
			State.FastMoveSeconds = Now;
		}
		if (State.FastLogSeconds <= 0.0 || Now - State.FastLogSeconds > 30.0)
		{
			State.FastLogSeconds = Now;
			State.FastKeptFrames = State.FastMoves = 0;
		}
		else if (Now - State.FastLogSeconds >= 10.0)
		{
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("[APS.Surface] fast observer step %.2f s: kept %d frames, moved %d times in %.0f s (%s, %.0f km above the ground, %.0f km/s)"),
				MinSeconds, State.FastKeptFrames, State.FastMoves, Now - State.FastLogSeconds, *GetNameSafe(Root->GetOwner()),
				Root->PlayerDistanceToGround / 1.0e5, SpeedCm / 1.0e5);
			State.FastLogSeconds = Now;
			State.FastKeptFrames = State.FastMoves = 0;
		}
		return bHold;
	}

	/** True when the half-second pass or a rebind must leave Root's observer where the per-frame rules keep it. */
	bool HoldsObserver(FPolicyState& State, AWorldScapeRoot* Root, const FVector& PawnLocation, const bool bAnchorChanged)
	{
		UpdateLeaveFreeze(State, Root, PawnLocation);
		// Rio 06.10 (review): a zero-time manual Tick (automation, tools) asks for the exact observer, as for the near step.
		if (State.bLeaveFrozen && State.DeltaSeconds > 0.0f)
		{
			return true;
		}
		// Rio 06.10 (audit: F10 near a planet): this frame's per-frame pass made the map camera the observer; the half-second
		// pass wrote the pawn over it for one frame, so WorldScape built two full LOD waves (and reset collision) per cycle.
		// A new anchor and a zero-time manual Tick still get the pawn. aps.Map.KeepObserverOnPass 0: the old behaviour.
		if (!bAnchorChanged && State.DeltaSeconds > 0.0f && State.MapObserverFrame == GFrameCounter
			&& CVarMapKeepObserverOnPass.GetValueOnGameThread() != 0)
		{
			return true;
		}
		// Rio 06.10 (publish hold): the per-frame pass held Root's observer this frame for aps.Surface.PublishHoldFrames; the
		// half-second pass does not undo it. A new anchor still gets the exact observer. Never set while the CVar is 0.
		// Rio 06.10 (review): a zero-time manual Tick gets the exact observer here too, even after a held frame's stamp.
		if (!bAnchorChanged && State.DeltaSeconds > 0.0f && State.PublishHoldFrame == GFrameCounter
			&& State.PublishHoldRoot.Get() == Root)
		{
			return true;
		}
		bool bHold = false;
		return !bAnchorChanged && NearStepApplies(State, Root, PawnLocation, bHold) && bHold;
	}
}

bool UAPSPlanetEnvironmentStreamingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSPlanetEnvironmentStreamingSubsystem::Deinitialize()
{
	APSPlanetReliefRuntime::Release(GetWorld());
	APSPlaceholderGlobe::Release(GetWorld());
	ClearGameplayCollisionAnchor();
	WarmingBody.Reset();
	PrewarmedBodies.Reset();
	FirstBuilds.Reset();
	VisibleLiquidBodies.Reset();
	CancelFlightReplacement();
	APSSurfacePolicyPrivate::GPolicyStates.Remove(this);
	Super::Deinitialize();
}

void UAPSPlanetEnvironmentStreamingSubsystem::Tick(float DeltaTime)
{
	APSSurfacePolicyPrivate::PolicyState(this).DeltaSeconds = DeltaTime;
	// The active body/family search is intentionally amortized below, but WorldScape's
	// visual producer must follow the possessed pawn every frame. Leaving this position
	// on the half-second cadence lets a fast manual approach outrun the generated patch,
	// while the visual chunk producer continues to target the pawn's previous location.
	RefreshGameplayObserverPosition();

	UpdateElapsed += DeltaTime;
	if (UpdateElapsed >= UpdateInterval)
	{
		UpdateElapsed = 0.0f;
		UpdateActiveEnvironment();
	}
	RefreshVisibleLiquidAppearance();
	AdvanceFirstBuilds();
	ProbeWorldScapeProxies();
	UpdateFlightResidency(DeltaTime);
	APSPlaceholderGlobe::Tick(GetWorld());
	APSPlanetReliefRuntime::Tick(GetWorld(),ActiveBody.Get(),ArrivingBody.Get());
	APSWorldScapeReveal::AdvanceStagedReveals(GetWorld());
}

void UAPSPlanetEnvironmentStreamingSubsystem::RefreshVisibleLiquidAppearance()
{
	for (const TWeakObjectPtr<APlanetaryBody>& WeakBody : VisibleLiquidBodies)
	{
		APlanetaryBody* Body = WeakBody.Get();
		if (IsValid(Body) && Body->bWorldScapeSurfaceReady
			&& (Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Active
				|| Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::FrozenVisible)
			&& IsValid(Body->PlanetaryEnvironmentGenerator))
		{
			Body->PlanetaryEnvironmentGenerator->UpdateOrbitalWaterAppearance();
		}
	}
}

TStatId UAPSPlanetEnvironmentStreamingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSPlanetEnvironmentStreamingSubsystem, STATGROUP_Tickables);
}

void UAPSPlanetEnvironmentStreamingSubsystem::UpdateStandbyWarmup(
	APlanetaryBody* Candidate, APawn* Observer)
{
	if (APlanetaryBody* Previous = WarmingBody.Get(); Previous && Previous != Candidate)
	{
		// An approach takes ownership of the same root; do not pause that new active
		// producer. A cancelled speculative build drains through the existing path.
		if (Previous->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Preloaded
			&& IsValid(Previous->PlanetaryEnvironmentGenerator))
			Previous->PlanetaryEnvironmentGenerator->PreloadWorldScapeRoot();
	}
	const bool bStarting = WarmingBody.Get() != Candidate;
	WarmingBody = Candidate;
	if (!IsValid(Candidate) || !IsValid(Observer)
		|| !IsValid(Candidate->PlanetaryEnvironmentGenerator)) return;
	auto* Generator = Candidate->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = Generator->WorldScapeRootInstance;
	if (!IsValid(Root) || Generator->IsSurfaceProfileApplyPending())
	{
		WarmingBody.Reset(); // Retry starting after the deferred profile drain.
		return;
	}
	if (bStarting)
	{
		// Allocation alone was not preload: the moon remained a white placeholder
		// until the player approached it. Build one real, collision-free payload now.
		// Keep its observer fixed until publication so travel cannot starve readiness.
		Root->bOverridePlayerPosition = true;
		Root->OverridedPlayerPosition = Observer->GetActorLocation();
		Generator->SpawnWorldScapeRoot();
		Root->DistanceToFreezeGeneration = 0.0f;
		Root->SetActorHiddenInGame(true);
		Root->SetActorEnableCollision(false);
		Root->bGenerateCollision = false;
#if WITH_EDITOR
		Root->bGenerateCollisionInEditor = false;
#endif
		HoldFirstBuild(Root, false);
	}
	if (Root->WorldScapeLodInGeneration.Num() > 0)
		Root->CheckForLodGeneration(); // Non-blocking IsDone fence, never wait/join.
	if (Candidate->RefreshWorldScapeSurfaceVisibility())
	{
		Candidate->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
		PrewarmedBodies.AddUnique(Candidate);
		WarmingBody.Reset();
		UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Published standby terrain: %s"),
			*Candidate->GetPathName());
	}
}

APlanet* UAPSPlanetEnvironmentStreamingSubsystem::ResolveFamilyPlanet(APlanetaryBody* Body) const
{
	if (APlanet* Planet = Cast<APlanet>(Body))
	{
		return Planet;
	}
	if (const AMoon* Moon = Cast<AMoon>(Body))
	{
		return Moon->ParentPlanet;
	}
	return nullptr;
}

void UAPSPlanetEnvironmentStreamingSubsystem::ClearGameplayCollisionAnchor()
{
	AWorldScapeRoot* Root = AnchoredWorldScapeRoot.Get();
	APawn* Pawn = CollisionAnchorPawn.Get();
	if (IsValid(Root) && IsValid(Pawn))
	{
		Root->CollisionDependantActor.Remove(Pawn);
	}
	AnchoredWorldScapeRoot.Reset();
	CollisionAnchorPawn.Reset();
}

void UAPSPlanetEnvironmentStreamingSubsystem::RefreshGameplayObserverPosition()
{
	AWorldScapeRoot* Root = AnchoredWorldScapeRoot.Get();
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!IsValid(Root) || !IsValid(Observer))
	{
		return;
	}
	FVector VisualObserver = Observer->GetActorLocation();
	bool bMapObserver = false;
	// Rio 03.10 ("in F10 the planet must load fully, through WorldScape itself"): while the strategic map is open, its
	// camera is the terrain's visual observer, so WorldScape builds the planet for the view the map shows. Collision stays
	// with the pawn (CollisionDependantActor, ApplyGameplayObserverContract), so nothing under the pawn changes.
	// Rio 06.10 (leave freeze hold): decided before the leave freeze below, which must not hold for the map camera; this
	// block only reads state.
	if (const AGravityPlayerController* GravityController = Cast<AGravityPlayerController>(PlayerController);
		GravityController && GravityController->IsStrategicMapOpen() && IsValid(GravityController->PlayerCameraManager)
		&& CVarMapCameraObserver.GetValueOnGameThread() != 0)
	{
		const FVector MapCamera = GravityController->PlayerCameraManager->GetCameraLocation();
		const double MapRadii = CVarMapObserverRadii.GetValueOnGameThread();
		if (MapRadii <= 0.0 || Root->PlanetScale <= 0.0
			|| FVector::DistSquared(MapCamera, Root->GetActorLocation()) <= FMath::Square(MapRadii * Root->PlanetScale))
		{
			VisualObserver = MapCamera;
			bMapObserver = true;
		}
	}
	// Rio 06.10 (freezes on departure): the observer's recession from this root is measured every frame, also across a
	// possession change (leaving the seat of a receding ship is not a stop).
	APSSurfacePolicyPrivate::FPolicyState& Policy = APSSurfacePolicyPrivate::PolicyState(this);
	Policy.bMapObserver = bMapObserver;
	// Rio 06.10 (audit: F10 near a planet): recorded only; HoldsObserver reads it for aps.Map.KeepObserverOnPass.
	Policy.MapObserverFrame = bMapObserver ? GFrameCounter : MAX_uint64;
	// Rio 06.10 (audit: a leave freeze on while F10 was open kept the map camera's position as the observer after the map
	// closed): on the closing frame the freeze lets go once and the pawn is written; it may freeze again from there. Inert
	// at aps.Surface.LeaveFreezeKmPerS 0. The freeze is not forced off while the map is open.
	const bool bMapJustClosed = Policy.bMapObserverLastFrame && !bMapObserver;
	Policy.bMapObserverLastFrame = bMapObserver;
	if (bMapJustClosed && Policy.bLeaveFrozen)
	{
		Policy.bLeaveFrozen = false;
		UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("[APS.Surface] leave freeze released: map closed (%s)"),
			*GetNameSafe(Root->GetOwner()));
	}
	APSSurfacePolicyPrivate::UpdateLeaveFreeze(Policy, Root, Observer->GetActorLocation());
	if (CollisionAnchorPawn.Get() != Observer)
	{
		// Possession changes are cheap observer-contract changes, not a reason to run
		// the global body/family search early. Rebind this already-active root now so
		// neither its visual nor collision producer follows the previous pawn.
		ApplyGameplayObserverContract(Root, Observer);
		return;
	}

	Root->bOverridePlayerPosition = true;
	// Rio 03.10 (flight stutter): far out every LOD batch (50-400 ms, about every 0.4 s) only redraws the same small globe.
	// Beyond FarFreezeRadii the observer keeps its last far position until the view from the planet's centre has turned
	// FarFreezeTurnDeg; inside that radius, and on the way in, it follows every frame as before.
	if (const double FreezeRadii = CVarFarFreezeRadii.GetValueOnGameThread(); FreezeRadii > 0.0 && Root->PlanetScale > 0.0)
	{
		const FVector Center = Root->GetActorLocation();
		const FVector FromCenter = VisualObserver - Center;
		const FVector LastFromCenter = Root->OverridedPlayerPosition - Center;
		const double FarSquared = FMath::Square(FreezeRadii * Root->PlanetScale);
		if (FromCenter.SizeSquared() > FarSquared && LastFromCenter.SizeSquared() > FarSquared
			&& FVector::DotProduct(FromCenter.GetSafeNormal(), LastFromCenter.GetSafeNormal())
				> FMath::Cos(FMath::DegreesToRadians(static_cast<double>(CVarFarFreezeTurnDeg.GetValueOnGameThread()))))
		{
			return;
		}
	}
	if (!bMapObserver)
	{
		// Rio 06.10 (freezes on departure): a planet being left faster than aps.Surface.LeaveFreezeKmPerS keeps its observer,
		// like the far freeze, instead of rebuilding every LOD for an observer that is gone a second later.
		// Rio 06.10 (audit): not on the frame F10 closed, so the pawn replaces the map camera's position.
		if (Policy.bLeaveFrozen && !bMapJustClosed && Policy.DeltaSeconds > 0.0f && Policy.MotionRoot.Get() == Root)
		{
			return;
		}
		// Rio 06.10 (stutter on foot): near the ground the observer moves in aps.Surface.NearObserverStepCm steps, not every
		// 1.2 m re-snap of the finest ring. Flight moves it at once (>8 m a frame); a world shift moves both together.
		bool bKeep = false;
		if (APSSurfacePolicyPrivate::NearStepApplies(Policy, Root, VisualObserver, bKeep))
		{
			APSSurfacePolicyPrivate::CountNearStep(Policy, Root, bKeep);
			if (bKeep)
			{
				return;
			}
		}
		// Rio 09.10 (autopilot low over a planet at hundreds of km/s): a fast observer above the near-step altitude moves at
		// most every aps.Surface.FastObserverSeconds (aps.Surface.FastObserverKmPerS 0: off).
		else if (APSSurfacePolicyPrivate::FastStepHolds(Policy, Root, Observer, VisualObserver))
		{
			return;
		}
	}
	// Rio 06.10 (takeoff from an ocean world, 1-2 hitches a second): a land+ocean LOD wave nearly fills WorldScape's
	// prepared-mesh budget and its reservations live on in render commands; the next wave, started as soon as the previous
	// one is published, then falls back to the slow legacy upload. Above 300 m, while prepared reservations are live, the
	// observer stays put during a wave's generation and for up to aps.Surface.PublishHoldFrames frames after it, so the render
	// thread can release them first. A zero-time manual Tick gets the exact observer. 0: off (as before).
	// Rio 06.10 (review): at most 30 frames after a wave, so a typo or a leaked reservation cannot park the observer.
	if (const int32 MaxPublishHold = FMath::Min(APSSurfacePolicyPrivate::CVarPublishHoldFrames.GetValueOnGameThread(), 30);
		MaxPublishHold > 0 && Policy.DeltaSeconds > 0.0f && Root->PlayerDistanceToGround > 3.0e4
		&& WorldScapePreparedMesh::CurrentBytes() > 0)
	{
		if (Policy.PublishHoldRoot.Get() != Root)
		{
			Policy.PublishHoldRoot = Root;
			Policy.PublishHeldFrames = 0;
		}
		if (Root->WorldScapeLodInGeneration.Num() > 0)
		{
			Policy.PublishHeldFrames = 0;
			Policy.PublishHoldFrame = GFrameCounter;
			return;
		}
		if (Policy.PublishHeldFrames++ < MaxPublishHold)
		{
			Policy.PublishHoldFrame = GFrameCounter;
			return;
		}
	}
	Policy.PublishHeldFrames = 0;
	// Rio 06.10 (review): a released frame is not a held one for a later pass of the same frame (MAX_uint64 at CVar 0 anyway).
	Policy.PublishHoldFrame = MAX_uint64;
	Root->OverridedPlayerPosition = VisualObserver;
}

void UAPSPlanetEnvironmentStreamingSubsystem::ApplyGameplayObserverContract(
	AWorldScapeRoot* Root, APawn* Observer)
{
	if (!IsValid(Root) || !IsValid(Observer))
	{
		return;
	}

	const bool bAnchorChanged = AnchoredWorldScapeRoot.Get() != Root
		|| CollisionAnchorPawn.Get() != Observer;
	if (bAnchorChanged)
	{
		ClearGameplayCollisionAnchor();
		AnchoredWorldScapeRoot = Root;
		CollisionAnchorPawn = Observer;
	}

	// WorldScape uses OverridedPlayerPosition only for its visual LOD producer.
	// CollisionLodHandler independently rebuilds a list from controllers/editor
	// viewport state. During possession/handoff the controller entry can be transient
	// while a detached editor camera remains present. Register the possessed pawn
	// explicitly in that second contract as well. AddUnique also repairs a root that
	// internally cleared the invoker list during regeneration without accumulating
	// duplicate anchors on the subsystem's half-second refresh.
	Root->CollisionDependantActor.AddUnique(Observer);
	const bool bHadObserver = Root->bOverridePlayerPosition;
	Root->bOverridePlayerPosition = true;
	// Rio 06.10 (planet freezes): the half-second pass rewrote the pawn's position here, undoing the near-ground step every
	// 0.5 s, and a surface resumed while being left (A->B->A on departure) rebuilt every LOD for the receding observer. The
	// rules of RefreshGameplayObserverPosition decide here too; a new anchor still gets the exact observer unless it is
	// being left faster than aps.Surface.LeaveFreezeKmPerS.
	if (!bHadObserver || !APSSurfacePolicyPrivate::HoldsObserver(APSSurfacePolicyPrivate::PolicyState(this), Root,
		Observer->GetActorLocation(), bAnchorChanged))
	{
		Root->OverridedPlayerPosition = Observer->GetActorLocation();
	}
	Root->DistanceToFreezeGeneration = 0.0f;
	const bool bTransit = Root->ActorHasTag(TEXT("APS.Surface.Transit"));
	Root->bGenerateCollision = !bTransit;
	Root->bGenerateCollisionForAllPlayer = true;
#if WITH_EDITOR
	Root->bGenerateCollisionInEditor = !bTransit;
	Root->bStaticCollisionInEditor = false;
#endif
	if (bAnchorWithoutCollision)
	{
		// A surface started ahead of arrival builds no collision until the observer is within its reach.
		Root->bGenerateCollision = false;
#if WITH_EDITOR
		Root->bGenerateCollisionInEditor = false;
#endif
	}

	if (bAnchorChanged)
	{
		UE_LOG(LogAPSWorldScapeStreaming, Log,
			TEXT("Bound WorldScape gameplay anchor: root=%s pawn=%s location=%s"),
			*Root->GetPathName(), *Observer->GetPathName(),
			*Observer->GetActorLocation().ToCompactString());
	}
}

void UAPSPlanetEnvironmentStreamingSubsystem::UpdateActiveEnvironment()
{
	UWorld* World = GetWorld();
	VisibleLiquidBodies.Reset();
	// Do not disable streaming for the complete authored SinglePlay map. Its
	// integrated home planet opts out explicitly (bStreamWorldScapeSurface=false),
	// while the other generated planets and moons still need the same distant
	// family preload and nearest-body activation used by generated gameplay.
	// A map-wide early return made every remote atmosphere permanently empty.

	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Observer)
	{
		return;
	}

	const FVector ObserverLocation = Observer->GetActorLocation();
	if (ObserverLocation.ContainsNaN()) return;
	bHasObservedPawn = true;
	TArray<APlanetaryBody*> StreamedBodies;
	TMap<APlanet*, TArray<APlanetaryBody*>> Families;
	for (TActorIterator<APlanetaryBody> It(World); It; ++It)
	{
		APlanetaryBody* Body = *It;
		if (!IsValid(Body) || !Body->bStreamWorldScapeSurface
			|| IsExplicitMenuPreviewBody(Body))
		{
			continue;
		}
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
		{
			// Gas MOONS obey the same rule as planets. A live solid->gas edit must
			// also release its old root, not leave it updating outside the candidate set.
			if (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded)
				Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
			continue;
		}
		// Closed physical geometry uses the same native profile and material, not a palette-only sphere.
		APSPlaceholderGlobe::Apply(Body);
		StreamedBodies.Add(Body);
		if (APlanet* Family = ResolveFamilyPlanet(Body); IsValid(Family))
		{
			Families.FindOrAdd(Family).Add(Body);
		}
	}

	// Rio 06.10 (planet freezes on arrival and departure): the observer's motion relative to each streamed body since the
	// previous pass, for the switch dwell, the pass-by rule and the leave freeze of a rebound root. Kept apart from the
	// arrival forecast, so that aps.Surface.ArrivalLeadSeconds 0 does not switch these rules off.
	using namespace APSSurfacePolicyPrivate;
	FPolicyState& Policy = PolicyState(this);
	const double PassNow = World->GetTimeSeconds();
	const double PassStep = PassNow - Policy.PassSeconds;
	const bool bPassMotion = Policy.PassObserver.Get() == Observer && PassStep >= 0.1 && PassStep <= 2.0;
	TMap<const APlanetaryBody*, FPassMotion> PassMotion;
	TMap<TWeakObjectPtr<APlanetaryBody>, FVector> PassRelative;
	Policy.PassRecedeByRoot.Reset();
	for (APlanetaryBody* Body : StreamedBodies)
	{
		const FVector Relative = ObserverLocation - Body->GetActorLocation();
		PassRelative.Add(Body, Relative);
		const FVector* Previous = bPassMotion ? Policy.PassRelative.Find(Body) : nullptr;
		if (!Previous)
		{
			continue;
		}
		FPassMotion& Motion = PassMotion.Add(Body);
		Motion.SpeedCmPerS = FVector::Distance(Relative, *Previous) / PassStep;
		Motion.RadialCmPerS = (Relative.Size() - Previous->Size()) / PassStep;
		Motion.Line = APSPlanetArrivalForecast::Evaluate(Relative, *Previous, PassStep, Body->GetWorldScapeBodyRadiusCm());
		if (IsValid(Body->PlanetaryEnvironmentGenerator) && IsValid(Body->PlanetaryEnvironmentGenerator->WorldScapeRootInstance))
		{
			Policy.PassRecedeByRoot.Add(Body->PlanetaryEnvironmentGenerator->WorldScapeRootInstance, Motion.RadialCmPerS);
		}
	}
	Policy.PassRelative = MoveTemp(PassRelative);
	Policy.PassObserver = Observer;
	Policy.PassSeconds = PassNow;
	const double NearRadii = FMath::Max(0.0, static_cast<double>(CVarActiveSwitchNearRadii.GetValueOnGameThread()));
	const auto IsNear = [&ObserverLocation, NearRadii](const APlanetaryBody* Body)
	{
		return FVector::DistSquared(ObserverLocation, Body->GetActorLocation())
			<= FMath::Square(NearRadii * Body->GetWorldScapeBodyRadiusCm());
	};
	const double MissRadii = FMath::Max(0.0, static_cast<double>(CVarPassByMissRadii.GetValueOnGameThread()));
	const auto IsApproach = [&PassMotion, MissRadii](const APlanetaryBody* Body)
	{
		const FPassMotion* Motion = PassMotion.Find(Body);
		return Motion && Motion->Line.ClosingSpeedCmPerSecond > 0.0
			&& Motion->Line.MissDistanceCm <= MissRadii * Body->GetWorldScapeBodyRadiusCm();
	};
	const double PassByLimit = CVarPassByKmPerS.GetValueOnGameThread() * 1.0e5;
	const auto PassesBy = [&PassMotion, &IsNear, &IsApproach, PassByLimit](const APlanetaryBody* Body)
	{
		const FPassMotion* Motion = Body && PassByLimit > 0.0 ? PassMotion.Find(Body) : nullptr;
		return Motion && Motion->SpeedCmPerS > PassByLimit && !IsNear(Body) && !IsApproach(Body);
	};
	const auto DescribeMotion = [&PassMotion](const APlanetaryBody* Body)
	{
		const FPassMotion* Motion = PassMotion.Find(Body);
		if (!Motion)
		{
			return FString(TEXT("no motion"));
		}
		const double Radius = Body->GetWorldScapeBodyRadiusCm();
		return Motion->Line.ClosingSpeedCmPerSecond > 0.0
			? FString::Printf(TEXT("%.1f km/s, line %.1f radii from its centre"), Motion->SpeedCmPerS / 1.0e5,
				Motion->Line.MissDistanceCm / Radius)
			: FString::Printf(TEXT("%.1f km/s, receding"), Motion->SpeedCmPerS / 1.0e5);
	};

	APlanetaryBody* Arriving = UpdateArrivalForecast(Observer, StreamedBodies);
	APlanet* BestFamily = nullptr;
	double BestFamilyScore = TNumericLimits<double>::Max();
	FString BestFamilyKey;
	for (const TPair<APlanet*, TArray<APlanetaryBody*>>& Pair : Families)
	{
		const bool bResident = ResidentFamily.Get() == Pair.Key;
		double FamilyScore = TNumericLimits<double>::Max();
		for (APlanetaryBody* Body : Pair.Value)
		{
			const double Limit = bResident
				? Body->GetWorldScapeUnloadRadiusCm()
				: Body->GetWorldScapePreloadRadiusCm();
			const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
			FamilyScore = FMath::Min(FamilyScore, APSWorldScapeStreamingPolicy::Score(
				Distance, Body->GetWorldScapeBodyRadiusCm(), Limit, bResident, 0.72));
		}
		const FString Key = Pair.Key->GetPathName();
		if (APSWorldScapeStreamingPolicy::Prefer(FamilyScore, bResident, Key,
			BestFamilyScore, BestFamily && BestFamily == ResidentFamily.Get(), BestFamilyKey))
		{
			BestFamilyScore = FamilyScore;
			BestFamily = Pair.Key;
			BestFamilyKey = Key;
		}
	}

	// Flying to another family's body wins over a family the observer only passes or leaves, so that the surface starts
	// before arrival. Within a body's activation radius the distance rule keeps its family (taking off, moving between
	// a planet and its moons).
	if (APlanet* ArrivingFamily = Arriving ? ResolveFamilyPlanet(Arriving) : nullptr;
		IsValid(ArrivingFamily) && ArrivingFamily != BestFamily && Families.Contains(ArrivingFamily))
	{
		bool bWithinReach = false;
		if (const TArray<APlanetaryBody*>* Current = BestFamily ? Families.Find(BestFamily) : nullptr)
		{
			for (const APlanetaryBody* Body : *Current)
			{
				bWithinReach |= FVector::Distance(ObserverLocation, Body->GetActorLocation())
					<= Body->GetWorldScapeActivationRadiusCm();
			}
		}
		if (!bWithinReach)
		{
			BestFamily = ArrivingFamily;
		}
	}

	const bool bFamilyChanged = BestFamily != ResidentFamily.Get();
	for (APlanetaryBody* Body : StreamedBodies)
	{
		if (ResolveFamilyPlanet(Body) != BestFamily
			&& (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded
				|| Body->IsWorldScapeStreamingActive()))
		{
			Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
		}
	}

	if (!BestFamily)
	{
		UpdateStandbyWarmup(nullptr, Observer);
		PrewarmedBodies.Reset();
		if (ResidentFamily.IsValid())
		{
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Unloaded WorldScape family: %s"),
				*GetNameSafe(ResidentFamily.Get()));
		}
		ClearGameplayCollisionAnchor();
		ActiveBody.Reset();
		ResidentFamily.Reset();
		return;
	}

	ResidentFamily = BestFamily;
	TArray<APlanetaryBody*>& FamilyBodies = Families.FindChecked(BestFamily);
	if (bFamilyChanged)
	{
		UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Selected WorldScape family: %s (%d bodies)"),
			*BestFamily->GetName(), FamilyBodies.Num());
	}

	APlanetaryBody* BestBody = nullptr;
	double BestBodyScore = TNumericLimits<double>::Max();
	FString BestBodyKey;
	for (APlanetaryBody* Body : FamilyBodies)
	{
		const bool bCurrentBody = ActiveBody.Get() == Body;
		const double Limit = bCurrentBody
			? Body->GetWorldScapeDeactivationRadiusCm()
			: Body->GetWorldScapeActivationRadiusCm();
		const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
		const double BodyScore = APSWorldScapeStreamingPolicy::Score(
			Distance, Body->GetWorldScapeBodyRadiusCm(), Limit, bCurrentBody, 0.8);
		const FString Key = Body->GetPathName();
		if (APSWorldScapeStreamingPolicy::Prefer(BodyScore, bCurrentBody, Key,
			BestBodyScore, BestBody && BestBody == ActiveBody.Get(), BestBodyKey))
		{
			BestBodyScore = BodyScore;
			BestBody = Body;
			BestBodyKey = Key;
		}
	}

	// Rio 06.10 (planet freezes, A->B->A): leaving a planet past its moon switched the active surface planet -> moon ->
	// planet within seconds, each switch a rebuild (50-84 ms hitches). A body passed faster than aps.Surface.PassByKmPerS
	// is not activated at all, and another body of the family takes over from a still eligible active one only after
	// being the nearest for aps.Surface.ActiveSwitchMinSeconds. Within aps.Surface.ActiveSwitchNearRadii, or flying at
	// the body, the switch is immediate as before.
	{
		APlanetaryBody* const RawBest = BestBody;
		APlanetaryBody* const Current = ActiveBody.Get();
		if (!RawBest || RawBest == Current)
		{
			Policy.SwitchCandidate.Reset();
			Policy.LoggedHold.Reset();
		}
		else
		{
			if (Policy.SwitchCandidate.Get() != RawBest)
			{
				Policy.SwitchCandidate = RawBest;
				Policy.SwitchCandidateSince = PassNow;
			}
			const bool bCurrentHolds = IsValid(Current) && FamilyBodies.Contains(Current)
				&& FVector::Distance(ObserverLocation, Current->GetActorLocation()) <= Current->GetWorldScapeDeactivationRadiusCm();
			const double Dwell = CVarActiveSwitchMinSeconds.GetValueOnGameThread();
			FString HoldReason;
			if (PassesBy(RawBest))
			{
				HoldReason = FString::Printf(TEXT("is only passed (%s)"), *DescribeMotion(RawBest));
			}
			else if (bCurrentHolds && Dwell > 0.0 && !IsNear(RawBest) && !IsApproach(RawBest)
				&& PassNow - Policy.SwitchCandidateSince < Dwell)
			{
				HoldReason = FString::Printf(TEXT("has been the nearest for %.1f of %.1f s (%s)"),
					PassNow - Policy.SwitchCandidateSince, Dwell, *DescribeMotion(RawBest));
			}
			if (!HoldReason.IsEmpty())
			{
				BestBody = bCurrentHolds ? Current : nullptr;
				if (Policy.LoggedHold.Get() != RawBest)
				{
					Policy.LoggedHold = RawBest;
					UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Held WorldScape switch: %s stays active, %s %s"),
						BestBody ? *BestBody->GetName() : TEXT("none"), *RawBest->GetName(), *HoldReason);
				}
			}
			else
			{
				Policy.LoggedHold.Reset();
			}
		}
	}

	// Nothing within activation range yet: the body the observer flies to starts now, without collision.
	// Rio 06.10: not a body only passed at speed (aps.Surface.PassByKmPerS).
	bAnchorWithoutCollision = !BestBody && Arriving && ResolveFamilyPlanet(Arriving) == BestFamily && !PassesBy(Arriving);
	if (bAnchorWithoutCollision)
	{
		BestBody = Arriving;
	}

	// The selected body is never delayed by the speculative sibling preload budget.
	if (BestBody && BestBody->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded)
	{
		BestBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	}
	if (BestBody && IsValid(BestBody->PlanetaryEnvironmentGenerator))
	{
		if (AWorldScapeRoot* ActiveRoot =
			BestBody->PlanetaryEnvironmentGenerator->WorldScapeRootInstance)
		{
			// WorldScape's WITH_EDITOR path prefers the editor viewport camera even
			// during PIE. Always supply the actual gameplay observer so chunks are
			// generated under the player rather than elsewhere on the planet. The
			// subsystem owns freezing explicitly, so disable the plugin's second,
			// editor-camera-based distance freeze as well.
			ApplyGameplayObserverContract(ActiveRoot, Observer);
		}
	}
	if (APlanetaryBody* PreviousBody = ActiveBody.Get(); PreviousBody && PreviousBody != BestBody)
	{
		if (ResolveFamilyPlanet(PreviousBody) == BestFamily)
		{
			// Keep already generated siblings visible and collision-free. Only the
			// nearest body updates its chunks, so approaching a moon never erases the
			// parent planet and returning does not rebuild it from scratch.
			PreviousBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Froze resident WorldScape surface: %s"),
				*PreviousBody->GetPathName());
		}
	}

	// Bound speculative allocations only. Frozen *published* siblings still render
	// their real geometry: replacing them with the placeholder is not safe eviction.
	// Existing warm roots get a small bias to avoid churning near equal distances.
	struct FStandbyCandidate
	{
		APlanetaryBody* Body;
		double Score;
		FString Key;
	};
	TArray<FStandbyCandidate> Standby;
	for (APlanetaryBody* Body : FamilyBodies)
	{
		if (Body == BestBody || Body->bWorldScapeSurfaceReady) continue;
		const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
		const bool bWarm = Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded;
		Standby.Add({Body, APSWorldScapeStreamingPolicy::Score(Distance,
			Body->GetWorldScapeBodyRadiusCm(), TNumericLimits<double>::Max(), bWarm, 0.8),
			Body->GetPathName()});
	}
	Standby.Sort([](const FStandbyCandidate& A, const FStandbyCandidate& B)
	{
		return A.Score != B.Score ? A.Score < B.Score
			: A.Key.Compare(B.Key, ESearchCase::CaseSensitive) < 0;
	});
	const int32 MaxStandby = FMath::Clamp(CVarMaxStandbyRoots.GetValueOnGameThread(), 0, 32);
	constexpr int32 MaxSiblingPreloadsPerUpdate = 2;
	int32 SiblingPreloads = 0;
	for (int32 Index = 0; Index < Standby.Num(); ++Index)
	{
		APlanetaryBody* Body = Standby[Index].Body;
		if (Index >= MaxStandby || Standby[Index].Score == TNumericLimits<double>::Max())
		{
			if (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded)
				Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
		}
		else if (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Preloaded
			&& SiblingPreloads < MaxSiblingPreloadsPerUpdate)
		{
			// This can also reclaim a never-published frozen attempt. The generator
			// drains workers before cleanup; no worker-owned arrays are freed here.
			Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
			++SiblingPreloads;
		}
	}

	ActiveBody = BestBody;
	if (BestBody)
	{
		if (BestBody->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Active
			|| !BestBody->IsWorldScapeStreamingActive())
		{
			BestBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Activated WorldScape surface: %s"),
				*BestBody->GetPathName());
			if (IsValid(BestBody->PlanetaryEnvironmentGenerator))
			{
				HoldFirstBuild(BestBody->PlanetaryEnvironmentGenerator->WorldScapeRootInstance, true);
			}
		}
		if (IsValid(BestBody->PlanetaryEnvironmentGenerator))
		{
			if (AWorldScapeRoot* ActiveRoot =
				BestBody->PlanetaryEnvironmentGenerator->WorldScapeRootInstance)
			{
				// SetWorldScapeStreamingState(Active) can create/replace the root. Reapply
				// the observer and collision contract to that final active instance in the
				// same subsystem update rather than waiting another half second.
				ApplyGameplayObserverContract(ActiveRoot, Observer);
			}
		}
		BestBody->RefreshWorldScapeSurfaceVisibility();
	}
	else
	{
		ClearGameplayCollisionAnchor();
	}
	// Warm sequentially, and only after the foreground body has actually published.
	// Count completed speculative surfaces too: otherwise each published moon frees
	// a hidden slot and eventually every moon gets a high-resolution allocation.
	PrewarmedBodies.RemoveAll([this, BestBody, BestFamily](const TWeakObjectPtr<APlanetaryBody>& WeakBody)
	{
		const APlanetaryBody* Body = WeakBody.Get();
		return !IsValid(Body) || Body == BestBody || !Body->bWorldScapeSurfaceReady
			|| ResolveFamilyPlanet(WeakBody.Get()) != BestFamily;
	});
	APlanetaryBody* WarmCandidate = nullptr;
	if (!IsValid(FlightReplacement) && BestBody && BestBody->bWorldScapeSurfaceReady && PrewarmedBodies.Num() < MaxStandby)
	{
		for (int32 Index = 0; Index < FMath::Min(MaxStandby, Standby.Num()); ++Index)
		{
			APlanetaryBody* Body = Standby[Index].Body;
			if (Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Preloaded
				&& !Body->bWorldScapeSurfaceReady)
			{
				if (!WarmCandidate || Body == WarmingBody.Get()) WarmCandidate = Body;
			}
		}
	}
	UpdateStandbyWarmup(WarmCandidate, Observer);
	for (APlanetaryBody* Body : FamilyBodies)
	{
		if (Body->bWorldScapeSurfaceReady && IsValid(Body->PlanetaryEnvironmentGenerator)
			&& Body->PlanetaryEnvironmentGenerator->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Water)
			VisibleLiquidBodies.Add(Body);
	}
}
