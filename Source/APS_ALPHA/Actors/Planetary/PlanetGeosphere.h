#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/Planetary/CrustThicknessLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/MagneticFieldStrength.h"
#include "APS_ALPHA/Core/Enums/Planetary/SeismicActivityLevel.h"
#include "PlanetGeosphere.generated.h"

enum class EMagneticFieldStrength : uint8;
enum class ECrustThicknessLevel : uint8;
enum class ESeismicActivityLevel : uint8;

USTRUCT(BlueprintType)
struct FPlanetGeosphere //: public AEnvironmentActor
{
	GENERATED_BODY()

	// Конструктор по умолчанию для инициализации всех свойств
	FPlanetGeosphere()
		: SeismicActivityLevel(ESeismicActivityLevel::Inactive)
		, SeismicActivity(0.0f)
		, CrustThicknessLevel(ECrustThicknessLevel::Thin)
		, CrustThickness(0.0f)
		, MagneticFieldStrengthLevel(EMagneticFieldStrength::NoField)
		, MagneticFieldStrength(0.0f)
	{
	}

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geosphere")
	ESeismicActivityLevel SeismicActivityLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Geosphere")
	float SeismicActivity; // ����������� ��������

	// ������� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geosphere")
	ECrustThicknessLevel CrustThicknessLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Geosphere")
	float CrustThickness; // ����������� ��������

	// ���� ���������� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geosphere")
	EMagneticFieldStrength MagneticFieldStrengthLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Geosphere")
	float MagneticFieldStrength; // ����������� ��������

	// ������ ���������� ��������
	void UpdateGeosphereProperties();
	
	float CalculateSeismicActivity(ESeismicActivityLevel Level);
	
	float CalculateCrustThickness(ECrustThicknessLevel Level);
	
	float CalculateMagneticFieldStrength(EMagneticFieldStrength Level);
};
