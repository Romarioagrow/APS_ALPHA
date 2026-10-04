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
	bool ShiftWorldBy(const FVector& Offset, const TCHAR* Reason);
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
	/** The pilot (and its components) whose ticks the map paused, given back once the origin is the pilot's again. */
	TWeakObjectPtr<APawn> MapHeldPawn;
	TArray<TWeakObjectPtr<UObject>> MapHeldTicks;
	void ReleaseMapHold();
};
