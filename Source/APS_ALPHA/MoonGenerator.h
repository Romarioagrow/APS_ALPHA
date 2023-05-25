// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Moon.h"
#include "MoonGenerationModel.h"
#include "PlanetGenerationModel.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "MoonGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UMoonGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UMoonGenerator();

public:
	// Функция генерации луны
	//AMoon* GenerateMoon(); // FMoonParams Params

    double CalculateGravitationalForce(double MassPlanet, double MassMoon, double Distance);

    double CalculateRandomMoonDensity(EMoonType MoonType);

	void ApplyModel(AMoon* Moon, FMoonGenerationModel MoonGenerationModel);

	FMoonGenerationModel GenerateMoonModel();

	FMoonGenerationModel GenerateRandomMoonModel();

	EMoonType GenerateMoonType(FPlanetGenerationModel PlanetModel);

	double CalculateMoonMass(EMoonType MoonType);

    double CalculateRandomMoonMass();

	double CalculateMoonRadius(double MoonDensity, double MoonMass);

	// override;


public:

    TMap<EMoonType, TPair<double, double>> MoonDensityRanges = 
    {
        {EMoonType::Rocky, {2.5, 3.5}}, // кг/см^3
        {EMoonType::Icy, {0.9, 1.2}},
        {EMoonType::Iron, {5.8, 7.9}},
        {EMoonType::Volcanic, {2.5, 3.0}},
        {EMoonType::Gas, {0.1, 1.0}},
        {EMoonType::Ocean, {1.0, 1.5}},
        {EMoonType::Continental, {2.0, 2.7}},
        {EMoonType::Desert, {2.0, 3.0}},
        {EMoonType::TidallyLocked, {1.0, 3.5}}, // Это будет зависеть от состава луны
        {EMoonType::Peculiar, {0.1, 5.9}}, // Может быть любым, в зависимости от особенностей
        {EMoonType::CapturedAsteroid, {2.0, 3.0}},
        {EMoonType::Unknown, {0.1, 7.9}} // Может быть любым
    };

    /*const TMap<EPlanetType, TMap<EMoonType, float>> StellarOrbitDistributions =
    {
        {
            EPlanetType::,
            {
                {EOrbitDistributionType::Uniform, 0.05f},
                {EOrbitDistributionType::Gaussian, 0.10f},
                {EOrbitDistributionType::Chaotic, 0.50f},
                {EOrbitDistributionType::InnerOuter, 0.25f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EPlanetType::SuperGiant,
            {
                {EOrbitDistributionType::Uniform, 0.10f},
                {EOrbitDistributionType::Gaussian, 0.20f},
                {EOrbitDistributionType::Chaotic, 0.40f},
                {EOrbitDistributionType::InnerOuter, 0.20f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EPlanetType::BrightGiant,
            {
                {EOrbitDistributionType::Uniform, 0.15f},
                {EOrbitDistributionType::Gaussian, 0.25f},
                {EOrbitDistributionType::Chaotic, 0.30f},
                {EOrbitDistributionType::InnerOuter, 0.20f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EPlanetType::Giant,
            {
                {EOrbitDistributionType::Uniform, 0.20f},
                {EOrbitDistributionType::Gaussian, 0.30f},
                {EOrbitDistributionType::Chaotic, 0.25f},
                {EOrbitDistributionType::InnerOuter, 0.15f},
                {EOrbitDistributionType::Dense, 0.10f}
            }
        },
        {
            EPlanetType::SubGiant,
            {
                {EOrbitDistributionType::Uniform, 0.25f},
                {EOrbitDistributionType::Gaussian, 0.35f},
                {EOrbitDistributionType::Chaotic, 0.20f},
                {EOrbitDistributionType::InnerOuter, 0.10f},
                {EOrbitDistributionType::Dense, 0.50f}
            }
        },
        {
            EPlanetType::MainSequence,
            {
                {EOrbitDistributionType::Uniform, 0.3f},
                {EOrbitDistributionType::Gaussian, 0.2f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.5f},
                {EOrbitDistributionType::Dense, 0.05f}
            }
        },
        {
            EPlanetType::SubDwarf,
            {
                {EOrbitDistributionType::Uniform, 0.35f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.10f},
                {EOrbitDistributionType::InnerOuter, 0.10f},
                {EOrbitDistributionType::Dense, 0.5f}
            }
        },
        {
            EPlanetType::WhiteDwarf,
            {
                {EOrbitDistributionType::Uniform, 0.40f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.10f},
                {EOrbitDistributionType::Dense, 0.5f}
            }
        },
        {   EPlanetType::BrownDwarf,
            {
                {EOrbitDistributionType::Uniform, 0.45f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.05f},
                {EOrbitDistributionType::Dense, 0.1f}
            }
        },
        {
            EPlanetType::Neutron,
            {
                {EOrbitDistributionType::Uniform, 0.50f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.03f},
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EPlanetType::Protostar,
            {
                {EOrbitDistributionType::Uniform, 0.50f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.03f},
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EPlanetType::Pulsar,
            {
                {EOrbitDistributionType::Uniform, 0.50f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.03f},
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EPlanetType::BlackHole,
            {
                {EOrbitDistributionType::Uniform, 0.50f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 1.f},
                {EOrbitDistributionType::InnerOuter, 0.03f},
                {EOrbitDistributionType::Dense, 0.02f}
            }
        },
        {
            EPlanetType::Unknown,
            {
                {EOrbitDistributionType::Uniform, 0.50f},
                {EOrbitDistributionType::Gaussian, 0.40f},
                {EOrbitDistributionType::Chaotic, 0.05f},
                {EOrbitDistributionType::InnerOuter, 0.03f},
                {EOrbitDistributionType::Dense, 0.02f}
            }
        }
    };*/

};

