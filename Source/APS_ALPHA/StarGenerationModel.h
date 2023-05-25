#pragma once
#include "StarType.h"
#include "StarSpectralClass.h"
#include "SpectralType.h"

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
        EStellarClass StellarClass; /// TODO: Rename to StellarType
	
	UPROPERTY(VisibleAnywhere)
		ESpectralClass SpectralClass;

	UPROPERTY(VisibleAnywhere)
		ESpectralType SpectralType;

	UPROPERTY(VisibleAnywhere)
		float Luminosity;

	UPROPERTY(VisibleAnywhere)
		int SurfaceTemperature;
	
	UPROPERTY(VisibleAnywhere)
		FString Age;

	UPROPERTY(VisibleAnywhere)
		FName FullSpectralClass;

	UPROPERTY(VisibleAnywhere)
		FName FullSpectralName;

	UPROPERTY(VisibleAnywhere)
		int SpectralSubclass;

	UPROPERTY(VisibleAnywhere)
		EStellarClass StarStellarClass;
};

