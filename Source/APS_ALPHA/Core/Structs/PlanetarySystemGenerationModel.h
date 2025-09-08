#pragma once

#include "PlanetGenerationModel.h"
#include "GenerationModel.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Planetary/PlanetAtmosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetBiosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetGeosphere.h"
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
		: InnerRadius(0.0)
		, OuterRadius(0.0)
	{
	}

	FZoneRadius(double InnerRadius, double OuterRadius)
		: InnerRadius(InnerRadius)
		, OuterRadius(OuterRadius)
	{
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

	// ������ �������
	TSharedPtr<FPlanetModel> PlanetModel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Mode")
	FPlanetModel PlanetModelData;

	// ��������� ���������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	FPlanetAtmosphere PlanetAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	FPlanetBiosphere PlanetBiosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	FPlanetGeosphere PlanetGeosphere;

	// �������������� ��������� �������
	UPROPERTY(EditAnywhere, Category = "Planet")
	int PlanetRadiusKM;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	int32 Temperature{0};

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	double PlanetDensity{0};

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	double PlanetGravityStrength{0};

	// ������������
	// Конструктор по умолчанию
	FPlanetData()
		: PlanetOrder(0)
		, OrbitRadius(0.0)
		, PlanetRadiusKM(0)
		, Temperature(0)
		, PlanetDensity(0.0)
		, PlanetGravityStrength(0.0)
	{
	}

	FPlanetData(int InPlanetOrder, double InOrbitRadius, TSharedPtr<FPlanetModel> InPlanetModel)
		: PlanetOrder(InPlanetOrder)
		, OrbitRadius(InOrbitRadius)
		, PlanetModel(InPlanetModel)
		, PlanetRadiusKM(0)
		, Temperature(0)
		, PlanetDensity(0.0)
		, PlanetGravityStrength(0.0)
	{
	}

	//   
	void InitializePlanetData(int InPlanetRadiusKM, int32 InTemperature, double InPlanetDensity, double InPlanetGravityStrength)
	{
		this->PlanetRadiusKM = InPlanetRadiusKM;
		this->Temperature = InTemperature;
		this->PlanetDensity = InPlanetDensity;
		this->PlanetGravityStrength = InPlanetGravityStrength;
	}
};

USTRUCT(BlueprintType)
struct FPlanetarySystemModel :
	public FGenerationModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planetary System")
	int AmountOfPlanets;

	//   
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planetary System")
	double DistanceBetweenPlanets;

	UPROPERTY(VisibleAnywhere)
	EPlanetarySystemType PlanetarySystemType;

	UPROPERTY(VisibleAnywhere)
	EOrbitDistributionType OrbitDistributionType;

	UPROPERTY(VisibleAnywhere)
	FName FullSpectralName;

	TArray<TSharedPtr<FPlanetData>> PlanetsList;

	// TODO: To one struct
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

	// Конструктор по умолчанию для инициализации всех свойств
	FPlanetarySystemModel()
		: AmountOfPlanets(0)
		, DistanceBetweenPlanets(0.0)
		, PlanetarySystemType(EPlanetarySystemType::Unknown)
		, OrbitDistributionType(EOrbitDistributionType::Uniform)
		, FullSpectralName(NAME_None)
		, HotZoneOuter(0.0)
		, WarmZoneOuter(0.0)
		, ColdZoneOuter(0.0)
		, IceZoneOuter(0.0)
		, GasGiantsZoneOuter(0.0)
		, KuiperBeltZoneOuter(0.0)
		, InnerZoneOuter(0.0)
		, OuterZoneOuter(0.0)
		, HabitableZoneOuter(0.0)
		, StarDeadZoneOuter(0.0)
	{
	}
};
