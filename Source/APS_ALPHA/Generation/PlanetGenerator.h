// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/*#include "Planet.h"
#include "PlanetGenerationModel.h"
#include "OrbitHeight.h"*/
#include "MoonGenerator.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "APS_ALPHA/Core/Structs/PlanetGenerationModel.h"
#include "PlanetGenerator.generated.h"

//enum class EOrbitHeight;
class AStar;
class APlanet;
/**
 * 
 */
UCLASS()
class APS_ALPHA_API UPlanetGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UPlanetGenerator();

public:
	void ConnectPlanetWithStar(APlanet* NewPlanet, AStar* NewStar);

	//double CalculateMoonRadius(EMoonType MoonType);

	FPlanetModel GenerateRandomPlanetModel();

	void ApplyModel(APlanet* PlanetActor, TSharedPtr<FPlanetModel> PlanetGenerationModel);

	TArray<FMoonData> GenerateMoonsList(FPlanetModel PlanetModel);

	double CalculateOrbitHeight(EOrbitHeight OrbitHeightType, double PlanetRadius);

	void CalculateLagrangePoints();
};
