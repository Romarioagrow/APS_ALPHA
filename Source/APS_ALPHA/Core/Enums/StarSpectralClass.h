#pragma once
#include "CoreMinimal.h"
#include "StarSpectralClass.generated.h"

UENUM(BlueprintType)
enum class ESpectralClass : uint8
{
	M 	UMETA(DisplayName = "M - Red"),
	K 	UMETA(DisplayName = "K - Orange"),
	G 	UMETA(DisplayName = "G - Yellow"),
	F 	UMETA(DisplayName = "F - Yellow-White"),
	A 	UMETA(DisplayName = "A - White"),
	B 	UMETA(DisplayName = "B - Blue-White"),
	O 	UMETA(DisplayName = "O - Blue"),
	L 	UMETA(DisplayName = "L - Brown Dwarf"),
	T 	UMETA(DisplayName = "T - Tauri Dwarf"),
	Y 	UMETA(DisplayName = "Y - Cool Brown Dwarf"),
	NS 	UMETA(DisplayName = "Neutron Star"),
	PS 	UMETA(DisplayName = "Proto Star"),
	BH 	UMETA(DisplayName = "Black Hole"),
	Unknown	UMETA(DisplayName = "Unknown")
};