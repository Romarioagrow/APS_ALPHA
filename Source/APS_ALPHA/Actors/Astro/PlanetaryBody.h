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

	/** Surface generation starts at this many body radii from the center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming", meta=(ClampMin="1.1"))
	double WorldScapeActivationRadiusMultiplier{12.0};

	/** Larger than the activation radius to prevent rapid load/unload oscillation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming", meta=(ClampMin="1.2"))
	double WorldScapeDeactivationRadiusMultiplier{18.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body|Streaming")
	bool bStreamWorldScapeSurface{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	bool bEnvironmentSpawned{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double AtmosphereHeight{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double OrbitHeight{0.0};

	UPROPERTY(EditAnywhere, Category = "Planet")
	int PlanetRadiusKM;

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
	void SetWorldScapeStreamingActive(bool bActive);
	bool IsWorldScapeStreamingActive() const;
	double GetWorldScapeActivationRadiusCm() const;
	double GetWorldScapeDeactivationRadiusCm() const;

};
