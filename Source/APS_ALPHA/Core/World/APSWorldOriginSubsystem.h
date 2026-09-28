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
	/** Shifts the world so WorldLocation becomes 0,0,0. Returns true when the world moved. */
	bool RebaseOnto(const FVector& WorldLocation, const TCHAR* Reason);

	/** Accumulated engine origin: generation frame = world + offset. */
	FVector GetOriginOffset() const;

	FVector ToGenerationFrame(const FVector& WorldLocation) const { return WorldLocation + GetOriginOffset(); }
	FVector FromGenerationFrame(const FVector& GenerationLocation) const { return GenerationLocation - GetOriginOffset(); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
