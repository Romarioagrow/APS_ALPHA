#pragma once

#include "CelestialGenerationModel.h"
#include "CoreMinimal.h"
#include "OrbitalBodyGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FOrbitalBodyGenerationModel :
    public FCelestialGenerationModel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float RotationSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float OrbitDistance;

};

