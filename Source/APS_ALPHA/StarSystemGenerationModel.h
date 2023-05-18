#pragma once

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "StarSystemGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FStarSystemGenerationModel :
    public FGenerationModel
{
    GENERATED_BODY()

	// ���������� ����� � �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
		int32 AmountOfStars;

	// ���������� ����� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
		double DistanceBetweenStars;

	//int32 AmountOfPlanets;


};

