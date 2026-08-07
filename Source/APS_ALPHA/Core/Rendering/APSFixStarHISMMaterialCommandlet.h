#pragma once

#include "Commandlets/Commandlet.h"
#include "APSFixStarHISMMaterialCommandlet.generated.h"

/**
 * Rebuilds the canonical stellar masters with one scale-independent procedural
 * surface recipe and removes temporal features that make distant HISM stars flicker.
 */
UCLASS()
class APS_ALPHA_API UAPSFixStarHISMMaterialCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSFixStarHISMMaterialCommandlet();
	virtual int32 Main(const FString& Params) override;
};
