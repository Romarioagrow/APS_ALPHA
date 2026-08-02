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
		, GenerationSeed(0)
		, PotentialPlanetCount(0)
		, bHasPlanetarySystem(false)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	int32 AmountOfStars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	EStarType StarSystemType;

	/** Stable identity shared by the cheap HISM point and its eventual actor hierarchy. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	FGuid StableId;

	/** Deterministic seed used when the lightweight system is materialized. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	int32 GenerationSeed;

	/** Cheap generation summary. Planet actors are deliberately not allocated at cluster range. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	int32 PotentialPlanetCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	bool bHasPlanetarySystem;
};
