#pragma once

#include "Commandlets/Commandlet.h"
#include "APSFixTranslucentNaniteCommandlet.generated.h"

/** One-shot maintenance commandlet for UE 5.4-incompatible translucent Nanite meshes. */
UCLASS()
class APS_ALPHA_API UAPSFixTranslucentNaniteCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSFixTranslucentNaniteCommandlet();
	virtual int32 Main(const FString& Params) override;
};
