// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "PlanetarySystemGenerationModel.h"
#include "StarGenerationModel.h"
#include "PlanetarySystem.h"
#include "OrbitDistributionType.h"
#include "PlanetGenerationModel.h"
#include "PlanetaryZoneType.h"
#include "PlanetGenerator.h"
#include "MoonGenerator.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "PlanetaryProceduralGenerator.generated.h"

typedef float PlanetProbability;

USTRUCT(BlueprintType)
struct FPlanetTypeProbability
{
    GENERATED_USTRUCT_BODY()

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
        EPlanetType PlanetType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
        float Probability;

    FPlanetTypeProbability()
    {
        PlanetType = EPlanetType::Unknown;
        Probability = 0.0f;
    }

    FPlanetTypeProbability(EPlanetType PlanetType, float Probability)
    {
        this->PlanetType = PlanetType;
        this->Probability = Probability;
    }
};

struct FDensityRange
{
    double MinDensity;
    double MaxDensity;

    FDensityRange() : MinDensity(0.0), MaxDensity(0.0) {}
    FDensityRange(double Min, double Max) : MinDensity(Min), MaxDensity(Max) {}
};

struct FRadiusRange 
{
    float MinRadius;
    float MaxRadius;

    FRadiusRange() {}

    FRadiusRange(float Min, float Max) : MinRadius(Min), MaxRadius(Max) {}
};

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UPlanetarySystemGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

//public:
    //UPlanetarySystemGenerator();

    //FRandomStream RandomStream;

public:
    void SetAstroLocation(int StarNumber, APlanetarySystem* NewPlanetarySystem);

    void GenerateCustomPlanetarySystem();

    int DetermineMaxPlanets(EStellarType StellarClass, FStarModel StarModel);

    void ApplyModel(APlanetarySystem* NewPlanetarySystem, TSharedPtr<FPlanetarySystemModel> PlanetraySystemModel);

    void GenerateCustomPlanetraySystemModel(TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel, TSharedPtr<FStarModel> StarModel, UPlanetGenerator* PlanetGenerator, UMoonGenerator* MoonGenerator);

    void GeneratePlanetraySystemModelByStar(TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel, TSharedPtr<FStarModel> StarModel, UPlanetGenerator* PlanetGenerator, UMoonGenerator* MoonGenerator);

    void GeneratePlanetOrbits();

    int CalculateMoons(double PlanetMass, EPlanetType PlanetType);

    FRadiusRange GetPlanetRadiusRange(EPlanetType PlanetType);

    FDensityRange GetPlanetDensityRange(EPlanetType PlanetType);
    
private:
    EPlanetaryZoneType DeterminePlanetZone(double OrbitRadius, TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel );

    EPlanetType DeterminePlanetType(EPlanetaryZoneType PlanetZone);

    double RandGauss();

    EOrbitDistributionType ChooseDistributionType(EStellarType StellarClass, float StarMass, float MinOrbit, float MaxOrbit);

    EOrbitDistributionType ChooseOrbitDistribution(EStellarType stellarClass);

    TArray<double> OrbitRadii;

