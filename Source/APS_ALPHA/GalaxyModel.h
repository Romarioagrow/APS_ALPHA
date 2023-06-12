#pragma once
#include "GalaxyClass.h"
#include "GalaxyType.h"

#include "CoreMinimal.h"
#include "CelestialGenerationModel.h"
#include "GalaxyModel.generated.h"

USTRUCT(BlueprintType)
struct FGalaxyModel : public FCelestialGenerationModel
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Galaxy")
		EGalaxyType GalaxyType;

	UPROPERTY(VisibleAnywhere, Category = "Galaxy")
		EGalaxyClass GalaxyGlass;

	UPROPERTY(VisibleAnywhere, Category = "Galaxy")
		int StarsCount;
};