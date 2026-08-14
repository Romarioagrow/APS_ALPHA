#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPlanetSurfaceDiagnosticsSubsystem.generated.h"

class APawn;
class AWorldScapeRoot;
class UMaterialInterface;

/**
 * Opt-in evidence collector for the selected gameplay WorldScape anchor.
 * Disabled by default so the accepted surface baseline pays no runtime cost.
 */
UCLASS()
class APS_ALPHA_API UAPSPlanetSurfaceDiagnosticsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	AWorldScapeRoot* ResolveAnchoredRoot(const APawn* Observer) const;
	void ResetSample();

	float SampleElapsed{0.0f};
	float HeartbeatElapsed{0.0f};
	FVector LastLod0RelativePosition{FVector::ZeroVector};
	FVector LastSnappedAngle{FVector::ZeroVector};
	int32 LastVertexCount{INDEX_NONE};
	int32 LastTriangleCount{INDEX_NONE};
	uint64 PublicationSequence{0};
	bool bSampleValid{false};
	TWeakObjectPtr<AWorldScapeRoot> LastRoot;
	TWeakObjectPtr<UMaterialInterface> LastTerrainMaterial;
};
