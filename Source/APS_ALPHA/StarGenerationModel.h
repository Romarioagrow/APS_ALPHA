#pragma once

#include "CelestialGenerationModel.h"
#include "CoreMinimal.h"
#include "StarGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FStarGenerationModel :
    public FCelestialGenerationModel
{
    GENERATED_BODY()
    
    
        // ��� ������ (��������, ������, ������, ����������� � �.�.)
    /*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star")
        EStarType StarType;*/
};

