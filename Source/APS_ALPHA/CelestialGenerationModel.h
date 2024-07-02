#pragma once

#include "GenerationModel.h"
#include "CoreMinimal.h"
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

