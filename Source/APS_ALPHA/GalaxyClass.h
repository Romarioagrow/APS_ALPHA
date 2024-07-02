#pragma once
UENUM(BlueprintType)
enum class EGalaxyClass : uint8
{
	E0			UMETA(DisplayName = "E0 Elliptical"),
	E1			UMETA(DisplayName = "E1 Elliptical"),
	E2			UMETA(DisplayName = "E2 Elliptical"),
	E3			UMETA(DisplayName = "E3 Elliptical"),
	E4			UMETA(DisplayName = "E4 Elliptical"),
	E5			UMETA(DisplayName = "E5 Elliptical"),
	E6			UMETA(DisplayName = "E6 Elliptical"),
	E7			UMETA(DisplayName = "E7 Elliptical"),
	S0			UMETA(DisplayName = "S0 Lenticular"),
	Sa			UMETA(DisplayName = "Sa Spiral"),
	Sb			UMETA(DisplayName = "Sb Spiral"),
	Sc			UMETA(DisplayName = "Sc Spiral"),
	Sd			UMETA(DisplayName = "Sd Spiral"),
	SBa			UMETA(DisplayName = "SBa Barred Spiral"),
	SBb			UMETA(DisplayName = "SBb Barred Spiral"),
	SBc			UMETA(DisplayName = "SBc Barred Spiral"),
	SBd			UMETA(DisplayName = "SBd Barred Spiral"),
	Irr			UMETA(DisplayName = "Irr Irregular"),
	Unknown		UMETA(DisplayName = "Unknown"),
};