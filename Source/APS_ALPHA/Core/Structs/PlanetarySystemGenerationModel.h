#pragma once

#include "PlanetGenerationModel.h"
#include "GenerationModel.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "CoreMinimal.h"
#include "PlanetarySystemGenerationModel.generated.h"

enum class EPlanetarySystemType : uint8;

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
struct FPlanetData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Mode")
	int PlanetOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Mode")
	double OrbitRadius;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Model")
	//FPlanetModel PlanetModel;
	TSharedPtr<FPlanetModel> PlanetModel;

	FPlanetData()
	{
		PlanetOrder = 0;
		OrbitRadius = 0.0;
	}

	FPlanetData(int PlanetOrder, double OrbitRadius, TSharedPtr<FPlanetModel> PlanetModel)
	{
		this->PlanetOrder = PlanetOrder;
		this->OrbitRadius = OrbitRadius;
		this->PlanetModel = PlanetModel;
	}
};

USTRUCT(BlueprintType)
struct FPlanetarySystemModel :
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
	FName FullSpectralName;

	//UPROPERTY(VisibleAnywhere)
	TArray<TSharedPtr<FPlanetData>> PlanetsList;

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

	double HotZoneOuter;
	double WarmZoneOuter;
	double ColdZoneOuter;
	double IceZoneOuter;
	double GasGiantsZoneOuter;
	double KuiperBeltZoneOuter;
	double InnerZoneOuter;
	double OuterZoneOuter;
	double HabitableZoneOuter;
	double StarDeadZoneOuter;
};
