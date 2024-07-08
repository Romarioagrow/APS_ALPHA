#pragma once

#include <Components/SphereComponent.h>
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GravityObjectInterface.generated.h"

UINTERFACE(MinimalAPI)
class UGravityObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class APS_ALPHA_API IGravityObjectInterface
{
	GENERATED_BODY()

public:
	virtual USphereComponent* GetGravityField() PURE_VIRTUAL(IHomingActorInterface, return nullptr;);
};
