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

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    bool bRandomStartPlanetNumber;

    UPROPERTY(VisibleAnywhere, Category = "Generation Params")
    EAstroGenerationLevel AstroGenerationLevel;

    UPROPERTY(VisibleAnywhere, Category = "Galaxy")
    EGalaxyType GalaxyType;

    UPROPERTY(VisibleAnywhere, Category = "Galaxy")
    EGalaxyClass GalaxyClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterSize StarClusterSize;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterType StarClusterType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterPopulation StarClusterPopulation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    EStarClusterComposition StarClusterComposition;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    EStarType StarType;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    EStellarType StellarType;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    ESpectralClass SpectralClass;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    EPlanetarySystemType PlanetarySystemType;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    EOrbitDistributionType OrbitDistributionType;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    EHomeSystemPosition HomeSystemPosition;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    EPlanetType PlanetType;

    UPROPERTY(VisibleAnywhere, Category = "Galaxy")
    int GalaxySize;

    UPROPERTY(VisibleAnywhere, Category = "Galaxy")
    int GalaxyStarCount;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    int PlanetsAmount;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    int MoonsAmount;

    UPROPERTY(VisibleAnywhere, Category = "Home System")
    int StartPlanetIndex;

    UPROPERTY(VisibleAnywhere, Category = "Galaxy")
    double GalaxyStarDensity;

    UPROPERTY(VisibleAnywhere, Category = "Galaxy")
    double PlanetRadius;

    UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
    double AtmosphereHeight;

    UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
    double AtmosphereOpacity;

    UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
    double AtmosphereMultiScattering;

    UPROPERTY(VisibleAnywhere, Category = "Atmosphere")
    double AtmosphereRayleighScattering;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Atmosphere")
    FLinearColor AtmosphereColor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
    int StarsAmount;
};