private:
    const TMap<EPlanetType, FRadiusRange> PlanetRadiusRanges =
    {
        {EPlanetType::Terrestrial,  FRadiusRange(0.9, 1.1)},     // Землеподобные
        {EPlanetType::Rocky,        FRadiusRange(0.5, 0.9)},     // Скалистые
        {EPlanetType::Greenhouse,   FRadiusRange(0.9, 1.1)},     // Парниковые
        {EPlanetType::Melted,       FRadiusRange(0.6, 1.1)},     // Расплавленные
        {EPlanetType::HotGiant,     FRadiusRange(9.4, 12.6)},    // Горячие гиганты
        {EPlanetType::GasGiant,     FRadiusRange(7.9, 11.0)},    // Газовые гиганты
        {EPlanetType::IceGiant,     FRadiusRange(3.1, 7.9)},     // Ледяные гиганты
        {EPlanetType::Dwarf,        FRadiusRange(0.2, 0.5)},     // Карликовые планеты
        {EPlanetType::Exoplanet,    FRadiusRange(0.2, 3)},    // Экзопланеты
        {EPlanetType::Rogue,        FRadiusRange(0.2, 2)},    // Бродячие планеты
        {EPlanetType::Ocean,        FRadiusRange(0.8, 1.1)},     // Океанические планеты
        {EPlanetType::Desert,       FRadiusRange(0.8, 1.1)},     // Пустынные планеты
        {EPlanetType::Forest,       FRadiusRange(0.9, 1.3)},     // Лесные планеты
        {EPlanetType::Volcanic,     FRadiusRange(0.8, 1.1)},     // Вулканические планеты
        {EPlanetType::Ice,          FRadiusRange(0.3, 0.8)},     // Ледяные планеты
        {EPlanetType::Frozen,       FRadiusRange(0.3, 0.8)},     // Замороженные планеты
        {EPlanetType::Ammonia,      FRadiusRange(0.6, 0.9)},     // Планеты аммиака
        {EPlanetType::Metal,         FRadiusRange(0.8, 1.1)},     // Железные планеты
        {EPlanetType::Carbon,       FRadiusRange(0.8, 1.1)},     // Углеродные планеты
        {EPlanetType::SuperEarth,   FRadiusRange(1.3, 2)},     // Суперземли
        {EPlanetType::Lava,         FRadiusRange(0.8, 1.1)},     // Лавовые планеты
        {EPlanetType::Metallic,     FRadiusRange(0.8, 1.3)},     // Металлические планеты
        {EPlanetType::Unknown,      FRadiusRange(0.2, 5)}     // Неизвестные планеты
    };

    const TMap<EPlanetType, FDensityRange> PlanetDensityRanges =
    {
        {EPlanetType::Terrestrial,  FDensityRange(5.0, 5.8)},
        {EPlanetType::Rocky,        FDensityRange(5.0, 6.0)},
        {EPlanetType::Greenhouse,   FDensityRange(4.5, 6.0)},
        {EPlanetType::Melted,       FDensityRange(4.0, 6.0)},
        {EPlanetType::HotGiant,     FDensityRange(0.4, 2.0)},
        {EPlanetType::GasGiant,     FDensityRange(0.2, 2.5)},
        {EPlanetType::IceGiant,     FDensityRange(1.5, 3.0)},
        {EPlanetType::Dwarf,        FDensityRange(1.3, 3.0)},
        {EPlanetType::Exoplanet,    FDensityRange(1.0, 10.0)},
        {EPlanetType::Rogue,        FDensityRange(1.0, 10.0)},
        {EPlanetType::Ocean,        FDensityRange(3.0, 5.0)},
        {EPlanetType::Desert,       FDensityRange(2.5, 4.5)},
        {EPlanetType::Forest,       FDensityRange(4.0, 6.0)},
        {EPlanetType::Volcanic,     FDensityRange(3.0, 6.0)},
        {EPlanetType::Ice,          FDensityRange(2.0, 3.5)},
        {EPlanetType::Frozen,       FDensityRange(2.0, 3.5)},
        {EPlanetType::Ammonia,      FDensityRange(2.0, 3.5)},
        {EPlanetType::Metal,         FDensityRange(6.0, 8.0)},
        {EPlanetType::Carbon,       FDensityRange(2.0, 3.5)},
        {EPlanetType::SuperEarth,   FDensityRange(6.0, 9.0)},
        {EPlanetType::Lava,         FDensityRange(3.0, 6.0)},
        {EPlanetType::Metallic,     FDensityRange(5.0, 10.0)},
        {EPlanetType::Unknown,      FDensityRange(0.1, 10.0)}
    };


    const TMap<EStellarType, TMap<EOrbitDistributionType, float>> StellarOrbitDistributions =
    {
        {
            EStellarType::HyperGiant, 
            {
                {EOrbitDistributionType::Uniform, 0.05f}, 
                {EOrbitDistributionType::Gaussian, 0.10f}, 
                {EOrbitDistributionType::Chaotic, 0.50f}, 
                {EOrbitDistributionType::InnerOuter, 0.25f}, 
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EStellarType::SuperGiant, 
            {
                {EOrbitDistributionType::Uniform, 0.10f}, 
                {EOrbitDistributionType::Gaussian, 0.20f}, 
                {EOrbitDistributionType::Chaotic, 0.40f}, 
                {EOrbitDistributionType::InnerOuter, 0.20f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EStellarType::BrightGiant, 
            {
                {EOrbitDistributionType::Uniform, 0.15f}, 
                {EOrbitDistributionType::Gaussian, 0.25f},
                {EOrbitDistributionType::Chaotic, 0.30f}, 
                {EOrbitDistributionType::InnerOuter, 0.20f}, 
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EStellarType::Giant, 
            {
                {EOrbitDistributionType::Uniform, 0.20f}, 
                {EOrbitDistributionType::Gaussian, 0.30f}, 
                {EOrbitDistributionType::Chaotic, 0.25f}, 
                {EOrbitDistributionType::InnerOuter, 0.15f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {   
            EStellarType::SubGiant,    
            {
                {EOrbitDistributionType::Uniform, 0.25f}, 
                {EOrbitDistributionType::Gaussian, 0.35f}, 
                {EOrbitDistributionType::Chaotic, 0.20f},
                {EOrbitDistributionType::InnerOuter, 0.10f},
                {EOrbitDistributionType::Dense, 0.50f}
            }
        },
        {
            EStellarType::MainSequence, 
            {
                {EOrbitDistributionType::Uniform, 0.3f}, 
                {EOrbitDistributionType::Gaussian, 0.2f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.5f}, 
                {EOrbitDistributionType::Dense, 0.05f}
            }
        },
        {
            EStellarType::SubDwarf, 
            {
                {EOrbitDistributionType::Uniform, 0.35f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.10f}, 
                {EOrbitDistributionType::InnerOuter, 0.10f}, 
                {EOrbitDistributionType::Dense, 0.5f}
            }
        },
        {   
            EStellarType::WhiteDwarf, 
            {
                {EOrbitDistributionType::Uniform, 0.40f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.10f}, 
                {EOrbitDistributionType::Dense, 0.5f}
            }
        },
        {   EStellarType::BrownDwarf, 
            {
                {EOrbitDistributionType::Uniform, 0.45f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.05f}, 
                {EOrbitDistributionType::Dense, 0.1f}
            }
        },
        {
            EStellarType::Neutron, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.03f},
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EStellarType::Protostar, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EStellarType::Pulsar, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {   
            EStellarType::BlackHole, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 1.f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EStellarType::Unknown, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        }
    };

    const TMap<EStellarType, int> BasePlanetCount = {
        {EStellarType::HyperGiant, 15},
        {EStellarType::SuperGiant, 10},
        {EStellarType::BrightGiant, 10},
        {EStellarType::Giant, 7},
        {EStellarType::SubGiant, 4},
        {EStellarType::MainSequence, 10},
        {EStellarType::SubDwarf, 3},
        {EStellarType::WhiteDwarf, 2},
        {EStellarType::BrownDwarf, 0},
        {EStellarType::Neutron, 1},
        {EStellarType::Protostar, 2},
        {EStellarType::Pulsar, 1},
        {EStellarType::BlackHole, 0},
};

    const TMap<EStellarType, PlanetProbability> BasePlanetProbabilities =
    {
        {EStellarType::HyperGiant, 0.7f},
        {EStellarType::SuperGiant, 0.7f},
        {EStellarType::BrightGiant, 0.7f},
        {EStellarType::Giant, 0.65f},
        {EStellarType::SubGiant, 0.6f},
        {EStellarType::MainSequence, 0.9f},
        {EStellarType::SubDwarf, 0.3f},
        {EStellarType::WhiteDwarf, 0.1f},
        {EStellarType::BrownDwarf, 0.05f},
        {EStellarType::Neutron, 0.01f},
        {EStellarType::Protostar, 0.6f},
        {EStellarType::Pulsar, 0.01f},
        {EStellarType::BlackHole, 0.01f}
    };

    template <typename T>
    T GetRandomWithProbability(const TMap<T, int32>& Probabilities)
    {
        int32 TotalWeight = 0;
        for (auto const& Pair : Probabilities) {
            TotalWeight += Pair.Value;
        }

        int32 RandomValue = FMath::RandRange(0, TotalWeight - 1);

        for (auto const& Pair : Probabilities) {
            if (RandomValue < Pair.Value) {
                return Pair.Key;
            }
            RandomValue -= Pair.Value;
        }

        // В случае ошибки возвращаем первый элемент в словаре
        return Probabilities.CreateConstIterator().Key();
    }

    struct FPlanetarySystemRanges
    {
        TTuple<int, int> AmountOfPlanetsRange;
        TTuple<double, double> DistanceBetweenPlanetsRange;

        FPlanetarySystemRanges(TTuple<int, int> Planets, TTuple<double, double> Distance)
            : AmountOfPlanetsRange(Planets), DistanceBetweenPlanetsRange(Distance) {}
    };

    const TMap<EPlanetaryZoneType, TArray<FPlanetTypeProbability>> ZonePlanetProbabilities =
    {
        {EPlanetaryZoneType::HabitableZone,
        {
            {EPlanetType::Terrestrial, 0.20f},
            {EPlanetType::Ocean, 0.10f},
            {EPlanetType::Forest, 0.10f},
            {EPlanetType::SuperEarth, 0.20f},
            {EPlanetType::Water, 0.10f},
            {EPlanetType::GasGiant, 0.01f},
            {EPlanetType::Desert, 0.5f},
            {EPlanetType::Rocky, 0.3f},

        }},

        {EPlanetaryZoneType::DeadZone,
        {
            {EPlanetType::Dwarf, 0.5f},
            //{EPlanetType::Asteroid, 0.5f}
        }},
        
        {EPlanetaryZoneType::Unknown,
        {
            {EPlanetType::Unknown, 1.f},
            //{EPlanetType::Asteroid, 0.5f}
        }},

        {EPlanetaryZoneType::ColdZone,
        {
            //{EPlanetType::IceGiant, 0.5f},
            {EPlanetType::Ice, 0.5f},
            {EPlanetType::Frozen, 0.5f},
            {EPlanetType::Rocky, 0.5f},
            {EPlanetType::Terrestrial, 0.1f}
        }},

        {EPlanetaryZoneType::IceZone,
        {
            {EPlanetType::IceGiant, 0.2f},
            {EPlanetType::GasGiant, 0.1f},
            {EPlanetType::Ice, 0.5f},
            {EPlanetType::Frozen, 0.9f},
            {EPlanetType::Rocky, 0.5f},
        }},

        {EPlanetaryZoneType::WarmZone,
        {
            {EPlanetType::Terrestrial, 0.2f},
            {EPlanetType::Ocean, 0.2f},
            {EPlanetType::Water, 0.2f},
            {EPlanetType::Carbon, 0.1f},
            {EPlanetType::Metal, 0.5f},
            {EPlanetType::Metallic, 0.5f},
            {EPlanetType::Volcanic, 0.3f},
            {EPlanetType::Greenhouse, 0.5f},
            {EPlanetType::Ammonia, 0.3f},
        }},

        {EPlanetaryZoneType::HotZone,
        {
            {EPlanetType::Volcanic, 0.3f},
            {EPlanetType::Lava, 0.4f},
            {EPlanetType::Metal, 0.3f},
            {EPlanetType::Metallic, 0.3f},
            {EPlanetType::HotGiant, 0.1f},
            {EPlanetType::Melted, 0.5f}
        }},

        {EPlanetaryZoneType::InnerPlanetZone,
        {
            {EPlanetType::Rocky, 0.4f},
            {EPlanetType::Metal, 0.3f},
            {EPlanetType::Terrestrial, 0.3f}
        }},

        {EPlanetaryZoneType::OuterPlanetZone,
        {
            {EPlanetType::GasGiant, 0.1f},
            {EPlanetType::IceGiant, 0.1f},
            {EPlanetType::Rocky, 0.1f},
            {EPlanetType::Metal, 0.1f},
            {EPlanetType::Terrestrial, 0.1f},
            {EPlanetType::Ammonia, 0.1f},
            {EPlanetType::Dwarf, 0.1f},
            {EPlanetType::Ice, 0.1f}

        }},

        {EPlanetaryZoneType::AsteroidBeltZone,
        {
            {EPlanetType::Dwarf, 0.5f},
            //{EPlanetType::Asteroid, 0.5f}
        }},

        {EPlanetaryZoneType::KuiperBeltZone,
        {
            {EPlanetType::Dwarf, 0.7f},
            {EPlanetType::Ice, 0.3f},
            {EPlanetType::Rocky, 0.2f},

        }},

        {EPlanetaryZoneType::GasGiantsZone,
        {
            {EPlanetType::GasGiant, 0.5f},
            {EPlanetType::IceGiant, 0.5f},
            {EPlanetType::Rocky, 0.1f},
            {EPlanetType::Metal, 0.1f},
            {EPlanetType::Terrestrial, 0.1f},
            {EPlanetType::Ammonia, 0.1f},
            {EPlanetType::Dwarf, 0.1f},
            {EPlanetType::Ice, 0.1f}
        }},
        // и так далее для каждой зоны...
    };

    // TODO: For Fully Random Generation 
    /*TMap<EPlanetarySystemType, FPlanetarySystemRanges> PlanetarySystemRanges =
    {
        {EPlanetarySystemType::NoPlanetSystem, FPlanetarySystemRanges(TTuple<int, int>(0, 0), TTuple<double, double>(0.0, 0.0))},
        {EPlanetarySystemType::SmallSystem, FPlanetarySystemRanges(TTuple<int, int>(1, 5), TTuple<double, double>(1.0, 3.0))},
        {EPlanetarySystemType::LargeSystem, FPlanetarySystemRanges(TTuple<int, int>(6, 12), TTuple<double, double>(1.0, 5.0))},
        {EPlanetarySystemType::ChaoticSystem, FPlanetarySystemRanges(TTuple<int, int>(5, 12), TTuple<double, double>(0.1, 2.0))},
        {EPlanetarySystemType::DenseSystem, FPlanetarySystemRanges(TTuple<int, int>(3, 10), TTuple<double, double>(0.2, 1.0))}
    };*/
};
