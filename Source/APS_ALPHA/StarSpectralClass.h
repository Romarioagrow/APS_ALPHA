#pragma once
#include "CoreMinimal.h"
#include "StarSpectralClass.generated.h"

UENUM(BlueprintType)
enum class EStarSpectralClass : uint8
{
	O 	UMETA(DisplayName = "O - Blue"),
	B 	UMETA(DisplayName = "B - Blue White"),
	A 	UMETA(DisplayName = "A - White"),
	F 	UMETA(DisplayName = "F - Yellow White"),
	G 	UMETA(DisplayName = "G - Yellow"),
	K 	UMETA(DisplayName = "K - Orange"),
	M 	UMETA(DisplayName = "M - Red"),
	L 	UMETA(DisplayName = "L - Brown Dwarf"),
	T 	UMETA(DisplayName = "T - Methane Dwarf"),
	Y 	UMETA(DisplayName = "Y - Cool Brown Dwarf"),
	WD 	UMETA(DisplayName = "White Dwarf"),
	NS 	UMETA(DisplayName = "Neutron Star"),
	BH 	UMETA(DisplayName = "Black Hole")
};