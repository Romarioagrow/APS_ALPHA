#pragma once

#include "CoreMinimal.h"
#include "PlanetarySystemType.generated.h"

UENUM(BlueprintType)
enum class EPlanetarySystemType : uint8
{
    MultiPlanetSystem
    UMETA(DisplayName = "Multi Planet System"),

    SinglePlanetSystem
    UMETA(DisplayName = "Single Planet System"),

    HabitableZoneSystem
    UMETA(DisplayName = "Habitable Zone System"),

    GasGiantDominatedSystem
    UMETA(DisplayName = "Gas Giant Dominated System"),

    NoPlanetSystem
    UMETA(DisplayName = "No Planet System"),

    Unknown
    UMETA(DisplayName = "Unknown"),
};