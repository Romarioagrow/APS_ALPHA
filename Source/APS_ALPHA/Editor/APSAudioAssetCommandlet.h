#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "APSAudioAssetCommandlet.generated.h"

/** Creates the small APS audio bank from installed Fab packs. Never resaves source waves. */
UCLASS()
class APS_ALPHA_API UAPSAudioAssetCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UAPSAudioAssetCommandlet();
	virtual int32 Main(const FString& Params) override;
};
