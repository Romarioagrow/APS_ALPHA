#pragma once

#include "CoreMinimal.h"
#include "PlanetarySystemType.generated.h"

UENUM(BlueprintType)
enum class EPlanetarySystemType : uint8
{
    NoPlanetSystem
    UMETA(DisplayName = "No Planet System"),

    SinglePlanetSystem
    UMETA(DisplayName = "Single Planet System"),

    MultiPlanetSystem
    UMETA(DisplayName = "Multi Planet System"),

    HabitableZoneSystem
    UMETA(DisplayName = "Habitable Zone System"),

    GasGiantDominatedSystem
    UMETA(DisplayName = "Gas Giant Dominated System"),

    Unknown
    UMETA(DisplayName = "Unknown"),
};