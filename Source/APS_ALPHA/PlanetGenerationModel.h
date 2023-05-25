#pragma once

#include "PlanetType.h"
#include "PlanetaryZoneType.h"
#include "MoonGenerationModel.h"

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "PlanetGenerationModel.generated.h"



USTRUCT(BlueprintType)
struct FMoonData
{

	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Mode")
		int MoonOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Mode")
		double OrbitRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Model")
		FMoonGenerationModel MoonModel;

	FMoonData()
	{
		MoonOrder = 0;
		OrbitRadius = 0.0;
	}

	FMoonData(int MoonOrder, double OrbitRadius, FMoonGenerationModel MoonModel)
	{
		this->MoonOrder = MoonOrder;
		this->OrbitRadius = OrbitRadius;
		this->MoonModel = MoonModel;
	}
};

USTRUCT(BlueprintType)
struct FPlanetGenerationModel :
    public FOrbitalBodyGenerationModel
{
	GENERATED_BODY()

		FPlanetGenerationModel();

		// Тип планеты 
		UPROPERTY(VisibleAnywhere, Category = "Planet")
			EPlanetType PlanetType;

		// Число спутников
		UPROPERTY(VisibleAnywhere, Category = "Planet")
			int32 AmountOfMoons { 0 };
	
		// Число спутников
		UPROPERTY(VisibleAnywhere, Category = "Planet")
			int32 Temperature { 0 };
	
		UPROPERTY(VisibleAnywhere, Category = "Planet")
			EPlanetaryZoneType PlanetZone;
		
		UPROPERTY(VisibleAnywhere, Category = "Planet")
			double PlanetDensity { 0 };
		
		UPROPERTY(VisibleAnywhere, Category = "Planet")
			double PlanetGravityStrength { 0 };

		UPROPERTY(VisibleAnywhere)
			TArray<FMoonData> MoonsList;
		
			TPair<double, double> MoonOrbitsRange;

};

