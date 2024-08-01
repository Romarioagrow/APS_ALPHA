#pragma once

#include "MoonGenerator.h"
#include "CoreMinimal.h"
#include "AstroGenerator.h"
#include "BaseProceduralGenerator.h"
#include "APS_ALPHA/Core/Structs/PlanetGenerationModel.h"
#include "PlanetGenerator.generated.h"

struct FPlanetAtmosphereModel;
class AStar;
class APlanet;

UCLASS()
class APS_ALPHA_API UPlanetGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UPlanetGenerator();

	void CalculateLagrangePoints();

	void ConnectPlanetWithStar(APlanet* NewPlanet, AStar* NewStar);
	
	void ApplyModel(APlanet* PlanetActor, TSharedPtr<FPlanetModel> PlanetGenerationModel);

	double CalculateOrbitHeight(EOrbitHeight OrbitHeightType, double PlanetRadius);

	FPlanetModel GenerateRandomPlanetModel();

	TArray<FMoonData> GenerateMoonsList(FPlanetModel PlanetModel);
	
	APlanet* GeneratePlanet(const TSharedPtr<FPlanetModel>& PlanetModel, const TSubclassOf<APlanet> PlanetClass, UWorld* World);

	TSharedPtr<FPlanetModel> CreatePlanetModelFromGeneratedWorld(const UGeneratedWorld* GeneratedWorld);
	
	void GeneratePlanetAtmosphere(APlanet* Planet, const TSharedPtr<FPlanetAtmosphereModel>& PlanetAtmosphereMode);
	
	TSharedPtr<FPlanetAtmosphereModel> CreateAtmosphereModelFromGeneratedWorld(UGeneratedWorld* GeneratedWorld);

private:
	const double KM_TO_CM = 100000.0;

	const double SCALE_FACTOR = 1.0 / 50.0;
	
};
