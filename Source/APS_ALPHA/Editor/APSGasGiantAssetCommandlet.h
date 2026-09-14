#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "APSGasGiantAssetCommandlet.generated.h"

/** Builds only the project-owned gas-giant cloud material; never edits worlds or imported assets. */
UCLASS()
class APS_ALPHA_API UAPSGasGiantAssetCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSGasGiantAssetCommandlet();
	virtual int32 Main(const FString& Params) override;
};
