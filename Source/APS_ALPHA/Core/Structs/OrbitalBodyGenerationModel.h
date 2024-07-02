#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/CelestialGenerationModel.h"
#include "OrbitalBodyGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FOrbitalBodyModel :
	public FCelestialBodyModel
{
	GENERATED_BODY()

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double RotationSpeed{0};

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double OrbitDistance{0};

	//UPROPERTY(VisibleAnywhere, Category = "Planet")
	TArray<FVector> LagrangePoints;
};
