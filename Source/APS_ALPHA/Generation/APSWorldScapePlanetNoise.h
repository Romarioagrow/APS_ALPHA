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

	void Configure(const FAPSResolvedPlanetSurfaceProfile& InProfile, bool bInUnifiedLavaSurface = false,
		bool bInCoastalReliefCandidate = false);

	// Capture on the owning thread together with SurfaceProfile before launching
	// value-sampler workers. Never reconstruct this from a command line on workers.
	bool UsesCoastalReliefCandidate() const { return bCoastalReliefCandidate; }

	/**
	 * Collision-only signed displacement of THIS configured generator, including
	 * its coastal flag and optional unified-lava envelope. Immutable while sampled.
	 * Does not sample material/climate channels or WorldScape volume overrides.
	 * Caller must supply native root noise coordinates/settings, not world space.
	 * Native collision integration is not enabled yet; use only after the owning
	 * root's heightmap/volume/custom-sampler guards and end-to-end validation.
	 */
	double SampleCollisionHeight(CustomNoise& NoiseClass, const DVector& Position,
		const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity,
		double PlanetScale, double Latitude) const;

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
	/**
	 * Thread-safe value sampler for preview mesh workers. The caller supplies an
	 * immutable profile snapshot and task-local noise state, so this path never reads
	 * or writes a UObject while preserving the exact resolved-profile math.
	 * An existing generator's caller must also capture UsesCoastalReliefCandidate().
	 */
	static FNoiseData SampleResolvedProfile(
		const FAPSResolvedPlanetSurfaceProfile& SurfaceProfile,
		CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
		DVector& NoisePosition, bool bCoastalReliefCandidate = false);
	/**
	 * Same signed displacement field, in the same centimetres as GetNoise(). Skips
	 * material/climate/foliage channels, not terrain frequency bands. Callers must
	 * supply the actual full-scale root settings for physical bathymetry; a preview's
	 * compressed settings deliberately describe different low-bandwidth geometry.
	 * Does not apply WorldScape noise/heightmap volumes or the ocean-height clamp.
	 * Independently optimized full/height paths may differ by floating-point roundoff.
	 * Pass the owning generator's captured coastal flag when sampling its field.
	 */
	static double SampleHeightResolvedProfile(
		const FAPSResolvedPlanetSurfaceProfile& SurfaceProfile,
		CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
		bool bCoastalReliefCandidate = false);

private:
	// Configure only while the owning root has no in-flight workers. Never enable
	// independently of the compatible material and removal of the ocean mesh.
	bool bUnifiedLavaSurface = false;
	// Immutable for the lifetime of an in-flight terrain/collision worker.
	bool bCoastalReliefCandidate = false;

	FNoiseData Evaluate(
		CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
		DVector& NoisePosition) const;
	template<bool bHeightOnly>
	static FNoiseData EvaluateProfile(
		const FAPSResolvedPlanetSurfaceProfile& SurfaceProfile,
		CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
		double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
		DVector& NoisePosition, bool bUseCoastalRelief);
};
