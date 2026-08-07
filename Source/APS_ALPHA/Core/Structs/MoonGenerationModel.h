#pragma once

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "MoonGenerationModel.generated.h"

enum class EMoonType : uint8;

USTRUCT(BlueprintType)
struct FMoonModel :
	public FOrbitalBodyModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	EMoonType Type;

	/** Surface resolver input retained independently from the astronomical moon class. */
	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	EPlanetType PlanetType;

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	double MoonDensity;

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	double MoonGravity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Atmosphere")
	double MoonAtmosphereHeight{0.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Surface")
	int32 SurfaceSeed{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Surface")
	double SurfaceFeatureScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Surface")
	double SurfaceReliefScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Surface")
	double SurfaceLandCoverageScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Surface")
	double SurfaceMountainScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Surface")
	double SurfaceCraterScale{1.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon Surface")
	double SurfaceRoughnessScale{1.0};

	// Конструктор по умолчанию для инициализации всех свойств
	FMoonModel()
		: Type(EMoonType::Unknown)
		, PlanetType(EPlanetType::Unknown)
		, MoonDensity(0.0)
		, MoonGravity(0.0)
		, MoonAtmosphereHeight(0.0)
	{
	}
};
