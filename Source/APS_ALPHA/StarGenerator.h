// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Star.h"
#include "StarGenerationModel.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "StarGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UStarGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UStarGenerator();

public:
	// Функция генерации звезды
	void ApplyModel(AStar* NewStar, FStarGenerationModel StarModel);

	FStarGenerationModel GenerateRandomStarModel();

	//EStellarClass ChooseRandomStellarClass();

	ESpectralClass ChooseSpectralClassByStellarClass(EStellarClass StellarClass);

private:
	ESpectralClass DetermineSpectralClassByTemperature(EStellarClass StarType, double Temperature);

	double GenerateRandomTemperatureBySpectralClass(ESpectralClass SpectralClass);

	//double GenerateRandomTemperatureBySpectralClass(EStellarClass StarType);

	double GenerateRandomLuminosity(EStellarClass StarType);

	double GenerateRandomMass(EStellarClass StarType);

	double GenerateRandomRadius(EStellarClass StarType);

private:
	struct FStarAttributeRanges
	{
		TTuple<ESpectralClass, ESpectralClass> SpectralClassRange;
		TTuple<double, double> Luminosity;
		TTuple<double, double> Mass;
		TTuple<double, double> Radius;
		TTuple<double, double> Age;
		TTuple<double, double> AbsoluteMagnitude;

		FStarAttributeRanges(
			TTuple<ESpectralClass, ESpectralClass> SpectralClassRange,
			TTuple<double, double> Luminosity, 
			TTuple<double, double> Mass, 
			TTuple<double, double> Radius, 
			TTuple<double, double> Age, 
			TTuple<double, double> AbsoluteMagnitude

		)
			: SpectralClassRange(SpectralClassRange), Luminosity(Luminosity), Mass(Mass), Radius(Radius), Age(Age), AbsoluteMagnitude(AbsoluteMagnitude)
		{}
	};
	TMap<EStellarClass, FStarAttributeRanges> StarAttributeRanges =
	{
		{EStellarClass::HyperGiant, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {90000.0, 100000.0}, {100, 250}, {30, 1000}, {1.0e5, 1.0e6}, {-8.0, -10.0})}, // Hypergiant stars
		{EStellarClass::SuperGiant, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {50000.0, 90000.0}, {8, 10}, {20, 500}, {1.0e6, 1.0e7}, {-4.0, -8.0})}, // Supergiant stars
		{EStellarClass::BrightGiant, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {600, 5000}, {5, 8}, {10, 20}, {1.0e10, 1.0e12}, {-2.0, -5.0})}, // Red Dwarf stars
		{EStellarClass::Giant, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {10.0, 1000.0}, {0.5, 10.0}, {10.0, 100.0}, {1.0e8, 1.0e9}, {0.0, 0.0})}, // Giant stars
		{EStellarClass::SubGiant, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {10.0, 1000.0}, {0.5, 10.0}, {10.0, 100.0}, {1.0e8, 1.0e9}, {0.0, 0.0})}, // Giant stars
		{EStellarClass::MainSequence, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {0.01, 100.0}, {0.08, 100.0}, {0.08, 10.0}, {1.0e7, 1.0e10}, {0.0, 0.0})}, // Main Sequence stars
		{EStellarClass::SubDwarf, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {0.01, 1.0}, {0.1, 0.8}, {0.1, 0.8}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // Subdwarf stars
		{EStellarClass::WhiteDwarf, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {0.0001, 0.01}, {0.5, 1.4}, {0.008, 0.02}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // White Dwarf stars
		{EStellarClass::BrownDwarf, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {0.000001, 0.01}, {0.01, 0.08}, {0.01, 0.1}, {1.0e6, 1.0e9}, {0.0, 0.0})}, // Brown Dwarf stars
		{EStellarClass::Neutron, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {0.0000001, 0.00001}, {1.4, 3.0}, {0.00001, 0.00002}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // Neutron stars
		{EStellarClass::Protostar, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {0.1, 1.0}, {0.01, 1.0}, {1.0, 10.0}, {1.0e4, 1.0e6}, {0.0, 0.0})}, // Protostar stars
		{EStellarClass::Pulsar, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {100000.0, 1000000.0}, {1.4, 3.0}, {0.00001, 0.00002}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // Pulsar stars
		{EStellarClass::BlackHole, FStarAttributeRanges({ESpectralClass::A, ESpectralClass::M}, {0.0, 0.0}, {5.0, 100000.0}, {0.01, 10}, { 1.0e9, 1.0e11}, {0.0, 0.0})} // Black Hole stars
	};
};
