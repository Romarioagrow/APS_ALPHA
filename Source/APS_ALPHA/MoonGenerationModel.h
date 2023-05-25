#pragma once
#include "MoonType.h"

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "MoonGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FMoonGenerationModel :
    public FOrbitalBodyGenerationModel
{
    GENERATED_BODY()

    FMoonGenerationModel();

    UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
    EMoonType Type;

    UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
        double MoonDensity;
    
    UPROPERTY(EditAnywhere, Category = "Moon Generation Model")
        double MoonGravity;
};

