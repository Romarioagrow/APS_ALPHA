#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpaceshipFleetSubsystem.generated.h"

/**
 * GameMode-independent bootstrap for generated ship hulls. It also runs once in
 * editor worlds, so converted actors are visible in the Outliner and can be saved
 * with the level without Blueprint or World Settings edits.
 */
UCLASS()
class APS_ALPHA_API USpaceshipFleetSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override { return true; }

	UFUNCTION(BlueprintCallable, Category = "Ships|Runtime Fleet")
	int32 BuildRuntimeShipFleet();

private:
	bool bFleetBuilt{false};
};
