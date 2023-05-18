// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Star.h"

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
		//TArray<APlanet*> Planets;

public:
	void SetStar(AStar* Star);
};
