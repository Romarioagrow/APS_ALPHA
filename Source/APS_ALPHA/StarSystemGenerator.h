// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StarSystemGenerationModel.h"
#include "PlanetarySystemGenerationModel.h"
#include "StarGenerator.h"
#include "PlanetGenerator.h"
#include "MoonGenerator.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "StarSystemGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UStarSystemGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()
	
public:
	UStarSystemGenerator();

public:
	// функция генерации стартовой системы
	void GenerateHomeStarSystem(FStarSystemGenerationModel StarSystemModel, UWorld* World); //FStartSystemParams Params

	// функция генерации общей звездной системы
	void GenerateRandomStarSystem(FStarSystemGenerationModel StarSystemModel, UWorld* World); //FStartSystemParams Params

	FStarSystemGenerationModel GenerateHomeStarSystemModel();

	FPlanetarySystemGenerationModel GenerateRandomPlanetraySystemModel();

	FStarSystemGenerationModel GenerateRandomStarSystemModel();

private:
	void CreatePlanetarySystem(FPlanetarySystemGenerationModel PlanetarySystemModel); //FPlanetarySystemParams Params

	//AStarSystem* SpawnNewStarSystem(FStarSystemGenerationModel StarSystemModel);

public:
	UPROPERTY()
		UStarGenerator* StarGenerator;

	UPROPERTY()
		UPlanetGenerator* PlanetGenerator;

	UPROPERTY()
		UMoonGenerator* MoonGenerator;
};
