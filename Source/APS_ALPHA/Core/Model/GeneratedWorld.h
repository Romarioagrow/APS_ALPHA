#pragma once

#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarSystemType.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "GeneratedWorld.generated.h"

class APlanetarySystem;
class APlanet;

enum class EHomeSystemPosition : uint8;
enum class EStarClusterComposition : uint8;
enum class EStarClusterPopulation : uint8;
enum class EStarClusterType : uint8;
enum class EStarClusterSize : uint8;
enum class EGalaxyClass : uint8;
enum class EGalaxyType : uint8;
enum class EOrbitDistributionType : uint8;
enum class EPlanetarySystemType : uint8;
enum class ESpectralClass : uint8;
enum class EStellarType : uint8;
enum class EStarSystemType : uint8;
enum class EAstroGenerationLevel : uint8;

UCLASS()
class UGeneratedWorld : public UObject
{
	GENERATED_BODY()
	
public:
	UGeneratedWorld();
	void PrintAllValues() const;

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateFullScaledWorld{ true };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateHomeSystem{ false };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bStartWithHomePlanet{ false };

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomHomeSystem{ true };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeSystemType;

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeStar;

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomStartPlanetNumber{ true };


	UPROPERTY(EditAnywhere, Category = "Generation Params")
	EAstroGenerationLevel AstroGenerationLevel{ EAstroGenerationLevel::StarCluster };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyGlass;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterSize StarClusterSize;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterType StarClusterType;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStarSystemType HomeSystemStarType{ EStarSystemType::SingleStar };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStellarType HomeStarStellarType{ EStellarType::MainSequence };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	ESpectralClass HomeStarSpectralClass{ ESpectralClass::G };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetarySystemType HomeSystemPlanetaryType{ EPlanetarySystemType::MultiPlanetSystem };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EOrbitDistributionType HomeSystemOrbitDistributionType{ EOrbitDistributionType::Uniform };

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	EHomeSystemPosition HomeSystemPosition;
	

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxySize{ 250 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxyStarCount{ 100000 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomStartPlanetNumber"))
	int StartPlanetNumber{ 0 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int PlanetsAmount{ 0 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double GalaxyStarDensity{ 10.0 };
	
	
	UPROPERTY()
	APlanetarySystem* HomePlanetarySystem;
	
	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	APlanet* HomePlanet;
};
