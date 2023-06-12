#pragma once
UENUM(BlueprintType)
enum class EGalaxyType : uint8
{
	Elliptical		UMETA(DisplayName = "Elliptical Galaxy"),
	Lenticular		UMETA(DisplayName = "Lenticular Galaxy"),
	Spiral 			UMETA(DisplayName = "Spiral Galaxy"),
	BarredSpiral	UMETA(DisplayName = "Barred Spiral Galaxy"),
	Irregular 		UMETA(DisplayName = "Irregular Galaxy"),
	Unknown			UMETA(DisplayName = "Unknown"),
};