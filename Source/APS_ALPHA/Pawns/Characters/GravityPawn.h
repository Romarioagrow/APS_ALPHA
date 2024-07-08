#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Gravity/GravityActorInterface.h"
#include "UObject/Interface.h"
#include "GravityPawn.generated.h"

UINTERFACE(MinimalAPI)
class UGravityPawn : public UGravityActorInterface
{
	GENERATED_BODY()
};

class APS_ALPHA_API IGravityPawn
{
	GENERATED_BODY()

public:
	virtual void UpdateGravity() = 0;

	virtual void UpdateGravityType() = 0;

	virtual void SwitchGravityType(AActor* GravitySourceActor) = 0;
};
