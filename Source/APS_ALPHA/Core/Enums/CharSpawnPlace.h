#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ECharSpawnPlace : uint8
{
	PlanetOrbit			UMETA(DisplayName = "Planet Orbit"),
	PlanetSurface		UMETA(DisplayName = "Planet Surface"),
	MoonOrbit			UMETA(DisplayName = "Moon Orbit"),
	MoonSurface			UMETA(DisplayName = "Moon Surface"),
	SpaceShip			UMETA(DisplayName = "Space Ship"),
};