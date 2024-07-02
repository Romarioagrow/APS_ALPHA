#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Structs/GenerationModel.h"
#include "CelestialGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FCelestialBodyModel :
    public FGenerationModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CelestialGeneration")
		float Mass {0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CelestialGeneration")
		float Radius{ 0 };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CelestialGeneration")
		float RadiusKM{ 0 };

};

