#include "PlanetAtmosphere.h"

#include "APS_ALPHA/Core/Enums/Planetary/CO2Level.h"
#include "APS_ALPHA/Core/Enums/Planetary/HumidityLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/OxigenLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/PressureLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/WindSpeed.h"

void FPlanetAtmosphere::UpdateAtmosphereProperties()
{
	OxygenConcentration = CalculateOxygenConcentration(OxygenLevel);
	CO2Concentration = CalculateCo2Concentration(CO2Level);
	WindSpeed = CalculateWindSpeed(WindSpeedLevel);
}

float FPlanetAtmosphere::CalculateOxygenConcentration(EOxigenLevel Level)
{
	switch(Level)
	{
	case EOxigenLevel::NoOxigen: return 0.0f;
	case EOxigenLevel::Trace: return 0.1f;
	case EOxigenLevel::LowOxygen: return 10.0f;
	case EOxigenLevel::EarthLike: return 21.0f;
	case EOxigenLevel::HighOxygen: return 30.0f;
	case EOxigenLevel::Hyperoxic: return 40.0f;
	default: return 21.0f;
	}
}

float FPlanetAtmosphere::CalculateCo2Concentration(ECO2Level Level)
{
	switch(Level)
	{
	case ECO2Level::NoCO2: return 0.0f;
	case ECO2Level::PreIndustrial: return 280.0f;
	case ECO2Level::Modern: return 410.0f;
	case ECO2Level::Elevated: return 600.0f;
	case ECO2Level::High: return 1000.0f;
	case ECO2Level::Extreme: return 5000.0f;
	default: return 410.0f;
	}
}

float FPlanetAtmosphere::CalculateWindSpeed(EWindSpeed Level)
{
	switch(Level)
	{
	case EWindSpeed::NoWind: return 0.0f;
	case EWindSpeed::Calm: return 0.5f;
	case EWindSpeed::Breeze: return 5.0f;
	case EWindSpeed::Gale: return 15.0f;
	case EWindSpeed::Storm: return 30.0f;
	case EWindSpeed::Hurricane: return 50.0f;
	default: return 5.0f;
	}
}

float FPlanetAtmosphere::CalculateHumidity(EHumidityLevel Level)
{
	switch(Level)
	{
	case EHumidityLevel::DesertDry: return 5.0f;
	case EHumidityLevel::Arid: return 20.0f;
	case EHumidityLevel::Moderate: return 50.0f;
	case EHumidityLevel::Humid: return 75.0f;
	case EHumidityLevel::Tropical: return 90.0f;
	default: return 50.0f;
	}
}

float FPlanetAtmosphere::CalculateAtmosphericPressure(EPressureLevel Level)
{
	switch(Level)
	{
	case EPressureLevel::LowPressure: return 80000.0f;
	case EPressureLevel::BelowAverage: return 95000.0f;
	case EPressureLevel::Average: return 101325.0f;
	case EPressureLevel::AboveAverage: return 105000.0f;
	case EPressureLevel::HighPressure: return 110000.0f;
	default: return 101325.0f;
	}
}
