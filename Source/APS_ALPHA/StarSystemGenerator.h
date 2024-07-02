// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StarSystemGenerationModel.h"
#include "PlanetarySystemGenerationModel.h"
#include "StarGenerator.h"
#include "PlanetGenerator.h"
#include "MoonGenerator.h"
#include "StarSystem.h"

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
	void ApplyModel(AStarSystem* StarSystem, TSharedPtr<FStarSystemModel> StarSystemGenerationModel);

	void GenerateCustomHomeSystemModel(TSharedPtr<FStarSystemModel> StarSystemModel);

	void GenerateRandomStarSystemModel(TSharedPtr<FStarSystemModel> StarSystemModel);
};
