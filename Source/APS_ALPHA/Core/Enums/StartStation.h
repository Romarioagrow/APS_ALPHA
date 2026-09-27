#pragma once
#include "CoreMinimal.h"

/** Which home-complex station an orbital start places the pilot in. */
UENUM(BlueprintType)
enum class EAPSStartStation : uint8
{
	HomeStation		UMETA(DisplayName = "Home Station"),
	Headquarters	UMETA(DisplayName = "Headquarters"),
	Shipyard		UMETA(DisplayName = "Shipyard"),
};
