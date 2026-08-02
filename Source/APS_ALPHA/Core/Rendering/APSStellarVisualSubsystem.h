#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSStellarVisualSubsystem.generated.h"

class ADirectionalLight;

/** Runtime-only bridge from the current generated star to playable global lighting. */
UCLASS()
class APS_ALPHA_API UAPSStellarVisualSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	void ResolveDirectionalLight();
	void ResolveNearestStar(const FVector& ObserverLocation);

	TWeakObjectPtr<ADirectionalLight> DirectionalLight;
	FVector TargetStarLocation{FVector::ZeroVector};
	FLinearColor TargetLightColor{FLinearColor::White};
	FLinearColor SmoothedLightColor{FLinearColor::White};
	float TargetLightIntensity{10.0f};
	float SmoothedLightIntensity{10.0f};
	float SearchElapsed{0.0f};
	bool bHasTargetStar{false};
	bool bCapturedOriginalLight{false};
	FRotator OriginalLightRotation{FRotator::ZeroRotator};
	FLinearColor OriginalLightColor{FLinearColor::White};
	float OriginalLightIntensity{10.0f};
	TEnumAsByte<EComponentMobility::Type> OriginalMobility{EComponentMobility::Movable};
	FString ActiveStarIdentity;
};
