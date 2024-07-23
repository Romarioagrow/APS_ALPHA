#pragma once

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "StarSystemGenerationModel.generated.h"

enum class EStarType : uint8;

USTRUCT(BlueprintType)
struct FStarSystemModel :
	public FGenerationModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	int32 AmountOfStars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	EStarType StarSystemType;
};
