// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "PlanetarySystem.h"
#include "StarSystemType.h"

#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "StarSystem.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AStarSystem : public ACelestialSystem
{
	GENERATED_BODY()
	
private:

	UPROPERTY(VisibleAnywhere, Category = "Star System")
		int StarsAmount;

	UPROPERTY(VisibleAnywhere)
		EStarSystemType StarSystemType;

	UPROPERTY()
		TArray<APlanetarySystem*> PlanetarySystems;

public:
	void SetStarsAmount(int StarsAmount);
	void AddPlanetarySystem(APlanetarySystem* PlanetarySystem);
};
