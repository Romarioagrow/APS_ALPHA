#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
#include "APS_ALPHA/Core/Enums/PlanetaryZoneType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APSPlanetHabitability.generated.h"

/** Shared generation/gameplay policy for assigning a default habitability class. */
UCLASS()
class APS_ALPHA_API UAPSPlanetHabitabilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "APS|Planetary|Habitability")
	static EPlanetHabitability ResolveDefaultHabitability(
		EPlanetType PlanetType,
		EPlanetaryZoneType PlanetZone,
		double AtmosphereHeightKm);

	UFUNCTION(BlueprintPure, Category = "APS|Planetary|Habitability")
	static bool IsHabitable(EPlanetHabitability Habitability)
	{
		return Habitability == EPlanetHabitability::Habitable;
	}
};
