#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/CelestialGenerationModel.h"
#include "GalaxyModel.generated.h"

USTRUCT(BlueprintType)
struct FGalaxyModel : public FCelestialBodyModel
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	EGalaxyType GalaxyType;

	UPROPERTY(VisibleAnywhere)
	EGalaxyClass GalaxyClass;

	UPROPERTY(VisibleAnywhere)
	int StarsCount;

	UPROPERTY(VisibleAnywhere)
	int GalaxySize;

	UPROPERTY(VisibleAnywhere)
	double StarsDensity;
};
