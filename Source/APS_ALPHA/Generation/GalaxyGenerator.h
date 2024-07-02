// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
/*#include "GalaxyModel.h"
#include "GalaxyClass.h"
#include "GalaxyType.h"
#include "Galaxy.h"*/
#include "StarGenerator.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "GalaxyGenerator.generated.h"

class AGalaxy;
enum class EGalaxyClass : uint8;
enum class EGalaxyType : uint8;
struct FGalaxyModel;
/**
 * 
 */
UCLASS()
class APS_ALPHA_API UGalaxyGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	void GenerateRandomGalaxyModel(TSharedPtr<FGalaxyModel> GalaxyModel);

	FGalaxyModel GenerateGalaxyByParamsModel(EGalaxyType GalaxyType, EGalaxyClass GalaxyGlass);

	void GenerateGalaxyOctreeStars(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy, TSharedPtr<FGalaxyModel> GalaxyModel);

	FVector GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius);

	FVector GenerateStarInLenticularGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius);

	FVector GenerateStarInSpiralGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius);

	FVector GenerateStarInBarredSpiralGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius);

	FVector GenerateStarInIrregularGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius);

	FVector GenerateStarInPeculiarGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius);

	FVector GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, float StarDistance);

	FVector GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass);

	//void GenerateStarsInGalaxy(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy);

	//void GenerateStarsInGalaxy(AGalaxy* NewGalaxy);

};