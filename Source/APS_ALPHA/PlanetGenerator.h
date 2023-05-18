// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Planet.h"
#include "PlanetGenerationModel.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "PlanetGenerator.generated.h"

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
	// Функция генерации планеты
	APlanet* GeneratePlanet(FPlanetGenerationModel PlanetModel); // FPlanetParams Params

	FPlanetGenerationModel GenerateRandomPlanetModel();

	void ApplyModel(AAstroActor* AstroActor, FGenerationModel GenerationModel); //override;
};
