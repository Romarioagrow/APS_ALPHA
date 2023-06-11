#pragma once
UENUM(BlueprintType)
enum class EAstroGenerationLevel : uint8
{
	Galaxy			UMETA(DisplayName = "Galaxy"), // Unknown
	Cluster			UMETA(DisplayName = "Cluster"), // Unknown
	StarSystem		UMETA(DisplayName = "StarSystem"), // Unknown
	PlanetSystem	UMETA(DisplayName = "PlanetSystem"), // Unknown
	SinglePlanet	UMETA(DisplayName = "SinglePlanet"), // Unknown
	Random			UMETA(DisplayName = "Random"), // Unknown
};