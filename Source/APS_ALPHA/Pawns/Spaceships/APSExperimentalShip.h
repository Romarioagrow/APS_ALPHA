#pragma once

#include "CoreMinimal.h"
#include "Spaceship.h"
#include "APSShipFlightModel.h"
#include "APSExperimentalShip.generated.h"

/**
 * The band flight model's test ship (Rio, 2026-09-28): an ordinary ASpaceship whose FlightModel keeps the bands
 * whatever aps.Ship.FlightModel says. Since 2026-09-29 every ship flies with the same model by default.
 *
 * Console: aps.ExpShip.Spawn [StaticMeshPath] puts the pilot into one beside the current ship with the same hull;
 * aps.ExpShip.Drive <Band 1-5> <Forward -1..1> [Boost] | off drives any band-model ship without a keyboard.
 */
UCLASS(Blueprintable)
class APS_ALPHA_API AAPSExperimentalShip : public ASpaceship
{
	GENERATED_BODY()

public:
	AAPSExperimentalShip();
};
