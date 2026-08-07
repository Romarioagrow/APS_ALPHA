#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "APSPlanetSurfaceAssetCommandlet.generated.h"

/** Generates project-owned WorldScape family instances and the surface catalog. */
UCLASS()
class APS_ALPHA_API UAPSPlanetSurfaceAssetCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSPlanetSurfaceAssetCommandlet();
	virtual int32 Main(const FString& Params) override;
};

#endif
