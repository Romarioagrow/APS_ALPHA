#pragma once
#include "PlanetarySystemType.h"

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

        // ���������� ����� ���������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planetary System")
        double DistanceBetweenPlanets;

    UPROPERTY(VisibleAnywhere)
        EPlanetarySystemType PlanetarySystemType;

};

