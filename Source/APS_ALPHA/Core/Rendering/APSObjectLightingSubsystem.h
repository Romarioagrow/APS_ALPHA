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
	/**
	 * Rio 04.10 evening ("from orbit the night side is an absolutely black body, it looks like a bug: let a silhouette
	 * show, beautifully"): above a world, past the near-surface fill's reach, a faint cool light travelling towards the
	 * star lights only what faces away from it, so a night side reads as a dim moonlit globe with its relief. What the
	 * star lights keeps exactly its look. aps.Planet.NightFill sets its lux (0 = off).
	 */
	void UpdateNightFill(float DeltaTime);
	void RefreshObjects();
	void ApplyStationLightScale();
	void UpdateExposure();

	TWeakObjectPtr<ADirectionalLight> ObjectFillLight;
	TWeakObjectPtr<ADirectionalLight> NightFillLight;
	float NightFillIntensity{0.0f};
	TWeakObjectPtr<APostProcessVolume> ExposureVolume;
	/** Actors already opted into the fill channel. */
	TSet<TWeakObjectPtr<AActor>> OptedInActors;
	/** Authored intensity of every station-owned local light, so the scale can change live. */
	TMap<TWeakObjectPtr<ULocalLightComponent>, float> StationLightBaseIntensity;
	float AppliedStationLightScale{-1.0f};
	float RefreshElapsed{0.0f};
};
