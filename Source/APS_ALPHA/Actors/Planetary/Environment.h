#pragma once

#include "CoreMinimal.h"
#include "EnvironmentActor.h"
#include "PlanetaryEnvironment.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "Environment.generated.h"

enum class EPressureLevel : uint8;
enum class EHumidityLevel : uint8;

UCLASS()
class APS_ALPHA_API AEnvironment : public AEnvironmentActor
{
	GENERATED_BODY()

public:
	AEnvironment();

protected:
	virtual void BeginPlay() override;

public:
	// ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	EHumidityLevel HumidityLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	float Humidity; // ����������� ��������

	// ����������� ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	EPressureLevel PressureLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	float AtmosphericPressure; // ����������� ��������

	// ������ ���������� ��������
	void UpdateEnvironmentProperties();
	
	float CalculateHumidity(EHumidityLevel Level);
	
	float CalculateAtmosphericPressure(EPressureLevel Level);
};
