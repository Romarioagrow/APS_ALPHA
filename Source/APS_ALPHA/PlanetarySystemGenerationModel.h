#pragma once
#include "PlanetarySystemType.h"
#include "OrbitDistributionType.h" 

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "PlanetarySystemGenerationModel.generated.h"


USTRUCT(BlueprintType)
struct FZoneRadius
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
		double InnerRadius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
		double OuterRadius;

	FZoneRadius()
	{
		InnerRadius = 0.0;
		OuterRadius = 0.0;
	}

	FZoneRadius(double InnerRadius, double OuterRadius)
	{
		this->InnerRadius = InnerRadius;
		this->OuterRadius = OuterRadius;
	}
};

USTRUCT(BlueprintType)
struct FPlanetarySystemGenerationModel :
    public FGenerationModel
{
    GENERATED_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planetary System")
        int AmountOfPlanets;

        // Расстояние между планетами
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planetary System")
        double DistanceBetweenPlanets;

    UPROPERTY(VisibleAnywhere)
        EPlanetarySystemType PlanetarySystemType;
    
    UPROPERTY(VisibleAnywhere)
        EOrbitDistributionType OrbitDistributionType;

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
	
	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius GasGiantsZoneRadius;

	//UPROPERTY(VisibleAnywhere, Category = "Zones")
	//	FZoneRadius HabitableZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	FZoneRadius ColdZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	FZoneRadius IceZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	FZoneRadius WarmZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	FZoneRadius HotZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	FZoneRadius InnerPlanetZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	FZoneRadius OuterPlanetZoneRadius;

	////UPROPERTY(VisibleAnywhere)
	//	//TMap<int32, FZoneRadius> AsteroidBeltZoneRadius;

	//UPROPERTY(VisibleAnywhere)
	//	FZoneRadius KuiperBeltZoneRadius;

 //   //UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> HabitableZoneRadius;


	//// Habitable Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> HabitableZoneRadius;

	//// Cold Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> ColdZoneRadius;

	//// Ice Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> IceZoneRadius;

	//// Warm Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> WarmZoneRadius;

	//// Hot Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> HotZoneRadius;

	//// Gas Giants Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> GasGiantsZoneRadius;

	//// Asteroid Belt Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> AsteroidBeltZoneRadius;

	//// Kuiper Belt Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> KuiperBeltZoneRadius;

	//// Inner Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> InnerZoneRadius;

	//// Outer Zone
	////UPROPERTY(VisibleAnywhere)
	//	TPair<double, double> OuterZoneRadius;

};

