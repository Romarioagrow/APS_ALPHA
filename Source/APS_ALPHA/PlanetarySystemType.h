#pragma once

#include "CoreMinimal.h"
#include "PlanetarySystemType.generated.h"

UENUM(BlueprintType)
enum class EPlanetarySystemType : uint8
{
    NoPlanetSystem
    UMETA(DisplayName = "No Planet System"),
    
    SmallSystem
    UMETA(DisplayName = "Small System"),
    
    LargeSystem
    UMETA(DisplayName = "Large System"),
    
    ChaoticSystem
    UMETA(DisplayName = "Chaotic System"),
    
    DenseSystem
    UMETA(DisplayName = "Dense System"),
    
    Unknown
    UMETA(DisplayName = "Unknown"),
};