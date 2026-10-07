#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "APSPlanetSurfaceAssetCommandlet.generated.h"

// Rio 06.10 (packaged build): always declared because UHT emits the .gen.cpp for game targets too; only
// the generator body in the .cpp is WITH_EDITOR (same pattern as APSAudioAssetCommandlet).
/** Generates project-owned WorldScape family instances and the surface catalog. */
UCLASS()
class APS_ALPHA_API UAPSPlanetSurfaceAssetCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSPlanetSurfaceAssetCommandlet();
	virtual int32 Main(const FString& Params) override;
};
