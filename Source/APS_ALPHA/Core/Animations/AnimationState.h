#pragma once

#include "CoreMinimal.h"
#include "AnimationState.generated.h"

UENUM(BlueprintType)
enum class EAnimationState : uint8
{
	Idle,
	Walk,
	Run,
	Jumping,
	Falling,
	Floating,
	OnGround
};

