#include "GeneratedWorld.h"

#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/HomeSystemPosition.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"

/*AGeneratedWorld::AGeneratedWorld(): bRandomHomeSystemType(false), bRandomHomeStar(false), GalaxyType(), GalaxyGlass(),
                                    StarClusterSize(),
                                    StarClusterType(),
                                    StarClusterPopulation(),
                                    StarClusterComposition(),
                                    HomeSystemPosition(),
                                    HomePlanetarySystem(nullptr),
                                    HomePlanet(nullptr)
{
}*/


AGeneratedWorld::AGeneratedWorld()
{
	bGenerateFullScaledWorld = true;
	bGenerateHomeSystem = false;
	bStartWithHomePlanet = false;
	bRandomHomeSystem = true;
	bRandomHomeSystemType = false;
	bRandomHomeStar = false;
	bRandomStartPlanetNumber = true;
	AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
	GalaxyType = EGalaxyType::Elliptical;  
	GalaxyGlass = EGalaxyClass::E0;  
	StarClusterSize = EStarClusterSize::Giant;  
	StarClusterType = EStarClusterType::Nebula;  
	StarClusterPopulation = EStarClusterPopulation::Dwarfs;  
	StarClusterComposition = EStarClusterComposition::Unknown;  
	HomeSystemStarType = EStarSystemType::SingleStar;
	HomeStarStellarType = EStellarType::MainSequence;
	HomeStarSpectralClass = ESpectralClass::G;
	HomeSystemPlanetaryType = EPlanetarySystemType::MultiPlanetSystem;
	HomeSystemOrbitDistributionType = EOrbitDistributionType::Uniform;
	HomeSystemPosition = EHomeSystemPosition::DirectPosition;  
	GalaxySize = 250;
	GalaxyStarCount = 100000;
	StartPlanetNumber = 0;
	PlanetsAmount = 0;
	GalaxyStarDensity = 10.0;
	HomePlanetarySystem = nullptr;
	HomePlanet = nullptr;
}