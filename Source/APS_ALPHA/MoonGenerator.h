// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Moon.h"
#include "MoonGenerationModel.h"
#include "PlanetGenerationModel.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "MoonGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UMoonGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UMoonGenerator();

public:
	// Функция генерации луны
	//AMoon* GenerateMoon(); // FMoonParams Params
	void ApplyModel(AAstroActor* AstroActor, FGenerationModel GenerationModel);

	FMoonGenerationModel GenerateMoonModel();

	FMoonGenerationModel GenerateRandomMoonModel();

	EMoonType GenerateMoonType(FPlanetGenerationModel PlanetModel);

	double CalculateMoonMass(EMoonType MoonType);

	double CalculateMoonRadius(EMoonType MoonType);

	// override;
};
