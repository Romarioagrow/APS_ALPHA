#pragma once

UENUM(BlueprintType)
enum class EPlanetaryZoneType : uint8
{
	DeadZone			UMETA(DisplayName = "Dead Zone"),
	HabitableZone		UMETA(DisplayName = "Habitable Zone"),
	ColdZone			UMETA(DisplayName = "Cold Zone"),
	IceZone				UMETA(DisplayName = "Ice Zone"),
	WarmZone			UMETA(DisplayName = "Warm Zone"),
	HotZone				UMETA(DisplayName = "Hot Zone"),
	InnerPlanetZone		UMETA(DisplayName = "Inner Planet Zone"),
	OuterPlanetZone		UMETA(DisplayName = "Outer Planet Zone"),
	AsteroidBeltZone	UMETA(DisplayName = "Asteroid Belt Zone"),
	KuiperBeltZone		UMETA(DisplayName = "Kuiper Belt Zone"),
	GasGiantsZone		UMETA(DisplayName = "Gas Giants Zone"),
	Unknown				UMETA(DisplayName = "Unknown Zone"),
}; 
