#pragma once

#include "Commandlets/Commandlet.h"
#include "APSFixStarHISMMaterialCommandlet.generated.h"

/** Removes temporal dither flags from the two legacy HISM star master materials. */
UCLASS()
class APS_ALPHA_API UAPSFixStarHISMMaterialCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSFixStarHISMMaterialCommandlet();
	virtual int32 Main(const FString& Params) override;
};
