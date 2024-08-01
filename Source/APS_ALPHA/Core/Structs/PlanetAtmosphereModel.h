#pragma once

#include "APS_ALPHA/Core/Enums/PlanetAtmosphereType.h"
#include "PlanetAtmosphereModel.generated.h"

USTRUCT(BlueprintType)
struct FPlanetAtmosphereModel
{
	GENERATED_BODY()

	FPlanetAtmosphereModel()
		: PlanetAtmosphereType(EPlanetAtmosphereType::Normal),
		  AtmosphereRadiusKm(6378.0),
		  AtmosphereHeight(100.0),
		  AtmosphereOpacity(1.0),
		  AtmosphereMultiScattering(1.0),
		  AtmosphereRayleighScattering(8.0),
		  AtmosphereColor(FLinearColor(3.8f, 13.5f, 33.0f, 0.0f)) 
	{
	}

	UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
	EPlanetAtmosphereType PlanetAtmosphereType;

	UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
	double AtmosphereRadiusKm;

	UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
	double AtmosphereHeight;

	UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
	double AtmosphereOpacity;

	UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
	double AtmosphereMultiScattering;

	UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
	double AtmosphereRayleighScattering;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FLinearColor AtmosphereColor;
};
