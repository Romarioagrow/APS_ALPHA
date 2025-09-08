#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/SpectralType.h"
#include "APS_ALPHA/Generation/CelestialGenerationModel.h"
#include "StarGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FStarModel :
	public FCelestialBodyModel
{
	GENERATED_BODY()

	// Конструктор по умолчанию для инициализации всех свойств
	FStarModel()
		: StellarType(EStellarType::MainSequence)
		, SpectralClass(ESpectralClass::Unknown)
		, SpectralType(ESpectralType::Unknown)
		, Luminosity(0.0f)
		, SurfaceTemperature(0)
		, Age(TEXT(""))
		, FullSpectralClass(NAME_None)
		, FullSpectralName(NAME_None)
		, SpectralSubclass(0)
		, StarStellarClass(EStellarType::MainSequence)
		, MinOrbit(0.0)
		, MaxOrbit(0.0)
	{
	}

	UPROPERTY(VisibleAnywhere)
	EStellarType StellarType;

	UPROPERTY(VisibleAnywhere)
	ESpectralClass SpectralClass;

	UPROPERTY(VisibleAnywhere)
	ESpectralType SpectralType;

	UPROPERTY(VisibleAnywhere)
	float Luminosity;

	UPROPERTY(VisibleAnywhere)
	int SurfaceTemperature;

	UPROPERTY(VisibleAnywhere)
	FString Age;

	UPROPERTY(VisibleAnywhere)
	FName FullSpectralClass;

	UPROPERTY(VisibleAnywhere)
	FName FullSpectralName;

	UPROPERTY(VisibleAnywhere)
	int SpectralSubclass;

	UPROPERTY(VisibleAnywhere)
	EStellarType StarStellarClass;

	double MinOrbit;

	double MaxOrbit;
};
