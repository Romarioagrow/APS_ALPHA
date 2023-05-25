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

//USTRUCT(BlueprintType)
//struct FPlanetTypeProbability
//{
//    GENERATED_USTRUCT_BODY()
//    EPlanetType Type;
//    float Probability;
//};

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
    //GENERATED_USTRUCT_BODY()

    double MinDensity;
    double MaxDensity;

    FDensityRange() : MinDensity(0.0), MaxDensity(0.0) {}
    FDensityRange(double Min, double Max) : MinDensity(Min), MaxDensity(Max) {}
};

struct FRadiusRange {

   // GENERATED_USTRUCT_BODY()

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

public:


    int DetermineMaxPlanets(EStellarClass StellarClass, FStarGenerationModel StarModel);

    void ApplyModel(APlanetarySystem* NewPlanetarySystem, FPlanetarySystemGenerationModel PlanetraySystemModel);

    FPlanetarySystemGenerationModel GeneratePlanetraySystemModelByStar(FStarGenerationModel StarModel, UPlanetGenerator* PlanetGenerator, UMoonGenerator* MoonGenerator);

    int CalculateMoons(double PlanetMass, EPlanetType PlanetType);

    FRadiusRange GetPlanetRadiusRange(EPlanetType PlanetType);

    FDensityRange GetPlanetDensityRange(EPlanetType PlanetType);

    
private:
    EPlanetaryZoneType DeterminePlanetZone(double OrbitRadius, FPlanetarySystemGenerationModel PlanetarySystemModel);

    EPlanetType DeterminePlanetType(EPlanetaryZoneType PlanetZone);

    double RandGauss();

    EOrbitDistributionType ChooseDistributionType(EStellarClass StellarClass, float StarMass, float MinOrbit, float MaxOrbit);

    EOrbitDistributionType ChooseOrbitDistribution(EStellarClass stellarClass);

