#pragma once
UENUM(BlueprintType)
enum class EAstroGenerationLevel : uint8
{
	GalaxiesCluster	UMETA(DisplayName = "Galaxies Cluster"), 
	Galaxy			UMETA(DisplayName = "Galaxy"), 
	StarCluster		UMETA(DisplayName = "Star Cluster"), 
	StarSystem		UMETA(DisplayName = "Star System"), 
	PlanetSystem	UMETA(DisplayName = "Planet System"), 
	SinglePlanet	UMETA(DisplayName = "Single Planet"), 
	Random			UMETA(DisplayName = "Random"),
};