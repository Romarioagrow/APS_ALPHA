// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Star.h"
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

	UPROPERTY(VisibleAnywhere, Category = "Star System")
		EStarSystemType StarSystemType;

	UPROPERTY()
		TArray<AStar*> Stars;

public:
	AStarSystem();

	void SetStarsAmount(int StarsAmount);

	void AddNewStar(AStar* MewStar);

	void SetStarSystemType(EStarSystemType Type);

	UPROPERTY(VisibleAnywhere, Category = "Star System")
		double StarSystemRadius;
	
	UPROPERTY(VisibleAnywhere, Category = "Star System")
		FVector StarSystemZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Star")
		USphereComponent* StarSystemZone;
};
