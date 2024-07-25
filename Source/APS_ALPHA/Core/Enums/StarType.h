#pragma once

#include "CoreMinimal.h"
#include "StarType.generated.h"

UENUM(BlueprintType)
enum class EStarType : uint8
{
    SingleStar    
    UMETA(DisplayName = "Single Star"),

    DoubleStar    
    UMETA(DisplayName = "Double Star"),

    TripleStar
    UMETA(DisplayName = "Tripple Star"),
    
    MultipleStar
    UMETA(DisplayName = "Multiple Star")
};