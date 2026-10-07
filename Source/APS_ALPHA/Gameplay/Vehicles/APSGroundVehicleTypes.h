#pragma once

#include "CoreMinimal.h"

/**
 * Ground transport (Rio 02.10: "a system of ground transport: a wheeled one, a hover one, and one that flies like a drone
 * or a small helicopter, but only up to the top of the atmosphere, or a little into space"). A vehicle is an ASpaceship of
 * one of these kinds: boarding, exit, camera and HUD are the ship's own, and UAPSShipFlightModel drives it in the local
 * gravity frame instead of the flight bands. Plain C++ on purpose: no reflection and no change to any save.
 */
enum class EAPSGroundVehicleKind : uint8
{
	/** A ship: flies with the bands exactly as before. */
	None,
	/** Four wheels on the ground, about 27 m/s (35 with boost); stays on the terrain, flies off sharp crests. */
	Rover,
	/** Floats 1.75 m over the ground and over liquid, about 55 m/s (70 with boost), drifts through turns. */
	Hover,
	/** VTOL: W/S, A/D strafe, Space/Alt up and down, mouse yaw and pitch, up to a little above the atmosphere. */
	Drone
};

namespace APSGroundVehicle
{
	/** ROVER, HOVER or DRONE: what the HUD, the maps and the boarding prompt show; empty for a ship. */
	inline const TCHAR* KindName(const EAPSGroundVehicleKind Kind)
	{
		switch (Kind)
		{
		case EAPSGroundVehicleKind::Rover: return TEXT("ROVER");
		case EAPSGroundVehicleKind::Hover: return TEXT("HOVER");
		case EAPSGroundVehicleKind::Drone: return TEXT("DRONE");
		case EAPSGroundVehicleKind::None:
		default: return TEXT("");
		}
	}
}
