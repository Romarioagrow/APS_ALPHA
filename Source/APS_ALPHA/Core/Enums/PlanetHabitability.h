#pragma once

#include "CoreMinimal.h"
#include "PlanetHabitability.generated.h"

/**
 * Gameplay-facing classification kept separate from a body's visual surface type.
 * A frozen or oceanic body can therefore retain its authored appearance while
 * gameplay systems independently decide whether it can support habitation.
 */
UENUM(BlueprintType)
enum class EPlanetHabitability : uint8
{
	Uninhabitable		UMETA(DisplayName = "Uninhabitable"),
	PotentiallyHabitable	UMETA(DisplayName = "Potentially Habitable"),
	Habitable			UMETA(DisplayName = "Habitable")
};
