#pragma once

#include "PlanetAtmosphereType.generated.h"

UENUM(BlueprintType)
enum class EPlanetAtmosphereType : uint8
{
	Normal,
	Thin,
	Dense
};
