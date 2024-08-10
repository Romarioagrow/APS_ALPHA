#pragma once

#include "CoreMinimal.h"
#include "PlanetAtmosphere.generated.h"

enum class EPressureLevel : uint8;
enum class EHumidityLevel : uint8;
enum class EWindSpeed : uint8;
enum class ECO2Level : uint8;
enum class EOxigenLevel : uint8;

USTRUCT(BlueprintType)

struct FPlanetAtmosphere
{
	GENERATED_BODY()

	// Уровень кислорода
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	EOxigenLevel OxygenLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	float OxygenConcentration; // Вычисляемое значение

	// Уровень CO2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	ECO2Level CO2Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	float CO2Concentration; // Вычисляемое значение

	// Скорость ветра
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	EWindSpeed WindSpeedLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	float WindSpeed; // Вычисляемое значение

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	EHumidityLevel HumidityLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	float Humidity; // Вычисляемое значение

	// Атмосферное давление
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	EPressureLevel PressureLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	float AtmosphericPressure;

	// Методы вычисления значений
	void UpdateAtmosphereProperties();
	
	float CalculateOxygenConcentration(EOxigenLevel Level);
	
	float CalculateCo2Concentration(ECO2Level Level);
	
	float CalculateWindSpeed(EWindSpeed Level);

	float CalculateHumidity(EHumidityLevel Level);
	
	float CalculateAtmosphericPressure(EPressureLevel Level);
};