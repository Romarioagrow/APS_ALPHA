// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GalaxyModel.h"
#include "GalaxyClass.h"
#include "GalaxyType.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "GalaxyGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UGalaxyGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	FGalaxyModel GenerateRandomGalaxyModel();

	FGalaxyModel GenerateGalaxyByParamsModel(EGalaxyType GalaxyType, EGalaxyClass GalaxyGlass);

	FVector GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, float StarDistance);

	FVector GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass);

};