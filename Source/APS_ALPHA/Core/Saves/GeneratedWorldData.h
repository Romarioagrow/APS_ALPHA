#pragma once
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/HomeSystemPosition.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "GeneratedWorldData.generated.h"

USTRUCT(BlueprintType)
struct FGeneratedWorldData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Generation Params")
    bool bGenerateFullScaledWorld;

    UPROPERTY(VisibleAnywhere, Category = "Generation Params")
    bool bGenerateHomeSystem;

    UPROPERTY(VisibleAnywhere, Category = "Generation Params")
    bool bStartWithHomePlanet;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    bool bRandomHomeSystem;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    bool bRandomHomeSystemType;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    bool bRandomHomeStar;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    bool bRandomStartPlanetNumber;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation Params")
    EAstroGenerationLevel AstroGenerationLevel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy")
    EGalaxyType GalaxyType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy")
    EGalaxyClass GalaxyClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterSize StarClusterSize;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterType StarClusterType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterPopulation StarClusterPopulation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterComposition StarClusterComposition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    EStarType StarType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    EStellarType StellarType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    ESpectralClass SpectralClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    EPlanetarySystemType PlanetarySystemType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    EOrbitDistributionType OrbitDistributionType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    EHomeSystemPosition HomeSystemPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    EPlanetType PlanetType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy")
    int GalaxySize;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy")
    int GalaxyStarCount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    int PlanetsAmount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    int MoonsAmount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Home System")
    int StartPlanetIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy")
    double GalaxyStarDensity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy")
    double PlanetRadius;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
    double AtmosphereHeight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
    double AtmosphereOpacity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
    double AtmosphereMultiScattering;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
    double AtmosphereRayleighScattering;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
    FLinearColor AtmosphereColor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    int StarsAmount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star")
    FName HomeStarName;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
    FName HomePlanetName;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star")
    FName FullSpectralName;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star")
    double HomeStarMass;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star")
    double HomeStarRadius;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star")
    int HomeStarTemperature;
};

