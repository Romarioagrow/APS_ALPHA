#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EStarClusterSize : uint8
{
	Tiny			UMETA(DisplayName = "Tiny"),
	Small			UMETA(DisplayName = "Small"),
	Medium			UMETA(DisplayName = "Medium"),
	Large			UMETA(DisplayName = "Large"),
	Giant			UMETA(DisplayName = "Giant"),
	Unknown			UMETA(DisplayName = "Unknown"),
	// Rio 03.10: a size above Giant. Appended after Unknown so every serialized byte stays.
	Colossal		UMETA(DisplayName = "Colossal"),
}; 