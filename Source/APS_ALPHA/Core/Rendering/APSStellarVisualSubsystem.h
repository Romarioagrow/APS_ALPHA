#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSStellarVisualSubsystem.generated.h"

class ADirectionalLight;
class AGravityCharacterPawn;
class APlanetaryBody;
class APointLight;
class ASpaceStation;

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
	void UpdatePreviewFillLight(
		const APlanetaryBody* PreviewBody,
		const FVector& PreviewCameraLocation,
		bool bHasPreviewCameraLocation,
		float DeltaTime);
	void UpdateGameplayStationFillLight(
		const AGravityCharacterPawn* CharacterPawn,
		const FVector& CameraLocation,
		bool bHasCameraLocation);

	TWeakObjectPtr<ADirectionalLight> DirectionalLight;
	/**
	 * Menu-only camera fill. It keeps the selected WorldScape material readable on
	 * the physical night side without moving or replacing the parent-star light.
	 */
	TWeakObjectPtr<ADirectionalLight> PreviewFillLight;
	/** Camera-local readability fill used only while a character is inside a station. */
	TWeakObjectPtr<APointLight> GameplayStationFillLight;
	TWeakObjectPtr<ASpaceStation> GameplayFillStation;
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
	int32 OriginalForwardShadingPriority{0};
	TEnumAsByte<EComponentMobility::Type> OriginalMobility{EComponentMobility::Movable};
	FString ActiveStarIdentity;
};
