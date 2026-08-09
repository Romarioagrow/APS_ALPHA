#pragma once

#include "Commandlets/Commandlet.h"
#include "APSFixStarHISMMaterialCommandlet.generated.h"

/**
 * Rebuilds the canonical stellar masters with one scale-independent procedural
 * surface recipe. A deliberately subtle seeded pulse keeps materialized stars alive,
 * while the unified shader strongly damps that temporal response for distant HISM stars.
 */
UCLASS()
class APS_ALPHA_API UAPSFixStarHISMMaterialCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSFixStarHISMMaterialCommandlet();
	virtual int32 Main(const FString& Params) override;
};
