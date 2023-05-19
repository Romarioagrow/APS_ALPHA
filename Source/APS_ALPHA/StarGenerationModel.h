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
    
        // Тип звезды (например, карлик, гигант, сверхгигант и т.д.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star")
        EStarType StarType;

	UPROPERTY(VisibleAnywhere)
		float Luminosity;

	UPROPERTY(VisibleAnywhere)
		int SurfaceTemperature;

	UPROPERTY(VisibleAnywhere)
		EStarSpectralClass StarSpectralClass;
};

