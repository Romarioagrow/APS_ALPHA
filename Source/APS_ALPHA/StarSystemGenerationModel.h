#pragma once
#include "StarSystemType.h"

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "StarSystemGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FStarSystemGenerationModel :
    public FGenerationModel
{
    GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
		int32 AmountOfStars;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
		EStarSystemType StarSystemType;
};

