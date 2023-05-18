#pragma once

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "PlanetarySystemGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FPlanetarySystemGenerationModel :
    public FGenerationModel
{
    GENERATED_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planetary System")
        int AmountOfPlanets;

        // Расстояние между планетами
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planetary System")
        double DistanceBetweenPlanets;

    FPlanetarySystemGenerationModel GenerateRandomPlanetraySystemModel();
};

