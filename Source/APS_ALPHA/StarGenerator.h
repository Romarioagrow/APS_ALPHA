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

	FString CalculateNonMainSequenceStarAge(double StarMass);

	FString CalculateMainSequenceStarAge(double mass);

	EStellarClass GenerateStarClassByRandomWeights();

private:

	FString GetSpectralClassColor(ESpectralClass Class);

	FString GetSpectralTypeDescription(ESpectralType Type);

	FName GenerateFullSpectralClass(const FStarGenerationModel& StarModel);

	FName GenerateFullSpectralName(const FStarGenerationModel& StarModel);

	ESpectralType CalculateSpectralType(EStellarClass StellarType, double Luminosity);

	int CalculateSpectralSubclass(double StarTemperature, ESpectralClass SpectralClass);

	double CalculateLuminosityByMass(double Mass);

	double RandomFromRange(TTuple<double, double> range);

	double RandomMass(ESpectralClass SpectralClass);

	double RandomRadius(ESpectralClass SpectralClass);

	double CalculateSurfaceTemperature(double Luminosity, double Radius);

	ESpectralClass ChooseSpectralClassByStellarClass(EStellarClass StellarClass);


	ESpectralClass DetermineSpectralClassByTemperature(EStellarClass StarType, double Temperature);

	double CalculateLuminosity(double Radius, double SurfaceTemperature);

	double GenerateRandomTemperatureBySpectralClass(ESpectralClass SpectralClass);

	//double GenerateRandomLuminosity(EStellarClass StarType);
	//double GenerateRandomMass(EStellarClass StarType);
	//double GenerateRandomRadius(EStellarClass StarType);

private:

