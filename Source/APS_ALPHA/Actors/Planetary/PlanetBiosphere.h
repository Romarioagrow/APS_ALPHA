#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/Planetary/BiodiversityIndex.h"
#include "APS_ALPHA/Core/Enums/Planetary/BiomassLevel.h"
#include "PlanetBiosphere.generated.h"

enum class EBiodiversityIndex : uint8;
enum class EBiomassLevel : uint8;

USTRUCT(BlueprintType)
struct FPlanetBiosphere //: public AEnvironmentActor
{
	GENERATED_BODY()
	
	// Конструктор по умолчанию для инициализации всех свойств
	FPlanetBiosphere()
		: BiomassLevel(EBiomassLevel::NoBiomass)
		, Biomass(0.0f)
		, BiodiversityLevel(EBiodiversityIndex::NoBiodiversity)
		, BiodiversityIndexValue(0.0f)
	{
	}

	// 
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