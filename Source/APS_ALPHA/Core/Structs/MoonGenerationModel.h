#pragma once

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "MoonGenerationModel.generated.h"

enum class EMoonType : uint8;

USTRUCT(BlueprintType)
struct FMoonModel :
	public FOrbitalBodyModel
{
	GENERATED_BODY()

	//FMoonModel();

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	EMoonType Type;

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	double MoonDensity;

	UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
	double MoonGravity;

	double MoonAtmosphereHeight{0.0};
};
