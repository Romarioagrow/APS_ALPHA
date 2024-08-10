#include "PlanetGeosphere.h"

#include "APS_ALPHA/Core/Enums/CrustThicknessLevel.h"
#include "APS_ALPHA/Core/Enums/MagneticFieldStrength.h"
#include "APS_ALPHA/Core/Enums/SeismicActivityLevel.h"

void FPlanetGeosphere::UpdateGeosphereProperties()
{
	SeismicActivity = CalculateSeismicActivity(SeismicActivityLevel);
	CrustThickness = CalculateCrustThickness(CrustThicknessLevel);
	MagneticFieldStrength = CalculateMagneticFieldStrength(MagneticFieldStrengthLevel);
}

float FPlanetGeosphere::CalculateSeismicActivity(ESeismicActivityLevel Level)
{
	switch(Level)
	{
	case ESeismicActivityLevel::Inactive: return 0.0f;    // Нет активности
	case ESeismicActivityLevel::Minor: return 1.0f;       // Незначительная активность
	case ESeismicActivityLevel::Moderate: return 3.0f;    // Умеренная активность
	case ESeismicActivityLevel::Active: return 5.0f;      // Активная сейсмическая активность
	case ESeismicActivityLevel::Intense: return 7.0f;     // Интенсивная активность
	default: return 3.0f; // Умеренная активность по умолчанию
	}
}

float FPlanetGeosphere::CalculateCrustThickness(ECrustThicknessLevel Level)
{
	switch(Level)
	{
	case ECrustThicknessLevel::VeryThin: return 5.0f;     // 5 км
	case ECrustThicknessLevel::Thin: return 10.0f;        // 10 км
	case ECrustThicknessLevel::Medium: return 35.0f;      // 35 км
	case ECrustThicknessLevel::Thick: return 70.0f;       // 70 км
	case ECrustThicknessLevel::VeryThick: return 100.0f;  // 100 км
	default: return 35.0f; // Средняя толщина по умолчанию
	}
}

float FPlanetGeosphere::CalculateMagneticFieldStrength(EMagneticFieldStrength Level)
{
	switch(Level)
	{
	case EMagneticFieldStrength::NoField: return 0.0f;       // Нет магнитного поля
	case EMagneticFieldStrength::WeakField: return 0.1f;     // Слабое магнитное поле
	case EMagneticFieldStrength::ModerateField: return 0.5f; // Умеренное магнитное поле
	case EMagneticFieldStrength::StrongField: return 1.0f;   // Сильное магнитное поле
	case EMagneticFieldStrength::IntenseField: return 2.0f;  // Интенсивное магнитное поле
	default: return 0.5f; // Умеренное магнитное поле по умолчанию
	}
}
