#pragma once

#include "PlanetType.h"

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "PlanetGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FPlanetGenerationModel :
    public FOrbitalBodyGenerationModel
{
	GENERATED_BODY()	

		// Тип планеты (например, газовый гигант, землеподобная планета и т.д.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		EPlanetType PlanetType;

	// Число спутников
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		int32 AmountOfMoons;


};

