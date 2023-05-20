#pragma once
#include "StarType.h"
#include "StarSpectralClass.h"

#include "CelestialGenerationModel.h"
#include "CoreMinimal.h"
#include "StarGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FStarGenerationModel :
    public FCelestialGenerationModel
{
    GENERATED_BODY()
    
        // ��� ������ (��������, ������, ������, ����������� � �.�.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star")
        EStellarClass StellarClass;
	
	UPROPERTY(VisibleAnywhere)
		ESpectralClass SpectralClass;

	UPROPERTY(VisibleAnywhere)
		float Luminosity;

	UPROPERTY(VisibleAnywhere)
		int SurfaceTemperature;
	
	UPROPERTY(VisibleAnywhere)
		double Age;

};

