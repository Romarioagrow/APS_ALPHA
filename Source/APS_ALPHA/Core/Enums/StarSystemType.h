#pragma once

#include "CoreMinimal.h"
#include "StarSystemType.generated.h"

UENUM(BlueprintType)
enum class EStarSystemType : uint8
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