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
	case ESeismicActivityLevel::Inactive: return 0.0f;    // ��� ����������
	case ESeismicActivityLevel::Minor: return 1.0f;       // �������������� ����������
	case ESeismicActivityLevel::Moderate: return 3.0f;    // ��������� ����������
	case ESeismicActivityLevel::Active: return 5.0f;      // �������� ������������ ����������
	case ESeismicActivityLevel::Intense: return 7.0f;     // ����������� ����������
	default: return 3.0f; // ��������� ���������� �� ���������
	}
}

float FPlanetGeosphere::CalculateCrustThickness(ECrustThicknessLevel Level)
{
	switch(Level)
	{
	case ECrustThicknessLevel::VeryThin: return 5.0f;     // 5 ��
	case ECrustThicknessLevel::Thin: return 10.0f;        // 10 ��
	case ECrustThicknessLevel::Medium: return 35.0f;      // 35 ��
	case ECrustThicknessLevel::Thick: return 70.0f;       // 70 ��
	case ECrustThicknessLevel::VeryThick: return 100.0f;  // 100 ��
	default: return 35.0f; // ������� ������� �� ���������
	}
}

float FPlanetGeosphere::CalculateMagneticFieldStrength(EMagneticFieldStrength Level)
{
	switch(Level)
	{
	case EMagneticFieldStrength::NoField: return 0.0f;       // ��� ���������� ����
	case EMagneticFieldStrength::WeakField: return 0.1f;     // ������ ��������� ����
	case EMagneticFieldStrength::ModerateField: return 0.5f; // ��������� ��������� ����
	case EMagneticFieldStrength::StrongField: return 1.0f;   // ������� ��������� ����
	case EMagneticFieldStrength::IntenseField: return 2.0f;  // ����������� ��������� ����
	default: return 0.5f; // ��������� ��������� ���� �� ���������
	}
}
