#pragma once

#include "CoreMinimal.h"
#include "PlanetBiosphere.generated.h"

enum class EBiodiversityIndex : uint8;
enum class EBiomassLevel : uint8;

USTRUCT(BlueprintType)
struct FPlanetBiosphere //: public AEnvironmentActor
{
	GENERATED_BODY()
	
	// Биомасса
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biosphere")
	EBiomassLevel BiomassLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biosphere")
	float Biomass; // Вычисляемое значение

	// Индекс биоразнообразия
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biosphere")
	EBiodiversityIndex BiodiversityLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biosphere")
	float BiodiversityIndexValue; // Вычисляемое значение

	// Методы вычисления значений
	void UpdateBiosphereProperties();
	
	float CalculateBiomass(EBiomassLevel Level);
	
	float CalculateBiodiversityIndex(EBiodiversityIndex Level);
};