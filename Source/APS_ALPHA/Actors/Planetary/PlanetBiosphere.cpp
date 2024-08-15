#include "PlanetBiosphere.h"

#include "APS_ALPHA/Core/Enums/Planetary/BiodiversityIndex.h"
#include "APS_ALPHA/Core/Enums/Planetary/BiomassLevel.h"


void FPlanetBiosphere::UpdateBiosphereProperties()
{
	Biomass = CalculateBiomass(BiomassLevel);
	BiodiversityIndexValue = CalculateBiodiversityIndex(BiodiversityLevel);
}

float FPlanetBiosphere::CalculateBiomass(EBiomassLevel Level)
{
	switch(Level)
	{
	case EBiomassLevel::NoBiomass: return 0.0f;
	case EBiomassLevel::Sparse: return 100.0f;
	case EBiomassLevel::LowBiomass: return 500.0f;
	case EBiomassLevel::ModerateBiomass: return 1000.0f;
	case EBiomassLevel::HighBiomass: return 5000.0f;
	case EBiomassLevel::Dense: return 10000.0f;
	default: return 1000.0f;
	}
}

float FPlanetBiosphere::CalculateBiodiversityIndex(EBiodiversityIndex Level)
{
	switch(Level)
	{
	case EBiodiversityIndex::NoBiodiversity: return 0.0f;
	case EBiodiversityIndex::Monoculture: return 0.1f;
	case EBiodiversityIndex::LowDiversity: return 1.0f;
	case EBiodiversityIndex::ModerateDiversity: return 3.0f;
	case EBiodiversityIndex::HighDiversity: return 5.0f;
	case EBiodiversityIndex::VeryDiverse: return 7.0f;
	default: return 3.0f;
	}
}
