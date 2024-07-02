#pragma once

#include "StarSystemGenerationModel.h"
#include "CoreMinimal.h"
#include "HomeStarSystemGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FHomeStarSystemGenerationModel :
    public FStarSystemModel
{
    GENERATED_BODY()

        double StartLocation;
};

