// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Planet.h"
#include "StarType.h"
#include "StarSpectralClass.h"

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Star.generated.h"


/**
 * 
 */
UCLASS()
class APS_ALPHA_API AStar : public ACelestialBody
{
	GENERATED_BODY()

public:
	AStar();

private:
	UPROPERTY(VisibleAnywhere)
		float Luminosity;

	UPROPERTY(VisibleAnywhere)
		int SurfaceTemperature;

	UPROPERTY(VisibleAnywhere)
		EStarType StarType;
	
	UPROPERTY(VisibleAnywhere)
		EStarSpectralClass StarSpectralClass;

private:
	UPROPERTY()
		TArray<APlanet*> Planets;

	public:
		void AddPlanet(APlanet* Planet);
	
};
