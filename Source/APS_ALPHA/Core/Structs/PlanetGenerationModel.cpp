#include "PlanetGenerationModel.h"

#include "APS_ALPHA/Core/Enums/PlanetaryZoneType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"

FPlanetModel::FPlanetModel()
{
	this->AmountOfMoons = 0;
	this->Temperature = 0;
	this->PlanetZone = EPlanetaryZoneType::Unknown;
	this->PlanetType = EPlanetType::Unknown;

}
