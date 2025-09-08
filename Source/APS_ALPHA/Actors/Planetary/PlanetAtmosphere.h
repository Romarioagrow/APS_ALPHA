#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/Planetary/CO2Level.h"
#include "APS_ALPHA/Core/Enums/Planetary/HumidityLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/OxigenLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/PressureLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/WindSpeed.h"
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

	// Конструктор по умолчанию для инициализации всех свойств
	FPlanetAtmosphere()
		: OxygenLevel(EOxigenLevel::NoOxigen)
		, OxygenConcentration(0.0f)
		, CO2Level(ECO2Level::NoCO2)
		, CO2Concentration(0.0f)
		, WindSpeedLevel(EWindSpeed::NoWind)
		, WindSpeed(0.0f)
		, HumidityLevel(EHumidityLevel::NoHumidity)
		, Humidity(0.0f)
		, PressureLevel(EPressureLevel::NoPressure)
		, AtmosphericPressure(0.0f)
	{
	}

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	EOxigenLevel OxygenLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	float OxygenConcentration; // ����������� ��������

	// ������� CO2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	ECO2Level CO2Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	float CO2Concentration; // ����������� ��������

	// �������� �����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	EWindSpeed WindSpeedLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	float WindSpeed; // ����������� ��������

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	EHumidityLevel HumidityLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	float Humidity; // ����������� ��������

	// ����������� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	EPressureLevel PressureLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	float AtmosphericPressure;

	// ������ ���������� ��������
	void UpdateAtmosphereProperties();
	
	float CalculateOxygenConcentration(EOxigenLevel Level);
	
	float CalculateCo2Concentration(ECO2Level Level);
	
	float CalculateWindSpeed(EWindSpeed Level);

	float CalculateHumidity(EHumidityLevel Level);
	
	float CalculateAtmosphericPressure(EPressureLevel Level);
};