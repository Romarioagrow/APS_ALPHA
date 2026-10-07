#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * The civilization's progress that the actor archive does not hold, as one versioned blob in UGameSave
 * (CivilizationState): colony modules by site (relative to the base or the headquarters), the fleet (each unit's
 * division, order and place, the surveys, the outposts it built) and the journal. Rio, 01.10: "stabilize the progress"
 * — before this a load lost every module, order, survey and outpost.
 *
 * Restoring hands each part to its system: the journal at once, the fleet when its units are back, each module when
 * its site is ready (the colony materializes after the load). Places are kept relative to anchors, so a moved world
 * origin between sessions does not move them.
 */
namespace APSCivilizationSave
{
	/** FallbackPilotedVehicleKey (Rio 06.10, audit: saves): the ship the player last piloted, for a save made after
	 * UnPossess (the lifecycle autosave); used only when the player is not seated now and aps.Save.RestorePilotedShip is 1. */
	APS_ALPHA_API void Capture(UWorld* World, TArray<uint8>& OutBytes,
		const FString& FallbackPilotedVehicleKey = FString());
	APS_ALPHA_API void Restore(UWorld* World, const TArray<uint8>& Bytes);
	/** aps.Save.RestorePilotedShip: a ship piloted at save time comes back with its pilot, at the saved seat. */
	APS_ALPHA_API bool RestorePilotedShipEnabled();
}
