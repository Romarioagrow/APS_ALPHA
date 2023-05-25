// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Star.h"
#include "PlanetarySystemType.h"
#include "OrbitDistributionType.h"
#include "PlanetarySystemGenerationModel.h"
#include "StarSpectralClass.h" 

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
	UPROPERTY(VisibleAnywhere)
		AStar* Star;

	UPROPERTY(VisibleAnywhere)
		int AmountOfPlanets;

	UPROPERTY(VisibleAnywhere)
		EPlanetarySystemType PlanetarySystemType;

	UPROPERTY(VisibleAnywhere)
		EOrbitDistributionType OrbitDistributionType;

	UPROPERTY(VisibleAnywhere)
		FName FullSpectralName;

	UPROPERTY(VisibleAnywhere)
		TArray<FPlanetData> PlanetsList;

	// ZONES 
	/// TODO: TO SINGLE STRUCT
	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius HabitableZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius ColdZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius IceZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius WarmZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius HotZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius InnerPlanetZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius OuterPlanetZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		TMap<int32, FZoneRadius> AsteroidBeltZoneRadius;
	
	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius KuiperBeltZoneRadius;

public:
	void SetStar(AStar* Star);

	void SetAmountOfPlanets(int AmountOfPlanets);

	void SetPlanetarySystemType(EPlanetarySystemType PlanetarySystemType);

	void SetOrbitDistributionType(EOrbitDistributionType OrbitDistributionType);

	void SetStarFullSpectralName(FName NewStarFullSpectralName);

	void SetPlanetsList(TArray<FPlanetData> NewPlanetsList);
};
