#pragma once

#include "CoreMinimal.h"
#include "PlanetBiosphere.generated.h"

enum class EBiodiversityIndex : uint8;
enum class EBiomassLevel : uint8;

USTRUCT(BlueprintType)
struct FPlanetBiosphere //: public AEnvironmentActor
{
	GENERATED_BODY()
	
	// ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biosphere")
	EBiomassLevel BiomassLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biosphere")
	float Biomass; // ����������� ��������

	// ������ ���������������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biosphere")
	EBiodiversityIndex BiodiversityLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biosphere")
	float BiodiversityIndexValue; // ����������� ��������

	// ������ ���������� ��������
	void UpdateBiosphereProperties();
	
	float CalculateBiomass(EBiomassLevel Level);
	
	float CalculateBiodiversityIndex(EBiodiversityIndex Level);
};