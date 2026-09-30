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

	/** Accumulated engine origin: generation frame = world + offset. */
	FVector GetOriginOffset() const;

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
};
