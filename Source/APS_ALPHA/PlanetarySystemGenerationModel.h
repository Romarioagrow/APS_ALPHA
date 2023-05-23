#pragma once
#include "PlanetarySystemType.h"
#include "OrbitDistributionType.h" 
#include "StarSpectralClass.h" 
#include "PlanetGenerationModel.h"

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
struct FPlanetData {

	GENERATED_USTRUCT_BODY()

		int PlanetOrder;

		double OrbitRadius;

		FPlanetGenerationModel PlanetModel;
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

	UPROPERTY(VisibleAnywhere)
		ESpectralClass StarSpectralClass;
	
	UPROPERTY(VisibleAnywhere)
		FPlanetData PlanetsList;





	/// TODO: To one struct

	UPROPERTY(VisibleAnywhere, Category = "Zones")
		FZoneRadius DeadZoneRadius;
	
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

};

