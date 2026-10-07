#pragma once

#include "CoreMinimal.h"
#include "APSGroundVehicleTypes.h"

class APlanetaryBody;
class ASpaceship;
class UWorld;

/**
 * The colony's ground transport (Rio 02.10: "spawn some basic ground transport right away on the planet where the colony
 * is"). Once the colony base stands in a game world, a rover, a hover and a drone park side by side in a motor pool
 * beside it (about 40-60 m from its centre, behind it, clear of the walk to the landing pad: the S5 spawn service picks
 * the spot), settle on the terrain once its collision exists there, and step aside when something is later built or
 * parked over them. Nothing is saved: a load finds them missing and parks them again; vehicles that exist are kept.
 * Plain C++ ticked by UAPSFleetCommandSubsystem, on timers: no per-frame actor scans.
 */
namespace APSGroundVehicles
{
	/** Called every frame by UAPSFleetCommandSubsystem; does its work on timers. */
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);

	/** Every vehicle chosen in the generation menu stands at the colony (the arrival curtain waits for them). */
	APS_ALPHA_API bool AreParked(UWorld* World);

	/**
	 * Spawns a ground vehicle of this kind (deferred: ASpaceship::ConfigureAsGroundVehicle sets its meshes, collision and
	 * name before construction) with its nose along Heading and its up along Up, attached to Body. Null on failure.
	 */
	APS_ALPHA_API ASpaceship* SpawnVehicle(UWorld* World, EAPSGroundVehicleKind Kind, const FVector& Location,
		const FVector& Heading, const FVector& Up, APlanetaryBody* Body);
}
