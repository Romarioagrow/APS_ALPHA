#pragma once

#include "GenerationModel.h"
#include "CoreMinimal.h"
#include "CelestialGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FCelestialGenerationModel :
    public FGenerationModel
{
	GENERATED_BODY()

	// ����� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CelestialGeneration")
		float Mass;

	// ������ ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CelestialGeneration")
		float Radius;


};

