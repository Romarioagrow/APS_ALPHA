#pragma once
UENUM(BlueprintType)
enum class EAstroGenerationLevel : uint8
{
	StarCluster		UMETA(DisplayName = "Star Cluster"), 
	GalaxiesCluster	UMETA(DisplayName = "Galaxies Cluster"), 
	Galaxy			UMETA(DisplayName = "Galaxy"), 
	StarSystem		UMETA(DisplayName = "Star System"), 
	PlanetSystem	UMETA(DisplayName = "Planet System"), 
	SinglePlanet	UMETA(DisplayName = "Single Planet"), 
};