    TArray<double> OrbitRadii;

typedef float PlanetProbability;



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
        {EPlanetType::Exoplanet,    FRadiusRange(0.2, 12.6)},    // Экзопланеты
        {EPlanetType::Rogue,        FRadiusRange(0.2, 12.6)},    // Бродячие планеты
        {EPlanetType::Ocean,        FRadiusRange(0.8, 1.1)},     // Океанические планеты
        {EPlanetType::Desert,       FRadiusRange(0.8, 1.1)},     // Пустынные планеты
        {EPlanetType::Forest,       FRadiusRange(0.9, 1.3)},     // Лесные планеты
        {EPlanetType::Volcanic,     FRadiusRange(0.8, 1.1)},     // Вулканические планеты
        {EPlanetType::Ice,          FRadiusRange(0.3, 0.8)},     // Ледяные планеты
        {EPlanetType::Frozen,       FRadiusRange(0.3, 0.8)},     // Замороженные планеты
        {EPlanetType::Ammonia,      FRadiusRange(0.6, 0.9)},     // Планеты аммиака
        {EPlanetType::Iron,         FRadiusRange(0.8, 1.1)},     // Железные планеты
        {EPlanetType::Carbon,       FRadiusRange(0.8, 1.1)},     // Углеродные планеты
        {EPlanetType::SuperEarth,   FRadiusRange(1.5, 3.1)},     // Суперземли
        {EPlanetType::Lava,         FRadiusRange(0.8, 1.1)},     // Лавовые планеты
        {EPlanetType::Metallic,     FRadiusRange(0.8, 1.3)},     // Металлические планеты
        {EPlanetType::Unknown,      FRadiusRange(0.2, 12.6)}     // Неизвестные планеты
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
        {EPlanetType::Iron,         FDensityRange(6.0, 8.0)},
        {EPlanetType::Carbon,       FDensityRange(2.0, 3.5)},
        {EPlanetType::SuperEarth,   FDensityRange(6.0, 9.0)},
        {EPlanetType::Lava,         FDensityRange(3.0, 6.0)},
        {EPlanetType::Metallic,     FDensityRange(5.0, 10.0)},
        {EPlanetType::Unknown,      FDensityRange(0.1, 10.0)}
    };


    const TMap<EStellarClass, TMap<EOrbitDistributionType, float>> StellarOrbitDistributions =
    {
        {
            EStellarClass::HyperGiant, 
            {
                {EOrbitDistributionType::Uniform, 0.05f}, 
                {EOrbitDistributionType::Gaussian, 0.10f}, 
                {EOrbitDistributionType::Chaotic, 0.50f}, 
                {EOrbitDistributionType::InnerOuter, 0.25f}, 
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EStellarClass::SuperGiant, 
            {
                {EOrbitDistributionType::Uniform, 0.10f}, 
                {EOrbitDistributionType::Gaussian, 0.20f}, 
                {EOrbitDistributionType::Chaotic, 0.40f}, 
                {EOrbitDistributionType::InnerOuter, 0.20f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EStellarClass::BrightGiant, 
            {
                {EOrbitDistributionType::Uniform, 0.15f}, 
                {EOrbitDistributionType::Gaussian, 0.25f},
                {EOrbitDistributionType::Chaotic, 0.30f}, 
                {EOrbitDistributionType::InnerOuter, 0.20f}, 
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EStellarClass::Giant, 
            {
                {EOrbitDistributionType::Uniform, 0.20f}, 
                {EOrbitDistributionType::Gaussian, 0.30f}, 
                {EOrbitDistributionType::Chaotic, 0.25f}, 
                {EOrbitDistributionType::InnerOuter, 0.15f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {   
            EStellarClass::SubGiant,    
            {
                {EOrbitDistributionType::Uniform, 0.25f}, 
                {EOrbitDistributionType::Gaussian, 0.35f}, 
                {EOrbitDistributionType::Chaotic, 0.20f},
                {EOrbitDistributionType::InnerOuter, 0.10f},
                {EOrbitDistributionType::Dense, 0.50f}
            }
        },
        {
            EStellarClass::MainSequence, 
            {
                {EOrbitDistributionType::Uniform, 0.3f}, 
                {EOrbitDistributionType::Gaussian, 0.2f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.5f}, 
                {EOrbitDistributionType::Dense, 0.05f}
            }
        },
        {
            EStellarClass::SubDwarf, 
            {
                {EOrbitDistributionType::Uniform, 0.35f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.10f}, 
                {EOrbitDistributionType::InnerOuter, 0.10f}, 
                {EOrbitDistributionType::Dense, 0.5f}
            }
        },
        {   
            EStellarClass::WhiteDwarf, 
            {
                {EOrbitDistributionType::Uniform, 0.40f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.10f}, 
                {EOrbitDistributionType::Dense, 0.5f}
            }
        },
        {   EStellarClass::BrownDwarf, 
            {
                {EOrbitDistributionType::Uniform, 0.45f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.05f}, 
                {EOrbitDistributionType::Dense, 0.1f}
            }
        },
        {
            EStellarClass::Neutron, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.03f},
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EStellarClass::Protostar, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EStellarClass::Pulsar, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {   
            EStellarClass::BlackHole, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 1.f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EStellarClass::Unknown, 
            {
                {EOrbitDistributionType::Uniform, 0.50f}, 
                {EOrbitDistributionType::Gaussian, 0.40f}, 
                {EOrbitDistributionType::Chaotic, 0.05f}, 
                {EOrbitDistributionType::InnerOuter, 0.03f}, 
                {EOrbitDistributionType::Dense, 0.02f}
            }
        }
    };

    const TMap<EStellarClass, int> BasePlanetCount = {
        {EStellarClass::HyperGiant, 15},
        {EStellarClass::SuperGiant, 10},
        {EStellarClass::BrightGiant, 10},
        {EStellarClass::Giant, 7},
        {EStellarClass::SubGiant, 4},
        {EStellarClass::MainSequence, 10},
        {EStellarClass::SubDwarf, 3},
        {EStellarClass::WhiteDwarf, 2},
        {EStellarClass::BrownDwarf, 0},
        {EStellarClass::Neutron, 1},
        {EStellarClass::Protostar, 2},
        {EStellarClass::Pulsar, 1},
        {EStellarClass::BlackHole, 0},
};

    const TMap<EStellarClass, PlanetProbability> BasePlanetProbabilities =
    {
        {EStellarClass::HyperGiant, 0.7f},
        {EStellarClass::SuperGiant, 0.7f},
        {EStellarClass::BrightGiant, 0.7f},
        {EStellarClass::Giant, 0.65f},
        {EStellarClass::SubGiant, 0.6f},
        {EStellarClass::MainSequence, 0.9f},
        {EStellarClass::SubDwarf, 0.3f},
        {EStellarClass::WhiteDwarf, 0.1f},
        {EStellarClass::BrownDwarf, 0.05f},
        {EStellarClass::Neutron, 0.01f},
        {EStellarClass::Protostar, 0.6f},
        {EStellarClass::Pulsar, 0.01f},
        {EStellarClass::BlackHole, 0.01f}
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
            {EPlanetType::IceGiant, 0.1f},
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
            {EPlanetType::Iron, 0.5f},
            {EPlanetType::Metallic, 0.5f},
            {EPlanetType::Volcanic, 0.3f},
            {EPlanetType::Greenhouse, 0.5f},
            {EPlanetType::Ammonia, 0.3f},

            //{EPlanetType::Forest, 0.4f}
        }},

        {EPlanetaryZoneType::HotZone,
        {
            {EPlanetType::Volcanic, 0.3f},
            {EPlanetType::Lava, 0.4f},
            {EPlanetType::Iron, 0.3f},
            {EPlanetType::Metallic, 0.3f},
            {EPlanetType::HotGiant, 0.1f},
            {EPlanetType::Melted, 0.5f}
        }},

        {EPlanetaryZoneType::InnerPlanetZone,
        {
            {EPlanetType::Rocky, 0.4f},
            {EPlanetType::Iron, 0.3f},
            {EPlanetType::Terrestrial, 0.3f}
        }},

        {EPlanetaryZoneType::OuterPlanetZone,
        {
            {EPlanetType::GasGiant, 0.5f},
            {EPlanetType::IceGiant, 0.5f}
        }},

        {EPlanetaryZoneType::AsteroidBeltZone,
        {
            {EPlanetType::Dwarf, 0.5f},
            //{EPlanetType::Asteroid, 0.5f}
        }},

        {EPlanetaryZoneType::KuiperBeltZone,
        {
            {EPlanetType::Dwarf, 0.7f},
            {EPlanetType::Ice, 0.3f}
        }},

        {EPlanetaryZoneType::GasGiantsZone,
        {
            {EPlanetType::GasGiant, 0.5f},
            {EPlanetType::IceGiant, 0.5f}
        }},
        // и так далее для каждой зоны...
    };

    /*TMap<EPlanetarySystemType, FPlanetarySystemRanges> PlanetarySystemRanges =
    {
        {EPlanetarySystemType::NoPlanetSystem, FPlanetarySystemRanges(TTuple<int, int>(0, 0), TTuple<double, double>(0.0, 0.0))},
        {EPlanetarySystemType::SmallSystem, FPlanetarySystemRanges(TTuple<int, int>(1, 5), TTuple<double, double>(1.0, 3.0))},
        {EPlanetarySystemType::LargeSystem, FPlanetarySystemRanges(TTuple<int, int>(6, 12), TTuple<double, double>(1.0, 5.0))},
        {EPlanetarySystemType::ChaoticSystem, FPlanetarySystemRanges(TTuple<int, int>(5, 12), TTuple<double, double>(0.1, 2.0))},
        {EPlanetarySystemType::DenseSystem, FPlanetarySystemRanges(TTuple<int, int>(3, 10), TTuple<double, double>(0.2, 1.0))}
    };*/
};
