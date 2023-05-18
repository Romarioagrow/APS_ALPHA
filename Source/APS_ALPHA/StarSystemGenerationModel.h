#pragma once

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "StarSystemGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FStarSystemGenerationModel :
    public FGenerationModel
{
    GENERATED_BODY()

	// Количество звезд в системе
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
		int32 AmountOfStars;

	// Расстояние между звездами
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
		double DistanceBetweenStars;

	//int32 AmountOfPlanets;


};

