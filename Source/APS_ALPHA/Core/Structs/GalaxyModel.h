#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Generation/CelestialGenerationModel.h"
#include "GalaxyModel.generated.h"

USTRUCT(BlueprintType)
struct FGalaxyModel : public FCelestialBodyModel
{
	GENERATED_BODY()

	FGalaxyModel()
			: GalaxyType(EGalaxyType::Elliptical)  
			, GalaxyClass(EGalaxyClass::E0) 
			, StarsCount(0)
			, GalaxySize(0)
			, StarsDensity(0.0)
	{}
	
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
