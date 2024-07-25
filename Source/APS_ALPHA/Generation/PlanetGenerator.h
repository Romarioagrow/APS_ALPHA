#pragma once

#include "MoonGenerator.h"
#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "APS_ALPHA/Core/Structs/PlanetGenerationModel.h"
#include "PlanetGenerator.generated.h"

class AStar;
class APlanet;

UCLASS()
class APS_ALPHA_API UPlanetGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UPlanetGenerator();

	void ConnectPlanetWithStar(APlanet* NewPlanet, AStar* NewStar);

	FPlanetModel GenerateRandomPlanetModel();

	void ApplyModel(APlanet* PlanetActor, TSharedPtr<FPlanetModel> PlanetGenerationModel);

	TArray<FMoonData> GenerateMoonsList(FPlanetModel PlanetModel);

	double CalculateOrbitHeight(EOrbitHeight OrbitHeightType, double PlanetRadius);

	void CalculateLagrangePoints();
};
