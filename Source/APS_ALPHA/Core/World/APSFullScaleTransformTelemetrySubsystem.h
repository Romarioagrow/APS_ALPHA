#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSFullScaleTransformTelemetrySubsystem.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/** Retains one deterministic far-field HISM probe between telemetry samples. */
struct FAPSFullScaleHismSentinelState
{
	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> Component;
	int32 InstanceCount{INDEX_NONE};
	int32 InstanceIndex{INDEX_NONE};
	bool bHasSample{false};
	bool bHadRenderedView{false};
	bool bHadProjectedNdc{false};
	FVector LastWorldLocation{FVector::ZeroVector};
	FVector LastViewRelativeLocation{FVector::ZeroVector};
	FVector2D LastProjectedNdc{FVector2D::ZeroVector};
};

/**
 * Default-off P0 evidence collector for post-generation transform stability.
 * It never moves actors or changes presentation state.
 */
UCLASS()
class APS_ALPHA_API UAPSFullScaleTransformTelemetrySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void ResetSample();

	float SampleElapsed{0.0f};
	uint64 Sequence{0};
	bool bHasSample{false};
	TWeakObjectPtr<AActor> LastGenerator;
	TWeakObjectPtr<AActor> LastHomeSystem;
	TWeakObjectPtr<AActor> LastPawn;
	FVector LastGeneratorLocation{FVector::ZeroVector};
	FVector LastHomeSystemLocation{FVector::ZeroVector};
	FVector LastPawnLocation{FVector::ZeroVector};
	FVector LastCameraLocation{FVector::ZeroVector};
	bool bHadRenderedView{false};
	FVector LastRenderedViewLocation{FVector::ZeroVector};
	FAPSFullScaleHismSentinelState GalaxySentinel;
	FAPSFullScaleHismSentinelState ClusterSentinel;
};
