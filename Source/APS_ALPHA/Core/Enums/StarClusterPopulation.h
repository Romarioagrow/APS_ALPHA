#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EStarClusterPopulation : uint8
{
	// Rio 09.10 (playtest 26): the label read "All Sequenses Stars". Only the display text changed: saves, benches and
	// the roll keep the value name AllSequenses.
	AllSequenses	UMETA(DisplayName = "All Sequences Stars"),
	MainSequence	UMETA(DisplayName = "Only Main Sequence Stars"),
	Giants			UMETA(DisplayName = "Mostly Giants"),
	Dwarfs			UMETA(DisplayName = "Mostly Dwarfs"),
	Protostars		UMETA(DisplayName = "Mostly Protostars"),
	Unknown			UMETA(DisplayName = "Unknown")
}; 
