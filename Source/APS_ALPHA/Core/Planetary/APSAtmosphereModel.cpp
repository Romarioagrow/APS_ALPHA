#include "APSAtmosphereModel.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"

#define LOCTEXT_NAMESPACE "APSAtmosphereModel"

namespace APSAtmosphereModelPrivate
{
	/** Relative to an Earth-like sky: airless rocks and metals, thin cold and dry worlds, thick hot ones. */
	float PlanetDensity(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::Dwarf:
		case EPlanetType::Metal:
			return 0.0f;
		case EPlanetType::Metallic:
		case EPlanetType::Rocky:
			return 0.1f;
		case EPlanetType::Crystal:
		case EPlanetType::Basalt:
			return 0.2f;
		case EPlanetType::Frozen:
		case EPlanetType::Rogue:
			return 0.3f;
		case EPlanetType::Carbon:
		case EPlanetType::Ice:
		case EPlanetType::Sand:
			return 0.4f;
		case EPlanetType::Desert:
		case EPlanetType::HighMountain:
			return 0.5f;
		case EPlanetType::Tundra:
		case EPlanetType::Ammonia:
			return 0.8f;
		case EPlanetType::Nordic:
		case EPlanetType::Oasis:
		case EPlanetType::Savanna:
			return 0.9f;
		case EPlanetType::Terrestrial:
		case EPlanetType::Forest:
		case EPlanetType::Archipelago:
		case EPlanetType::Pangea:
		case EPlanetType::Exoplanet:
		case EPlanetType::Melted:
			return 1.0f;
		case EPlanetType::Ocean:
		case EPlanetType::Water:
		case EPlanetType::Lava:
			return 1.1f;
		case EPlanetType::SuperEarth:
			return 1.4f;
		case EPlanetType::Volcanic:
			return 1.6f;
		case EPlanetType::Sulfur:
			return 2.0f;
		case EPlanetType::Greenhouse:
		case EPlanetType::GasGiant:
		case EPlanetType::HotGiant:
		case EPlanetType::IceGiant:
			return 3.0f;
		default:
			return 1.0f;
		}
	}

	float MoonDensity(const EMoonType Type)
	{
		switch (Type)
		{
		case EMoonType::CapturedAsteroid:
			return 0.0f;
		case EMoonType::Iron:
		case EMoonType::Rocky:
			return 0.05f;
		case EMoonType::Icy:
			return 0.1f;
		case EMoonType::TidallyLocked:
			return 0.2f;
		case EMoonType::Desert:
		case EMoonType::Peculiar:
			return 0.3f;
		case EMoonType::Volcanic:
			return 0.5f;
		case EMoonType::Ocean:
			return 0.6f;
		case EMoonType::Continental:
			return 0.8f;
		case EMoonType::Gas:
			return 1.0f;
		default:
			return 0.2f;
		}
	}
}

float APSAtmosphereModel::Density(const APlanetaryBody* Body)
{
	using namespace APSAtmosphereModelPrivate;
	if (!IsValid(Body) || Body->AtmosphereHeight <= 0.0)
	{
		return 0.0f;
	}
	float Base = 1.0f;
	if (const AMoon* Moon = Cast<AMoon>(Body))
	{
		Base = MoonDensity(Moon->MoonType);
	}
	else if (const APlanet* Planet = Cast<APlanet>(Body))
	{
		Base = PlanetDensity(Planet->PlanetType);
	}
	// A little variety per world, fixed by its surface seed: +-15%.
	const uint32 Hash = GetTypeHash(Body->WorldScapeSeed) * 2654435761u;
	const float Variation = 0.85f + 0.3f * static_cast<float>(Hash >> 8) / static_cast<float>(1u << 24);
	return FMath::Clamp(Base * Variation, 0.0f, 3.0f);
}

float APSAtmosphereModel::DaySkyMasking(const float Density)
{
	// An airless sky is black by day and hides nothing; from about 0.7 of Earth's air a lit sky hides the stars.
	return FMath::Pow(FMath::Clamp(Density / 0.7f, 0.0f, 1.0f), 0.7f);
}

FText APSAtmosphereModel::Describe(const float Density)
{
	return Density < 0.05f ? LOCTEXT("None", "NONE")
		: Density < 0.5f ? LOCTEXT("Thin", "THIN")
		: Density < 1.5f ? LOCTEXT("EarthLike", "EARTH-LIKE")
		: Density < 2.5f ? LOCTEXT("Dense", "DENSE")
		: LOCTEXT("Crushing", "CRUSHING");
}

#undef LOCTEXT_NAMESPACE
