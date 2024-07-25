#pragma once
UENUM(BlueprintType)
enum class EHomeSystemPosition : uint8
{
	WorldCenter				UMETA(DisplayName = "World Center"), 
	RandomPosition			UMETA(DisplayName = "Random Position"), 
	DirectPosition			UMETA(DisplayName = "Direct Position"), 
};