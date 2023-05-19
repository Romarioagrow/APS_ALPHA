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

private:
	EStarSpectralClass DetermineSpectralClass(EStarType StarType, double Temperature);

	double GenerateRandomTemperature(EStarType StarType);

	double GenerateRandomLuminosity(EStarType StarType);

	double GenerateRandomMass(EStarType StarType);

	double GenerateRandomRadius(EStarType StarType);

private:
	struct FStarAttributeRanges
	{
		TTuple<double, double> Luminosity;
		TTuple<double, double> Mass;
		TTuple<double, double> Radius;
		TTuple<double, double> Age;

		FStarAttributeRanges(TTuple<double, double> Luminosity, TTuple<double, double> Mass, TTuple<double, double> Radius, TTuple<double, double> Age)
			: Luminosity(Luminosity), Mass(Mass), Radius(Radius), Age(Age)
		{}
	};
	TMap<EStarType, FStarAttributeRanges> StarAttributeRanges =
	{
		{EStarType::MainSequence, FStarAttributeRanges({0.01, 100.0}, {0.08, 100.0}, {0.08, 10.0}, {1.0e7, 1.0e10})}, // Main Sequence stars
		{EStarType::Giant, FStarAttributeRanges({10.0, 1000.0}, {0.5, 10.0}, {10.0, 100.0}, {1.0e8, 1.0e9})}, // Giant stars
		{EStarType::SuperGiant, FStarAttributeRanges({1000.0, 500000.0}, {10.0, 100.0}, {100.0, 1000.0}, {1.0e6, 1.0e7})}, // Supergiant stars
		{EStarType::WhiteDwarf, FStarAttributeRanges({0.0001, 0.01}, {0.5, 1.4}, {0.008, 0.02}, {1.0e9, 1.0e10})}, // White Dwarf stars
		{EStarType::Neutron, FStarAttributeRanges({0.0000001, 0.00001}, {1.4, 3.0}, {0.00001, 0.00002}, {1.0e9, 1.0e10})}, // Neutron stars
		{EStarType::BrownDwarf, FStarAttributeRanges({0.000001, 0.01}, {0.01, 0.08}, {0.01, 0.1}, {1.0e6, 1.0e9})}, // Brown Dwarf stars
		{EStarType::RedDwarf, FStarAttributeRanges({0.01, 0.1}, {0.08, 0.45}, {0.1, 0.5}, {1.0e10, 1.0e12})}, // Red Dwarf stars
		{EStarType::SubDwarf, FStarAttributeRanges({0.01, 1.0}, {0.1, 0.8}, {0.1, 0.8}, {1.0e9, 1.0e10})}, // Subdwarf stars
		{EStarType::HyperGiant, FStarAttributeRanges({500000.0, 1000000.0}, {20.0, 120.0}, {1000.0, 2000.0}, {1.0e5, 1.0e6})}, // Hypergiant stars
		{EStarType::Protostar, FStarAttributeRanges({0.1, 1.0}, {0.01, 1.0}, {1.0, 10.0}, {1.0e4, 1.0e6})}, // Protostar stars
		{EStarType::Pulsar, FStarAttributeRanges({100000.0, 1000000.0}, {1.4, 3.0}, {0.00001, 0.00002}, {1.0e9, 1.0e10})}, // Pulsar stars
		{EStarType::BlackHole, FStarAttributeRanges({0.0, 0.0}, {5.0, 100000.0}, {0.01, 10}, { 1.0e9, 1.0e11})} // Black Hole stars
	};
};
