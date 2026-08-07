#pragma once

#include "CoreMinimal.h"
#include "WorldScapeNoise/Public/WorldScapeNoiseClass.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APSWorldScapePlanetNoise.generated.h"

/**
 * One immutable, per-planet WorldScape generator. Data.Height carries signed physical displacement while
 * WorldScape material channels stay normalized (R=height 0..1, G=temperature, B=humidity), without
 * mutating a shared noise asset.
 */
UCLASS(BlueprintType)
class APS_ALPHA_API UAPSWorldScapePlanetNoise : public UWorldScapeNoiseClass
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface")
	FAPSResolvedPlanetSurfaceProfile SurfaceProfile;

	void Configure(const FAPSResolvedPlanetSurfaceProfile& InProfile);

	virtual FNoiseData GetNoise(
		CustomNoise NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld,
		double Latitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;

	virtual FNoiseData GetOceanNoise(
		CustomNoise NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld,
		double Latitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;

	/**
	 * Pure resolved-profile sampler used by the closed orbital preview mesh. Passing
	 * the already seeded noise state by reference avoids copying it once per vertex
	 * while preserving exactly the same height and RGB material-channel contract as
	 * WorldScape's terrain worker.
	 */
	FNoiseData SampleResolved(
		CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
		DVector& NoisePosition) const;

private:
	FNoiseData Evaluate(
		CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
		DVector& NoisePosition) const;
};
