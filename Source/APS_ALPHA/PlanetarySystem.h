// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Star.h"
#include "PlanetarySystemType.h"

#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "PlanetarySystem.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API APlanetarySystem : public ACelestialSystem
{
	GENERATED_BODY()

private:
	UPROPERTY()
		AStar* Star;

	UPROPERTY(VisibleAnywhere)
		int AmountOfPlanets;

	UPROPERTY(VisibleAnywhere)
		EPlanetarySystemType PlanetarySystemType;

public:
	void SetStar(AStar* Star);

	void SetAmountOfPlanets(int AmountOfPlanets);

	void SetPlanetarySystemType(EPlanetarySystemType PlanetarySystemType);
};