//O - класс: 30000 - 60000 К
//B - класс : 10000 - 30000 К
//A - класс : 7500 - 10000 К
//F - класс : 6000 - 7500 К
//G - класс : 5200 - 6000 К
//K - класс : 3700 - 5200 К
//M - класс : 2000 - 3700 К

	TMap<EStellarClass, int> StarTypeWeights =
	{
		{EStellarClass::HyperGiant, 1}, // Гипергиганты
		{EStellarClass::SuperGiant, 3}, // Супергиганты
		{EStellarClass::BrightGiant, 3}, // Белые карлики
		{EStellarClass::Giant, 5}, // Гиганты
		{EStellarClass::SubGiant, 5}, // Супергиганты
		{EStellarClass::MainSequence, 80}, // Главная последовательность
		{EStellarClass::SubDwarf, 5}, // Субкарлики
		{EStellarClass::WhiteDwarf, 2}, // Коричневые карлики
		{EStellarClass::BrownDwarf, 2}, // Коричневые карлики
		{EStellarClass::Protostar, 1}, // Протозвезды
		{EStellarClass::Neutron, 1}, // Нейтронные звезды
		{EStellarClass::Pulsar, 1}, // Пульсары
		{EStellarClass::BlackHole, 1} // Черные дыры
	};

	// Создаем TMap для цветов спектральных классов.
	TMap<ESpectralClass, FString> SpectralClassColorMap =
	{
		{ESpectralClass::O, TEXT("Blue")},
		{ESpectralClass::B, TEXT("Blue-White")},
		{ESpectralClass::A, TEXT("White")},
		{ESpectralClass::F, TEXT("Yellow-White")},
		{ESpectralClass::G, TEXT("Yellow")},
		{ESpectralClass::K, TEXT("Orange")},
		{ESpectralClass::M, TEXT("Red")},
		{ESpectralClass::L, TEXT("Red-Brown")},
		{ESpectralClass::T, TEXT("Magenta")},
		{ESpectralClass::Y, TEXT("Cool-Brown")},
		{ESpectralClass::Unknown, TEXT("Unknown ")},

	};

	// Создаем TMap для описания спектральных типов.
	TMap<ESpectralType, FString> SpectralTypeDescriptionMap =
	{
		{ESpectralType::O, TEXT("Hypergiant")},
		{ESpectralType::Ia, TEXT("Luminous Supergiant")},
		{ESpectralType::Iab, TEXT("Normal Supergiant")},
		{ESpectralType::Ib, TEXT("Supergiant")},
		{ESpectralType::II, TEXT("Bright Giant")},
		{ESpectralType::III, TEXT("Giant")},
		{ESpectralType::IV, TEXT("Subgiant")},
		{ESpectralType::V, TEXT("Dwarf Main-Sequence")},
		{ESpectralType::VI, TEXT("Subdwarf")},
		{ESpectralType::VII, TEXT("White-Dwarf")},
		{ESpectralType::VIII, TEXT("Brown Dwarf")},
		{ESpectralType::Unknown, TEXT("Unknown ")}
		// Добавьте остальные типы, если они есть.
	};

	TMap<ESpectralClass, TTuple<double, double>> MainSequenceMassRanges =
	{
		{ESpectralClass::O, TTuple<double, double>(16, 60)},
		{ESpectralClass::B, TTuple<double, double>(2.1, 16)},
		{ESpectralClass::A, TTuple<double, double>(1.5, 3)},
		{ESpectralClass::F, TTuple<double, double>(1.04, 1.6)},
		{ESpectralClass::G, TTuple<double, double>(0.8, 1.04)},
		{ESpectralClass::K, TTuple<double, double>(0.45, 0.8)},
		{ESpectralClass::M, TTuple<double, double>(0.08, 0.45)},
	};

	TMap<ESpectralClass, TTuple<double, double>> MainSequenceRadiusRanges =
	{
		{ESpectralClass::O, TTuple<double, double>(6.6, 10)},
		{ESpectralClass::B, TTuple<double, double>(1.8, 6.6)},
		{ESpectralClass::A, TTuple<double, double>(1.4, 2.5)},
		{ESpectralClass::F, TTuple<double, double>(1.15, 1.4)},
		{ESpectralClass::G, TTuple<double, double>(0.96, 1.15)},
		{ESpectralClass::K, TTuple<double, double>(0.7, 0.96)},
		{ESpectralClass::M, TTuple<double, double>(0.3, 0.7)},
	};

	TMap<ESpectralClass, TTuple<double, double>> StarTypeTemperatureRanges =
	{
		{ESpectralClass::O, TTuple<double, double>(30000, 60000)},
		{ESpectralClass::B, TTuple<double, double>(10000, 30000)},
		{ESpectralClass::A, TTuple<double, double>(7500, 10000)},
		{ESpectralClass::F, TTuple<double, double>(6000, 7500)},
		{ESpectralClass::G, TTuple<double, double>(5000, 6000)},
		{ESpectralClass::K, TTuple<double, double>(3800, 5000)},
		{ESpectralClass::M, TTuple<double, double>(2500, 3800)},
		{ESpectralClass::L, TTuple<double, double>(1300, 2400)},
		{ESpectralClass::T, TTuple<double, double>(700, 1300)},
		{ESpectralClass::Y, TTuple<double, double>(350, 700)},
		{ESpectralClass::NS, TTuple<double, double>(1000000, 1000000)},
		{ESpectralClass::PS, TTuple<double, double>(2000, 3000)},
		{ESpectralClass::BH, TTuple<double, double>(0, 0)}, // Black holes do not have a definite temperature
		{ESpectralClass::Unknown, TTuple<double, double>(0, 0)} // Black holes do not have a definite temperature
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
		{EStellarClass::HyperGiant, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::SuperGiant, FStarAttributeRanges(
			{FLuminosityRange({50000.0, 90000.0}), FMassRange({15, 100}), FRadiusRange({30, 500}), FAgeRange({1.0e6, 1.0e7}), FAbsoluteMagnitudeRange({-4.0, -8.0})})},
		{EStellarClass::BrightGiant, FStarAttributeRanges(
			{FLuminosityRange({650, 5000}), FMassRange({15, 70}), FRadiusRange({20, 100}), FAgeRange({1.0e10, 1.0e12}), FAbsoluteMagnitudeRange({-2.0, -5.0})})},
		{EStellarClass::Giant, FStarAttributeRanges(
			{FLuminosityRange({10.0, 1000.0}), FMassRange({10, 500}), FRadiusRange({10.0, 50}), FAgeRange({1.0e8, 1.0e9}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::SubGiant, FStarAttributeRanges(
			{FLuminosityRange({5.0, 100.0}), FMassRange({5, 10}), FRadiusRange({2, 10}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-4.7, 3.2})})},
		{EStellarClass::MainSequence, FStarAttributeRanges(
			{FLuminosityRange({0.00001, 100000.0}), FMassRange({0.1, 120}), FRadiusRange({0.1, 120}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::SubDwarf, FStarAttributeRanges(
			{FLuminosityRange({10, 100.0}), FMassRange({0.5, 2}), FRadiusRange({0.5, 2}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::WhiteDwarf, FStarAttributeRanges(
			{FLuminosityRange({0.00001, 0.01}), FMassRange({0.6, 1.4}), FRadiusRange({0.01, 0.2}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::BrownDwarf, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({0.005, 0.075}), FRadiusRange({0.01, 0.1}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::Neutron, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::Protostar, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({0.1, 150}), FRadiusRange({10, 20}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::Pulsar, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({2, 200}), FRadiusRange({0.0001, 0.001}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarClass::BlackHole, FStarAttributeRanges(
			{FLuminosityRange({0.0, 0.0}), FMassRange({3, 100}), FRadiusRange({0.001, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		// остальные классы звезд...
	};
};
