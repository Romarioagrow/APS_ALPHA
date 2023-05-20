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

	double RandomFromRange(TTuple<double, double> range);

	double RandomMass(ESpectralClass SpectralClass);

	double RandomRadius(ESpectralClass SpectralClass);

	double CalculateSurfaceTemperature(double Luminosity, double Radius);

	//EStellarClass ChooseRandomStellarClass();

	ESpectralClass ChooseSpectralClassByStellarClass(EStellarClass StellarClass);

private:
	ESpectralClass DetermineSpectralClassByTemperature(EStellarClass StarType, double Temperature);

	double CalculateLuminosity(double Radius, double SurfaceTemperature);

	double GenerateRandomTemperatureBySpectralClass(ESpectralClass SpectralClass);

	//double GenerateRandomTemperatureBySpectralClass(EStellarClass StarType);

	double GenerateRandomLuminosity(EStellarClass StarType);

	double GenerateRandomMass(EStellarClass StarType);

	double GenerateRandomRadius(EStellarClass StarType);

private:

//O - класс: 30000 - 60000 К
//B - класс : 10000 - 30000 К
//A - класс : 7500 - 10000 К
//F - класс : 6000 - 7500 К
//G - класс : 5200 - 6000 К
//K - класс : 3700 - 5200 К
//M - класс : 2000 - 3700 К

	TMap<ESpectralClass, TTuple<double, double>> MainSequenceMassRanges =
	{
		{ESpectralClass::O, TTuple<double, double>(16, 60)},
		{ESpectralClass::B, TTuple<double, double>(2.1, 16)},
		{ESpectralClass::A, TTuple<double, double>(1.4, 2.1)},
		{ESpectralClass::F, TTuple<double, double>(1.04, 1.4)},
		{ESpectralClass::G, TTuple<double, double>(0.8, 1.04)},
		{ESpectralClass::K, TTuple<double, double>(0.45, 0.8)},
		{ESpectralClass::M, TTuple<double, double>(0.08, 0.45)},
	};

	TMap<ESpectralClass, TTuple<double, double>> MainSequenceRadiusRanges =
	{
		{ESpectralClass::O, TTuple<double, double>(6.6, 10)},
		{ESpectralClass::B, TTuple<double, double>(1.8, 6.6)},
		{ESpectralClass::A, TTuple<double, double>(1.4, 1.8)},
		{ESpectralClass::F, TTuple<double, double>(1.15, 1.4)},
		{ESpectralClass::G, TTuple<double, double>(0.96, 1.15)},
		{ESpectralClass::K, TTuple<double, double>(0.7, 0.96)},
		{ESpectralClass::M, TTuple<double, double>(0.3, 0.7)},
	};

	TMap<ESpectralClass, TTuple<double, double>> StarTypeTemperatureRanges =
	{

		//30 

		{ESpectralClass::O, TTuple<double, double>(30000, 60000)},
		{ESpectralClass::B, TTuple<double, double>(10000, 30000)},
		{ESpectralClass::A, TTuple<double, double>(7500, 10000)},
		{ESpectralClass::F, TTuple<double, double>(6000, 7500)},
		{ESpectralClass::G, TTuple<double, double>(5200, 6000)},
		{ESpectralClass::K, TTuple<double, double>(250, 1300)},
		{ESpectralClass::M, TTuple<double, double>(3700, 5200)},
		{ESpectralClass::L, TTuple<double, double>(1300, 2400)},
		{ESpectralClass::T, TTuple<double, double>(700, 1300)},
		{ESpectralClass::Y, TTuple<double, double>(350, 700)},
		{ESpectralClass::NS, TTuple<double, double>(1000000, 1000000)},
		{ESpectralClass::BH, TTuple<double, double>(0, 0)} // Black holes do not have a definite temperature
	};

	struct FLuminosityRange
	{
		TTuple<double, double> Range;
		FLuminosityRange(TTuple<double, double> range) : Range(range) {}
	};

	struct FMassRange
	{
		TTuple<double, double> Range;
		FMassRange(TTuple<double, double> range) : Range(range) {}
	};

	struct FRadiusRange
	{
		TTuple<double, double> Range;
		FRadiusRange(TTuple<double, double> range) : Range(range) {}
	};

	struct FAgeRange
	{
		TTuple<double, double> Range;
		FAgeRange(TTuple<double, double> range) : Range(range) {}
	};

	struct FAbsoluteMagnitudeRange
	{
		TTuple<double, double> Range;
		FAbsoluteMagnitudeRange(TTuple<double, double> range) : Range(range) {}
	};

	struct FStarAttributeRanges
	{
		FLuminosityRange Luminosity;
		FMassRange Mass;
		FRadiusRange Radius;
		FAgeRange Age;
		FAbsoluteMagnitudeRange AbsoluteMagnitude;

		FStarAttributeRanges(FLuminosityRange luminosity, FMassRange mass, FRadiusRange radius, FAgeRange age, FAbsoluteMagnitudeRange absoluteMagnitude)
			: Luminosity(luminosity), Mass(mass), Radius(radius), Age(age), AbsoluteMagnitude(absoluteMagnitude) {}
	};

	TMap<EStellarClass, FStarAttributeRanges> StarAttributeRanges =
	{
		{EStellarClass::HyperGiant, FStarAttributeRanges({FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::SuperGiant, FStarAttributeRanges({FLuminosityRange({50000.0, 90000.0}), FMassRange({15, 100}), FRadiusRange({30, 500}), FAgeRange({1.0e6, 1.0e7}), FAbsoluteMagnitudeRange({-4.0, -8.0})})},
		{EStellarClass::BrightGiant, FStarAttributeRanges({FLuminosityRange({650, 5000}), FMassRange({15, 70}), FRadiusRange({20, 100}), FAgeRange({1.0e10, 1.0e12}), FAbsoluteMagnitudeRange({-2.0, -5.0})})},
		{EStellarClass::Giant, FStarAttributeRanges({FLuminosityRange({10.0, 1000.0}), FMassRange({10, 500}), FRadiusRange({10.0, 50}), FAgeRange({1.0e8, 1.0e9}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::SubGiant, FStarAttributeRanges({FLuminosityRange({5.0, 100.0}), FMassRange({5, 10}), FRadiusRange({2, 10}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-4.7, 3.2})})},
		{EStellarClass::MainSequence, FStarAttributeRanges({FLuminosityRange({0.00001, 100000.0}), FMassRange({0.1, 120}), FRadiusRange({0.1, 120}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::SubDwarf, FStarAttributeRanges({FLuminosityRange({10, 100.0}), FMassRange({0.5, 2}), FRadiusRange({0.5, 2}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::WhiteDwarf, FStarAttributeRanges({FLuminosityRange({0.00001, 0.01}), FMassRange({0.6, 1.4}), FRadiusRange({0.01, 0.2}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::BrownDwarf, FStarAttributeRanges({FLuminosityRange({90000.0, 100000.0}), FMassRange({0.005, 0.075}), FRadiusRange({0.01, 0.1}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::Neutron, FStarAttributeRanges({FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::Protostar, FStarAttributeRanges({FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::Pulsar, FStarAttributeRanges({FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::BlackHole, FStarAttributeRanges({FLuminosityRange({0.0, 0.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		// остальные классы звезд...
	};


	//	{EStellarClass::HyperGiant, FStarAttributeRanges({90000.0, 100000.0}, {100, 250}, {40, 1000}, {1.0e5, 1.0e6}, {-8.0, -10.0})}, // Hypergiant stars
	//	{EStellarClass::SuperGiant, FStarAttributeRanges({50000.0, 90000.0}, {15, 100}, {30, 500}, {1.0e6, 1.0e7}, {-4.0, -8.0})}, // Supergiant stars
	//	{EStellarClass::BrightGiant, FStarAttributeRanges({650, 5000}, {15, 70}, {20, 100}, {1.0e10, 1.0e12}, {-2.0, -5.0})}, // Red Dwarf stars
	//	{EStellarClass::Giant, FStarAttributeRanges({10.0, 1000.0}, {10, 50}, {10.0, 50}, {1.0e8, 1.0e9}, {0.0, 0.0})}, // Giant stars
	//	{EStellarClass::SubGiant, FStarAttributeRanges({5.0, 100.0}, {5, 10.0}, {2, 10}, {1.0e8, 1.0e9}, {-4.7, 3.2})}, // Giant stars
	//	{EStellarClass::MainSequence, FStarAttributeRanges({0.01, 100.0}, {0.1, 120}, {0.08, 10.0}, {1.0e7, 1.0e10}, {0.0, 0.0})}, // Main Sequence stars
	//	{EStellarClass::SubDwarf, FStarAttributeRanges({0.00005, 1.0}, {0.1, 0.8}, {0.1, 0.8}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // Subdwarf stars
	//	{EStellarClass::WhiteDwarf, FStarAttributeRanges({0.0001, 0.01}, {0.5, 1.4}, {0.008, 0.02}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // White Dwarf stars
	//	{EStellarClass::BrownDwarf, FStarAttributeRanges({0.000001, 0.01}, {0.01, 0.08}, {0.01, 0.1}, {1.0e6, 1.0e9}, {0.0, 0.0})}, // Brown Dwarf stars
	//	{EStellarClass::Neutron, FStarAttributeRanges({0.0000001, 0.00001}, {1.4, 3.0}, {0.00001, 0.00002}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // Neutron stars
	//	{EStellarClass::Protostar, FStarAttributeRanges({0.1, 1.0}, {0.01, 1.0}, {1.0, 10.0}, {1.0e4, 1.0e6}, {0.0, 0.0})}, // Protostar stars
	//	{EStellarClass::Pulsar, FStarAttributeRanges({100000.0, 1000000.0}, {1.4, 3.0}, {0.00001, 0.00002}, {1.0e9, 1.0e10}, {0.0, 0.0})}, // Pulsar stars
	//	{EStellarClass::BlackHole, FStarAttributeRanges({0.0, 0.0}, {5.0, 100000.0}, {0.01, 10}, { 1.0e9, 1.0e11}, {0.0, 0.0})} // Black Hole stars

};
