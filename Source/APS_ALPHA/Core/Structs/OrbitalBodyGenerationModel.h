#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/CelestialGenerationModel.h"
#include "OrbitalBodyGenerationModel.generated.h"

USTRUCT(BlueprintType)
struct FOrbitalBodyModel :
	public FCelestialBodyModel
{
	GENERATED_BODY()

	double RotationSpeed{0};

	double OrbitDistance{0};

	TArray<FVector> LagrangePoints;
};
