#pragma once

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "MoonGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FMoonGenerationModel :
    public FOrbitalBodyGenerationModel
{
    GENERATED_BODY()
};

