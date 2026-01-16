#pragma once

#include "CoreMinimal.h"
#include "CivMenuTypes.generated.h"

UENUM(BlueprintType)
enum class ECivMenuScreen : uint8
{
	Overview        UMETA(DisplayName="Overview"),
	Divisions       UMETA(DisplayName="Divisions"),
	DivisionDetails UMETA(DisplayName="Division Details"),
};
