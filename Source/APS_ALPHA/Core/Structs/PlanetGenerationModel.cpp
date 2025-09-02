#include "PlanetGenerationModel.h"

#include "APS_ALPHA/Core/Enums/PlanetaryZoneType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"

FPlanetModel::FPlanetModel()
	: AmountOfMoons(0)
	, Temperature(0)
	, PlanetType(EPlanetType::Rocky)
	, PlanetZone(EPlanetaryZoneType::Unknown)
	, PlanetDensity(0.0)
	, PlanetGravityStrength(0.0)
	, AtmosphereHeight(0.0)
{
}
