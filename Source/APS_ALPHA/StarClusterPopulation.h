#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EStarClusterPopulation : uint8
{
	AllSequenses	UMETA(DisplayName = "All Sequenses Stars"),
	MainSequence	UMETA(DisplayName = "Only Main Sequence Stars"),
	Giants			UMETA(DisplayName = "Mostly Giants"),
	Dwarfs			UMETA(DisplayName = "Mostly Dwarfs"),
	Protostars		UMETA(DisplayName = "Mostly Protostars"),
	Unknown			UMETA(DisplayName = "Unknown")
}; 
