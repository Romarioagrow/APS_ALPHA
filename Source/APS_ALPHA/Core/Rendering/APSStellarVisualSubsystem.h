#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSStellarVisualSubsystem.generated.h"

class ADirectionalLight;
class APlanetaryBody;
class APointLight;
class APawn;
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
	/** Snapshot used by runtime parity tests and diagnostics; never mutates selection. */
	bool GetActiveStellarTarget(FVector& OutTargetLocation, FString& OutTargetIdentity) const;

private:
	void ResolveDirectionalLight();
	void ResolveNearestStar(const FVector& ObserverLocation);
	void UpdatePreviewFillLight(
		const APlanetaryBody* PreviewBody,
		const FVector& PreviewCameraLocation,
		bool bHasPreviewCameraLocation,
		float DeltaTime);
	void UpdateGameplayStationFillLight(
		const APawn* CharacterPawn,
		const FVector& CameraLocation,
		bool bHasCameraLocation);
	void UpdateGameplaySurfaceFillLight(
		const APawn* CharacterPawn,
		const FVector& ObserverLocation,
		bool bHasObserverLocation,
		float DeltaTime);

	TWeakObjectPtr<ADirectionalLight> DirectionalLight;
	/**
	 * Menu-only camera fill. It keeps the selected WorldScape material readable on
	 * the physical night side without moving or replacing the parent-star light.
	 */
	TWeakObjectPtr<ADirectionalLight> PreviewFillLight;
	/** Camera-local readability fill used only while a character is inside a station. */
	TWeakObjectPtr<APointLight> GameplayStationFillLight;
	TWeakObjectPtr<ASpaceStation> GameplayFillStation;
	/**
	 * Low-energy off-axis daylight fill used only while the gameplay pawn is close
	 * to an active physical WorldScape surface. It does not own geometry, materials
	 * or displacement; it only keeps the real mesh normals readable with the
	 * project's fixed exposure when the generated star is at a grazing angle.
	 */
	TWeakObjectPtr<ADirectionalLight> GameplaySurfaceFillLight;
	TWeakObjectPtr<APlanetaryBody> GameplayFillBody;
	FVector TargetStarLocation{FVector::ZeroVector};
	FLinearColor TargetLightColor{FLinearColor::White};
	FLinearColor SmoothedLightColor{FLinearColor::White};
	float TargetLightIntensity{10.0f};
	float SmoothedLightIntensity{10.0f};
	float SearchElapsed{0.0f};
	bool bHasTargetStar{false};
	bool bCapturedOriginalLight{false};
	/** True only for the transient gameplay key created when the level has no authored sun. */
	bool bOwnsDirectionalLight{false};
	FRotator OriginalLightRotation{FRotator::ZeroRotator};
	FLinearColor OriginalLightColor{FLinearColor::White};
	float OriginalLightIntensity{10.0f};
	int32 OriginalForwardShadingPriority{0};
	TEnumAsByte<EComponentMobility::Type> OriginalMobility{EComponentMobility::Movable};
	FString ActiveStarIdentity;
};
