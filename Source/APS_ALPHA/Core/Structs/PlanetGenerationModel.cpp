#include "PlanetGenerationModel.h"

#include "APS_ALPHA/Core/Enums/PlanetaryZoneType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"

FPlanetModel::FPlanetModel()
	: PlanetType(EPlanetType::Rocky)
	, AmountOfMoons(0)
	, Temperature(0)
	, PlanetZone(EPlanetaryZoneType::Unknown)
	, PlanetDensity(0.0)
	, PlanetGravityStrength(0.0)
	, AtmosphereHeight(0.0)
{
}
