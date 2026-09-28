#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSObjectLightingSubsystem.generated.h"

class ADirectionalLight;
class APostProcessVolume;
class ULocalLightComponent;

/**
 * Playable, consistent lighting for gameplay objects (Rio, 2026-09-28): the generated star stays the
 * key light, while
 * - one camera-aligned fill on lighting channel 1 keeps ships, stations and the pilot readable on
 *   their shadow side everywhere (planets and terrain stay on channel 0 and keep their look);
 * - station-owned lamps are scaled down for the project's fixed exposure (interiors were blown out);
 * - an optional, range-limited auto exposure can be switched on for comparison.
 * Every value is a console variable (aps.Lighting.*) so it can be tuned live in PIE.
 */
UCLASS()
class APS_ALPHA_API UAPSObjectLightingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	void UpdateObjectFill(const FVector& CameraLocation, const FRotator& CameraRotation, bool bInsideStation);
	void RefreshObjects();
	void ApplyStationLightScale();
	void UpdateExposure();

	TWeakObjectPtr<ADirectionalLight> ObjectFillLight;
	TWeakObjectPtr<APostProcessVolume> ExposureVolume;
	/** Actors already opted into the fill channel. */
	TSet<TWeakObjectPtr<AActor>> OptedInActors;
	/** Authored intensity of every station-owned local light, so the scale can change live. */
	TMap<TWeakObjectPtr<ULocalLightComponent>, float> StationLightBaseIntensity;
	float AppliedStationLightScale{-1.0f};
	float RefreshElapsed{0.0f};
};
