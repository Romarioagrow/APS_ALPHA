#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EStarClusterComposition : uint8
{
	AllSpectral		UMETA(DisplayName = "All Spectral"),
	OnlyBlue		UMETA(DisplayName = "Only Blue"),
	MostlyBlue		UMETA(DisplayName = "Mostly Blue"),
	BlueWhite		UMETA(DisplayName = "Blue White"),
	OnlyWhite		UMETA(DisplayName = "Only White"),
	MostlyWhite		UMETA(DisplayName = "Mostly White"),
	WhiteYellow		UMETA(DisplayName = "White Yellow"),
	OnlyYellow		UMETA(DisplayName = "Only Yellow"),
	MostlyYellow	UMETA(DisplayName = "Mostly Yellow"),
	YellowOrange	UMETA(DisplayName = "Yellow Orange"),
	OnlyOrange		UMETA(DisplayName = "Only Orange"),
	MostlyOrange	UMETA(DisplayName = "Mostly Orange"),
	OrangeRed		UMETA(DisplayName = "Orange Red"),
	OnlyRed			UMETA(DisplayName = "Only Red"),
	MostlyRed		UMETA(DisplayName = "Mostly Red"),
	Unknown			UMETA(DisplayName = "Unknown")
};
