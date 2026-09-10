#include "APSPlanetHabitability.h"

EPlanetHabitability UAPSPlanetHabitabilityLibrary::ResolveDefaultHabitability(
	const EPlanetType PlanetType,
	const EPlanetaryZoneType PlanetZone,
	const double AtmosphereHeightKm)
{
	if (!FMath::IsFinite(AtmosphereHeightKm) || AtmosphereHeightKm <= 0.0)
	{
		return EPlanetHabitability::Uninhabitable;
	}

	const bool bTemperateOrbit = PlanetZone == EPlanetaryZoneType::HabitableZone
		|| PlanetZone == EPlanetaryZoneType::Unknown;
	const bool bCoolButViableOrbit = bTemperateOrbit
		|| PlanetZone == EPlanetaryZoneType::WarmZone
		|| PlanetZone == EPlanetaryZoneType::ColdZone;

	switch (PlanetType)
	{
	case EPlanetType::Terrestrial:
	case EPlanetType::Forest:
	case EPlanetType::Oasis:
	case EPlanetType::Ocean:
	case EPlanetType::Water:
	case EPlanetType::Archipelago:
	case EPlanetType::Pangea:
		if (bTemperateOrbit)
		{
			return EPlanetHabitability::Habitable;
		}
		return bCoolButViableOrbit
			? EPlanetHabitability::PotentiallyHabitable
			: EPlanetHabitability::Uninhabitable;

	case EPlanetType::Nordic:
	case EPlanetType::Tundra:
	case EPlanetType::HighMountain:
	case EPlanetType::SuperEarth:
	case EPlanetType::Ice:
	case EPlanetType::Exoplanet:
		return bCoolButViableOrbit
			? EPlanetHabitability::PotentiallyHabitable
			: EPlanetHabitability::Uninhabitable;

	default:
		return EPlanetHabitability::Uninhabitable;
	}
}
