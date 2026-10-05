#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSWorldOriginSubsystem.generated.h"

/**
 * Keeps the player near world 0,0,0 (Rio, 2026-09-29).
 *
 * Generation builds a full-scale system with the orbital headquarters at 0,0,0, so a surface start put the player
 * 600-11 000 km from the origin (median 2 571 km in the logs). RebaseOnto shifts the whole world with the engine's
 * UWorld::SetNewWorldOrigin: every actor, physics body, WorldScape chunk, light and render proxy moves by the same
 * offset, so nothing changes on screen while the player's spot becomes 0,0,0. WorldScape and the planet materials
 * already follow engine rebases (Pre/PostWorldOriginOffset).
 *
 * Saves keep the generation frame: ToGenerationFrame / FromGenerationFrame convert with the accumulated offset
 * (UWorld::OriginLocation), so saves written before and after this change load the same way.
 */
UCLASS()
class APS_ALPHA_API UAPSWorldOriginSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Shifts the world so WorldLocation becomes 0,0,0. Returns true when the world moved now. While the generator's
	 * star catalogue is not final yet the shift waits for it and then centres on the player pawn (returns false).
	 */
	bool RebaseOnto(const FVector& WorldLocation, const TCHAR* Reason);

	/** Accumulated origin (the engine's and the floating one): generation frame = world + offset. */
	FVector GetOriginOffset() const;

	/**
	 * Rio 03.10 ("far out the physics breaks: shift the world, carefully, without freezes; nothing should change on
	 * screen"): moves every actor and component, the render scene and the navigation by Offset in double precision, with
	 * UWorld::SetNewWorldOrigin's own steps but past its FIntVector origin (about 21 000 km a side), and folds the shift
	 * into the generation frame the saves use. The VSM clipmap panning is switched off for the jump first.
	 */
	bool ShiftWorldBy(const FVector& Offset, const TCHAR* Reason, bool bQuiet = false, AActor* Carrier = nullptr);
	/**
	 * Rio 05.10 (REAL SCALE: at billions of c the hull, the camera and the pilot walking aboard jumped by metres every
	 * frame): a fast ship carrying the player used to move by its whole step (up to 4e18 cm a frame, where a double's step
	 * is 5 m) before the world shifted back, so its parts, the camera and the passengers were placed with metres of
	 * rounding. Here the whole grains of the step go straight into the world shift, the ship and what rides it staying
	 * put; the ship then moves by the returned remainder only. The speed, the physics and the drawn flight are the same.
	 * Returns Delta unchanged when the step is short or the ship is not the player's fast REAL SCALE ship.
	 */
	FVector FlowPastShip(AActor& Ship, const FVector& Delta, double SpeedCmPerS);
	/** Rio 05.10 evening: after the ship's own step that followed a flow, its riders' velocity state forgets the shift. */
	void FinishFlowMove(AActor& Ship);
	/** The floating origin, once a frame: far from 0,0,0 at a calm moment (a slow ship, or on foot), the world shifts so
	 * the player stands at 0,0,0 again; a fast ship shifts once it is farther out than a few seconds of its flight. */
	void UpdateFloatingOrigin(float DeltaSeconds);
	/**
	 * Rio 04.10 ("from ~900 AU the home planet's layers slide apart on the map"): the strategic map's camera asks, every
	 * frame it holds still, to bring its look point near 0,0,0. The next update shifts the world there when the view is
	 * close to something far (aps.WorldOrigin.MapShift) and the pilot is a ship in open space; when the map closes the
	 * pilot is the origin again at once.
	 */
	void RequestMapView(const FVector& FocusLocation, double ViewDistanceCm);

	FVector ToGenerationFrame(const FVector& WorldLocation) const { return WorldLocation + GetOriginOffset(); }
	FVector FromGenerationFrame(const FVector& GenerationLocation) const { return GenerationLocation - GetOriginOffset(); }
	/** Rio 05.10 evening: the world flowed past a fast REAL SCALE ship within the last Seconds (it shifts every frame). */
	bool IsWorldFlowing(const double Seconds = 0.5) const { return SinceFlowShiftSeconds < Seconds; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool RebaseNow(const FVector& WorldLocation, const TCHAR* Reason);
	/** A generator whose canonical star catalogue has not been finalized yet (it validates bounds in its frame). */
	bool IsStellarCatalogueSettling() const;
	void RetryDeferredRebase();

	bool bRebaseDeferred{false};
	int32 DeferredRebaseTicks{0};
	FString DeferredRebaseReason;
	/** ShiftWorldBy's shifts, past the engine origin: generation frame = world + engine origin + this. */
	FVector FloatOrigin{FVector::ZeroVector};
	double SinceFloatShiftSeconds{1000.0};
	/** The pawn the floating origin followed last: a new one (the pilot getting up, boarding) waits before any shift. */
	TWeakObjectPtr<APawn> FloatAnchorPawn;
	/** The strategic map's latest look point and view distance (RequestMapView), taken by the next update. */
	bool bMapRequest{false};
	FVector MapFocusLocation{FVector::ZeroVector};
	double MapViewDistanceCm{0.0};
	/** The map brought its view near 0,0,0, away from the pilot: back to the pilot as soon as it closes. */
	bool bMapShifted{false};
	/** Shifts the world to the map's far view; a pilot not in open space holds still meanwhile. True when it shifted. */
	bool TryMapShift();
	/** Rio 05.10 afternoon (flight FPS): re-creates the render state of the Nanite meshes (the culling grid files them anew),
	 * all of them or only those near NearOnly; the carried actors' meshes never moved. Returns how many. */
	int32 RefileNanite(const FVector* NearOnly, const TArray<AActor*>& Carried);
	/** Far Nanite meshes skipped by flow shifts; re-filed once the flow has stopped for a moment. */
	bool bNaniteRefileOwed{false};
	double SinceFlowShiftSeconds{1000.0};
	/** The last flow told the renderer its view stayed with the ship (aps.RealScale.FlowViewStill); FinishFlowMove resets. */
	bool bFlowViewStill{false};
	double LastFlowViewLogSeconds{0.0};
	/** The pilot (and its components) whose ticks the map paused, given back once the origin is the pilot's again. */
	TWeakObjectPtr<APawn> MapHeldPawn;
	TArray<TWeakObjectPtr<UObject>> MapHeldTicks;
	void ReleaseMapHold();
};
