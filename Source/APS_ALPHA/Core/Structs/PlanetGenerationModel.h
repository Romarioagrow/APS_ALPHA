#pragma once

#include "MoonGenerationModel.h"
#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Enums/PlanetaryZoneType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "PlanetGenerationModel.generated.h"

enum class EPlanetaryZoneType : uint8;
enum class EPlanetType : uint8;
enum class EOrbitHeight : uint8;

USTRUCT(BlueprintType)
struct FOrbitInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	EOrbitHeight OrbitHeightType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	double OrbitHeight;

	// Конструктор по умолчанию для инициализации всех свойств
	FOrbitInfo()
		: OrbitHeightType(EOrbitHeight::LowOrbit)
		, OrbitHeight(0.0)
	{
	}
};

USTRUCT(BlueprintType)
struct FMoonData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Mode")
	int MoonOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Mode")
	double OrbitRadius;

	/** Serializable counterpart of the runtime shared pointer below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Mode")
	FMoonModel MoonModelData;

	TSharedPtr<FMoonModel> MoonModel;

	FMoonData()
	{
		MoonOrder = 0;
		OrbitRadius = 0.0;
	}

	FMoonData(int MoonOrder, double OrbitRadius, TSharedPtr<FMoonModel> MoonModel)
	{
		this->MoonOrder = MoonOrder;
		this->OrbitRadius = OrbitRadius;
		this->MoonModel = MoonModel;
		if (MoonModel.IsValid())
		{
			MoonModelData = *MoonModel;
		}
	}
};

USTRUCT(BlueprintType)
struct FPlanetModel : public FOrbitalBodyModel
{
	GENERATED_BODY()

	FPlanetModel();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	TArray<FOrbitInfo> Orbits;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	EPlanetType PlanetType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	int32 AmountOfMoons{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	int32 Temperature{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	EPlanetaryZoneType PlanetZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	double PlanetDensity{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	double PlanetGravityStrength{0};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	TArray<FMoonData> MoonsListData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	double AtmosphereHeight{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	int32 SurfaceSeed{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	double SurfaceFeatureScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	double SurfaceReliefScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	double SurfaceLandCoverageScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	double SurfaceMountainScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	double SurfaceCraterScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	double SurfaceRoughnessScale{1.0};

	TArray<TSharedPtr<FMoonData>> MoonsList;

	TPair<double, double> MoonOrbitsRange;
	
	TArray<FMoonData> GetMoonsData() const
	{
		TArray<FMoonData> Result;
		for (const TSharedPtr<FMoonData>& MoonPtr : MoonsList)
		{
			if (MoonPtr.IsValid())
			{
				FMoonData Data = *MoonPtr;
				if (Data.MoonModel.IsValid())
				{
					Data.MoonModelData = *Data.MoonModel;
				}
				Result.Add(MoveTemp(Data));
			}
		}
		return Result;
	}
};
