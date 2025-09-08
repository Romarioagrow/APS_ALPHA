#pragma once

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "StarSystemGenerationModel.generated.h"

enum class EStarType : uint8;

USTRUCT(BlueprintType)
struct FStarSystemModel :
	public FGenerationModel
{
	GENERATED_BODY()

	// Конструктор по умолчанию для инициализации всех свойств
	FStarSystemModel()
		: AmountOfStars(0)
		, StarSystemType(EStarType::SingleStar)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	int32 AmountOfStars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	EStarType StarSystemType;
};
