#pragma once
#include "CoreMinimal.h"
#include "GenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FGenerationModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FVector Location{FVector::ZeroVector};
};
