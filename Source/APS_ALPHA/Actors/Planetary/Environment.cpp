#include "Environment.h"

#include "APS_ALPHA/Core/Enums/HumidityLevel.h"
#include "APS_ALPHA/Core/Enums/PressureLevel.h"

AEnvironment::AEnvironment()
{
	// Инициализация по умолчанию
	HumidityLevel = EHumidityLevel::NoHumidity;
	PressureLevel = EPressureLevel::NoPressure;

	UpdateEnvironmentProperties();
}

void AEnvironment::BeginPlay()
{
	Super::BeginPlay();
	UpdateEnvironmentProperties();
}

void AEnvironment::UpdateEnvironmentProperties()
{
	Humidity = CalculateHumidity(HumidityLevel);
	AtmosphericPressure = CalculateAtmosphericPressure(PressureLevel);
}

float AEnvironment::CalculateHumidity(EHumidityLevel Level)
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

float AEnvironment::CalculateAtmosphericPressure(EPressureLevel Level)
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
