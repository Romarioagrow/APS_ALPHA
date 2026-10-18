#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Rio 19.10 (0.6.6, SPACE TRIPS without the console; "more depth and detail in everything"): the trip's own keys and its
 * route card. O opens the card of routes (Up / Down choose, Enter sets out, O or Escape closes), P takes a still of the view
 * without the instruments. The keys live on an input component of the player controller's own (no project input settings
 * change) and only in a SPACE TRIPS world.
 */
namespace APSSpaceTripsHud
{
	/** Once a frame from APSSpaceTrips::Tick in a trip world: the keys bound once, the card kept current. */
	APS_ALPHA_API void Tick(UWorld* World);
	APS_ALPHA_API bool IsCardOpen(const UWorld* World);
	/** Opens the route card (the trip's start does it once the pilot sits). */
	APS_ALPHA_API void OpenCard(UWorld* World);
	/** Closes it (a route started, by the card or the console). */
	APS_ALPHA_API void CloseCard(UWorld* World);
}
