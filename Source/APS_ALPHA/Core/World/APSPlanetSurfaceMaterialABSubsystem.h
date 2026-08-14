#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPlanetSurfaceMaterialABSubsystem.generated.h"

class UMaterialInstanceDynamic;
class AWorldScapeRoot;
class UWorldScapeLod;

/**
 * Opt-in runtime A/B harness for classifying surface material crawl.
 * Disabled by default and always restores the anchored terrain MID on exit.
 */
UCLASS()
class APS_ALPHA_API UAPSPlanetSurfaceMaterialABSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	UMaterialInstanceDynamic* ResolveAnchoredTerrainMaterial(
		AWorldScapeRoot*& OutRoot, UWorldScapeLod*& OutLod0) const;
	void LogLodLifecycleState(
		const AWorldScapeRoot* Root, const UWorldScapeLod* Lod0, int32 Preset);
	void CaptureOriginals(UMaterialInstanceDynamic* Material);
	void ApplyPreset(UMaterialInstanceDynamic* Material, int32 Preset);
	void RestoreActiveMaterial();

	float SampleElapsed{0.0f};
	int32 AppliedPreset{0};
	TWeakObjectPtr<UMaterialInstanceDynamic> ActiveMaterial;
	TMap<FName, float> OriginalScalars;
	FString LastLifecycleState;
};
