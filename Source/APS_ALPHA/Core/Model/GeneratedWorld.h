#pragma once

#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Saves/GeneratedWorldData.h"
#include "GeneratedWorld.generated.h"

class APlanetarySystem;
class APlanet;

enum class EPlanetType : uint8;
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
enum class EStarType : uint8;
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
	bool bGenerateHomeSystem{ true };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bStartWithHomePlanet{ false };

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomHomeSystem{ false };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeSystemType{ false };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeStar{ false };

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomStartPlanetNumber{ false };


	UPROPERTY(EditAnywhere, Category = "Generation Params")
	EAstroGenerationLevel AstroGenerationLevel{ EAstroGenerationLevel::StarCluster };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterSize StarClusterSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterType StarClusterType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStarType StarType{ EStarType::SingleStar };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStellarType StellarType{ EStellarType::MainSequence };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	ESpectralClass SpectralClass{ ESpectralClass::G };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetarySystemType PlanetarySystemType{ EPlanetarySystemType::MultiPlanetSystem };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EOrbitDistributionType OrbitDistributionType{ EOrbitDistributionType::Uniform };

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	EHomeSystemPosition HomeSystemPosition;
	
	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetType PlanetType{ EPlanetType::Frozen };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxySize{ 250 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxyStarCount{ 100000 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int PlanetsAmount{ 0 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int MoonsAmount{ 0 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int StartPlanetIndex{ 0 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double GalaxyStarDensity{ 10.0 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double PlanetRadius{ 6750.0 };
		
	
	UPROPERTY()
	APlanetarySystem* HomePlanetarySystem;
	
	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	APlanet* HomePlanet;

	
	// Свойства атмосферы
	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereHeight{ 100.0 };

	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereOpacity{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereMultiScattering{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereRayleighScattering{ 8.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FLinearColor AtmosphereColor {FLinearColor(3.8f, 13.5f, 33.0f, 0.0f)};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	int StarsAmount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	FName HomeStarName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	FName HomePlanetName;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	FName FullSpectralName;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	int HomeStarTemperature;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	double HomeStarRadius;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	double HomeStarMass;

	FGeneratedWorldData SaveWorldData() const
	{
		FGeneratedWorldData WorldData;
		WorldData.bGenerateFullScaledWorld = bGenerateFullScaledWorld;
		WorldData.bGenerateHomeSystem = bGenerateHomeSystem;
		WorldData.bStartWithHomePlanet = bStartWithHomePlanet;
		WorldData.bRandomHomeSystem = bRandomHomeSystem;
		WorldData.bRandomHomeSystemType = bRandomHomeSystemType;
		WorldData.bRandomHomeStar = bRandomHomeStar;
		WorldData.bRandomStartPlanetNumber = bRandomStartPlanetNumber;
		WorldData.AstroGenerationLevel = AstroGenerationLevel;
		WorldData.GalaxyType = GalaxyType;
		WorldData.GalaxyClass = GalaxyClass;
		WorldData.StarClusterSize = StarClusterSize;
		WorldData.StarClusterType = StarClusterType;
		WorldData.StarClusterPopulation = StarClusterPopulation;
		WorldData.StarClusterComposition = StarClusterComposition;
		WorldData.StarType = StarType;
		WorldData.StellarType = StellarType;
		WorldData.SpectralClass = SpectralClass;
		WorldData.PlanetarySystemType = PlanetarySystemType;
		WorldData.OrbitDistributionType = OrbitDistributionType;
		WorldData.HomeSystemPosition = HomeSystemPosition;
		WorldData.PlanetType = PlanetType;
		WorldData.GalaxySize = GalaxySize;
		WorldData.GalaxyStarCount = GalaxyStarCount;
		WorldData.PlanetsAmount = PlanetsAmount;
		WorldData.MoonsAmount = MoonsAmount;
		WorldData.StartPlanetIndex = StartPlanetIndex;
		WorldData.GalaxyStarDensity = GalaxyStarDensity;
		WorldData.PlanetRadius = PlanetRadius;
		WorldData.AtmosphereHeight = AtmosphereHeight;
		WorldData.AtmosphereOpacity = AtmosphereOpacity;
		WorldData.AtmosphereMultiScattering = AtmosphereMultiScattering;
		WorldData.AtmosphereRayleighScattering = AtmosphereRayleighScattering;
		WorldData.AtmosphereColor = AtmosphereColor;
		WorldData.StarsAmount = StarsAmount;
		WorldData.HomeStarName = HomeStarName;
		WorldData.HomePlanetName = HomePlanetName;
		WorldData.FullSpectralName = FullSpectralName;
		WorldData.HomeStarTemperature = HomeStarTemperature;
		WorldData.HomeStarRadius = HomeStarRadius;
		WorldData.HomeStarMass = HomeStarMass;

		return WorldData;
	}
};
