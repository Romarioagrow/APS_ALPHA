#pragma once

#include "CoreMinimal.h"
#include "PlanetGeosphere.generated.h"

enum class EMagneticFieldStrength : uint8;
enum class ECrustThicknessLevel : uint8;
enum class ESeismicActivityLevel : uint8;

USTRUCT(BlueprintType)
struct FPlanetGeosphere //: public AEnvironmentActor
{
	GENERATED_BODY()

	// ������������ ����������
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
