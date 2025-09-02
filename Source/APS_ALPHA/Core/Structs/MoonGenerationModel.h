#pragma once

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "MoonGenerationModel.generated.h"

enum class EMoonType : uint8;

USTRUCT(BlueprintType)
struct FMoonModel :
	public FOrbitalBodyModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	EMoonType Type;

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	double MoonDensity;

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	double MoonGravity;

	double MoonAtmosphereHeight{0.0};

	// Конструктор по умолчанию для инициализации всех свойств
	FMoonModel()
		: Type(EMoonType::Unknown)
		, MoonDensity(0.0)
		, MoonGravity(0.0)
		, MoonAtmosphereHeight(0.0)
	{
	}
};
