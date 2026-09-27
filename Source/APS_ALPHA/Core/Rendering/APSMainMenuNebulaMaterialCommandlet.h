#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "APSMainMenuNebulaMaterialCommandlet.generated.h"

/** Builds only the reusable landing/deep-space nebula master material. */
UCLASS()
class APS_ALPHA_API UAPSMainMenuNebulaMaterialCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSMainMenuNebulaMaterialCommandlet();
	virtual int32 Main(const FString& Params) override;
};
