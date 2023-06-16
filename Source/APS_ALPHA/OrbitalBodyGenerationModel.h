#pragma once

#include "CelestialGenerationModel.h"
#include "CoreMinimal.h"
#include "OrbitalBodyGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FOrbitalBodyModel :
    public FCelestialBodyModel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        double RotationSpeed{ 0 };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        double OrbitDistance{ 0 };

};

