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
		ESpectralClass StarSpectralClass;

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

	//// habitat zone
	//UPROPERTY(VisibleAnywhere)
	//	double InnerHabitableZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	double OuterHabitableZoneRadius;

	// habitable zone
	//UPROPERTY()
	//	TMap<double, double> HabitableZoneRadius;


	//// Habitable Zone
	////UPROPERTY()
	//	TPair<double, double> HabitableZoneRadius1;

	//// Cold Zone
	////UPROPERTY()
	//	TPair<double, double> ColdZoneRadius;

	//// Ice Zone
	////UPROPERTY()
	//	TPair<double, double> IceZoneRadius;

	//// Warm Zone
	////UPROPERTY()
	//	TPair<double, double> WarmZoneRadius;

	//// Hot Zone
	////UPROPERTY()
	//	TPair<double, double> HotZoneRadius;

	//// Gas Giants Zone
	////UPROPERTY()
	//	TPair<double, double> GasGiantsZoneRadius;

	//// Asteroid Belt Zone
	////UPROPERTY()
	//	TPair<double, double> AsteroidBeltZoneRadius;

	//// Kuiper Belt Zone
	////UPROPERTY()
	//	TPair<double, double> KuiperBeltZoneRadius;

	//// Inner Zone
	////UPROPERTY()
	//	TPair<double, double> InnerZoneRadius;

	//// Outer Zone
	////UPROPERTY()
	//	TPair<double, double> OuterZoneRadius;



	//// snow line
	//UPROPERTY(VisibleAnywhere)
	//	double SnowLineRadius;

	//// frost line
	//UPROPERTY(VisibleAnywhere)
	//	double FrostLineRadius;

	//// asteroid belt
	//UPROPERTY(VisibleAnywhere)
	//	double InnerAsteroidBeltRadius;

	//UPROPERTY(VisibleAnywhere)
	//	double OuterAsteroidBeltRadius;

	//// kuiper belt
	//UPROPERTY(VisibleAnywhere)
	//	double InnerKuiperBeltRadius;

	//UPROPERTY(VisibleAnywhere)
	//	double OuterKuiperBeltRadius;

	//// oort cloud
	//UPROPERTY(VisibleAnywhere)
	//	double InnerOortCloudRadius;

	//UPROPERTY(VisibleAnywhere)
	//	double OuterOortCloudRadius;

	//// inner planets
	//UPROPERTY(VisibleAnywhere)
	//	double InnerPlanetRadius;

	//UPROPERTY(VisibleAnywhere)
	//	double OuterPlanetRadius;

	//// outer planets
	//UPROPERTY(VisibleAnywhere)
	//	double InnerGiantPlanetRadius;

	//UPROPERTY(VisibleAnywhere)
	//	double OuterGiantPlanetRadius;



public:
	void SetStar(AStar* Star);

	void SetAmountOfPlanets(int AmountOfPlanets);

	void SetPlanetarySystemType(EPlanetarySystemType PlanetarySystemType);

	void SetOrbitDistributionType(EOrbitDistributionType OrbitDistributionType);

	void SetStarSpectralClass(ESpectralClass StarSpectralClass);
};
