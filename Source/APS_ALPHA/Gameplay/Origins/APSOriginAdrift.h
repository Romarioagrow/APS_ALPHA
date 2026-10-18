#pragma once

#include "CoreMinimal.h"

class AActor;
class APlanetaryBody;
class AStar;
class UWorld;

/**
 * Rio 18-19.10 (0.6.6 origins: "ARK, SPACE TRIPS, then ADRIFT"; "energy, but not stuffy"; "more depth and detail"):
 * ADRIFT · THE LONG NIGHT. The ship alone at the edge of an unknown system, its cabin dark, its reactor at eleven
 * percent (the colony's energy stock is the reactor in this origin). Every flight burns it, the cruise band most; empty, the
 * ship limps on at a third of the cruise speed (never stranded). Two wrecks of the expedition before ours drift in the
 * outer system (hold beside one and the drones strip its cells), and over the living world hangs that expedition's dead
 * station: hold beside it and it wakes, the reactor fills, the lights come back. Landing makes the ship the home: the
 * colony's site anchor moves to where the pilot first stood.
 * Everything here runs only in an ADRIFT world founded with its kit (IsAdriftWorld).
 */
namespace APSOriginAdrift
{
	APS_ALPHA_API bool IsAdriftWorld(const UWorld* World);
	/** The generator: where the ADRIFT ship starts (beyond the outermost world, its nose toward the sun). */
	APS_ALPHA_API bool StartPlace(const AStar& Star, const APlanetaryBody& Home, FVector& OutLocation, FRotator& OutRotation);
	/** The flight model: the cruise band's speed scale (1; the empty reactor's limp below 1). */
	APS_ALPHA_API double CruiseScale(const AActor* Ship);
	/** "REACTOR 23 %" for the TASKS card; empty outside ADRIFT. */
	APS_ALPHA_API FText ReactorLine(const UWorld* World);
	/** A wreck or the dead station of ADRIFT: never piloted, never in the fleet. */
	APS_ALPHA_API bool IsDeadHull(const AActor* Actor);
	/** Once a frame (the fleet command subsystem): the reactor, the wrecks, the station, the course, the landing. */
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);
}
