#pragma once

#include "PlanetType.h"

#include "OrbitalBodyGenerationModel.h"
#include "CoreMinimal.h"
#include "PlanetGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FPlanetGenerationModel :
    public FOrbitalBodyGenerationModel
{
	GENERATED_BODY()	

		// ��� ������� (��������, ������� ������, ������������� ������� � �.�.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		EPlanetType PlanetType;

	// ����� ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		int32 AmountOfMoons;


};

