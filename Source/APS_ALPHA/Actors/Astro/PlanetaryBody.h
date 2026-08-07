#pragma once

#include "CoreMinimal.h"
#include "OrbitalBody.h"
#include "APS_ALPHA/Actors/Planetary/PlanetaryEnvironment.h"
#include "APS_ALPHA/Actors/Planetary/PlanetAtmosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetBiosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetGeosphere.h"
#include "Kismet/GameplayStatics.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "PlanetaryBody.generated.h"

class APlanetarySurfaceGenerator;
class AWorldScapeRoot;
enum class EPlanetType : uint8;

UENUM(BlueprintType)
enum class EWorldScapeSurfaceState : uint8
{
	Unloaded,
	Preloaded,
	FrozenVisible,
	Active
};

UCLASS()
class APS_ALPHA_API APlanetaryBody : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()

public:
	APlanetaryBody();

protected:
	virtual void BeginPlay() override;
	

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Data")
	FPlanetData PlanetData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Body")
	APlanetarySurfaceGenerator* PlanetaryEnvironmentGenerator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	bool bGenerateByDefault{false};

	UPROPERTY()
	APawn* PlayerPawn{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double WscZoneScale{10.0};

	/** Surface generation starts while the body is still a small disk on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming", meta=(ClampMin="1.1"))
	double WorldScapeActivationRadiusMultiplier{96.0};

	/** Larger than the activation radius to prevent rapid load/unload oscillation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming", meta=(ClampMin="1.2"))
	double WorldScapeDeactivationRadiusMultiplier{128.0};

	/** Family metadata becomes resident before the nearest body needs detailed terrain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming", meta=(ClampMin="1.2"))
	double WorldScapePreloadRadiusMultiplier{144.0};

	/** A complete planet/moon family remains known until every member is outside this radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming", meta=(ClampMin="1.2"))
	double WorldScapeUnloadRadiusMultiplier{192.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming")
	bool bStreamWorldScapeSurface{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	bool bEnvironmentSpawned{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Body|Streaming")
	EWorldScapeSurfaceState WorldScapeSurfaceState{EWorldScapeSurfaceState::Unloaded};

	/** Becomes true only after WorldScape has produced an actual visible mesh section. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Planet Body|Streaming")
	bool bWorldScapeSurfaceReady{false};

	/** Stable procedural surface seed. Zero derives it once from the persistent body identity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming")
	int32 WorldScapeSeed{0};

	/** Resolver multipliers copied from the selected planet model. Asset references
	 * remain owned by UAPSPlanetSurfaceProfileResolver and are never exposed here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Surface", meta = (ClampMin = "0.25", ClampMax = "4.0"))
	double SurfaceFeatureScale{1.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Surface", meta = (ClampMin = "0.25", ClampMax = "2.5"))
	double SurfaceReliefScale{1.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Surface", meta = (ClampMin = "0.25", ClampMax = "2.0"))
	double SurfaceLandCoverageScale{1.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Surface", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	double SurfaceMountainScale{1.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Surface", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	double SurfaceCraterScale{1.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Surface", meta = (ClampMin = "0.25", ClampMax = "2.0"))
	double SurfaceRoughnessScale{1.0};

	/**
	 * Uniform presentation scale used only by disposable, normalized menu previews.
	 * Runtime worlds keep 1.0, so the WorldScape radius remains physically full-scale.
	 */
	UPROPERTY(Transient)
	double WorldScapePresentationScale{1.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double AtmosphereHeight{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double OrbitHeight{0.0};

	UPROPERTY(EditAnywhere, Category = "Planet")
	int PlanetRadiusKM{0};

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	int32 Temperature{0};

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	double PlanetDensity{0};

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	double PlanetGravityStrength{0};

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	EPlanetType PlanetType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Environment")
	FPlanetAtmosphere PlanetAtmosphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Environment")
	FPlanetBiosphere PlanetBiosphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Environment")
	FPlanetGeosphere PlanetGeosphere;

	void FillPlanetData();

	APlanetarySurfaceGenerator* EnsurePlanetaryEnvironmentGenerator();
	bool EnsureWorldScapeSurface();
	void SetWorldScapeStreamingState(EWorldScapeSurfaceState NewState);
	void SetWorldScapeStreamingActive(bool bActive);
	bool IsWorldScapeStreamingActive() const;
	bool RefreshWorldScapeSurfaceVisibility();
	EWorldScapeSurfaceState GetWorldScapeStreamingState() const { return WorldScapeSurfaceState; }
	double GetWorldScapeActivationRadiusCm() const;
	double GetWorldScapeBodyRadiusCm() const;
	double GetWorldScapeDeactivationRadiusCm() const;
	double GetWorldScapePreloadRadiusCm() const;
	double GetWorldScapeUnloadRadiusCm() const;

};
