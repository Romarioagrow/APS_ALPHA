#pragma once

#include "CoreMinimal.h"
#include "GravityState.generated.h"

UENUM(BlueprintType)
enum class EGravityState : uint8
{
	LowG,
	Attracting,
	Jumping,
	Falling,
	Attracted
};

