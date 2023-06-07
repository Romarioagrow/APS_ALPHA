// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Star.h"
#include "StarGenerationModel.h"
#include "StarClusterType.h"
#include "StarClusterPopulation.h"
#include "StarCluster.h"

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
	// Базовая светимость Солнца.
	const double SolarEmissiveLuminosity = 1.0;
	// Минимальная и максимальная светимости для звезд.
	const double MinStarLuminosity = 100;
	const double MaxStarLuminosity = 500;

public:
	void ApplySpectralMaterial(AStar* NewStar, FStarModel StarModel);

	FStarModel GenerateStarModelByProbability(FStarClusterModel FStarClusterModel);

	double WienLaw(double temperature);

	FLinearColor WavelengthToRGB(double wavelength);

	FLinearColor TemperatureToColor(double temperature);

	FLinearColor GetStarColor(ESpectralClass spectralClass, int subclass);

	FLinearColor TemperatureToRGB(float Temperature);

	void ApplyModel(AStar* NewStar, FStarModel StarModel);

	FStarModel GenerateStarModel(FStarModel StarModel);

	FStarModel GenerateRandomStarModel();

	FString CalculateNonMainSequenceStarAge(double StarMass);

	FString CalculateMainSequenceStarAge(double mass);

	ESpectralClass GenerateSpectralClassByProbability(TMap<ESpectralClass, int> StarSpectralClassProbabilities);

	EStellarType GenerateStellarTypeByRandomWeights(TMap<EStellarType, int> StarTypeProbabilities);

	EStellarType GenerateStellarTypeByRandomWeights();

	// Функция для вычисления эмиссии звезды.
	double CalculateEmission(float starLuminosity);// {

private:
	FString GetSpectralClassColor(ESpectralClass Class);

	FString GetSpectralTypeDescription(ESpectralType Type);

	FName GenerateFullSpectralClass(const FStarModel& StarModel);

	FName GenerateFullSpectralName(const FStarModel& StarModel);

	ESpectralType CalculateSpectralType(EStellarType StellarType, double Luminosity);

	int CalculateSpectralSubclass(double StarTemperature, ESpectralClass SpectralClass);

	double CalculateLuminosityByMass(double Mass);

	double RandomFromRange(TTuple<double, double> range);

	double RandomMass(ESpectralClass SpectralClass);

	double RandomRadius(ESpectralClass SpectralClass);

	double CalculateSurfaceTemperature(double Luminosity, double Radius);

	//ESpectralClass ChooseSpectralClassByProbability(TMap<ESpectralClass, int> Probabilities);

	ESpectralClass ChooseSpectralClassByStellarClass(EStellarType StellarClass);

	ESpectralClass DetermineSpectralClassByTemperature(EStellarType StarType, double Temperature);

	double CalculateLuminosity(double Radius, double SurfaceTemperature);

	double GenerateRandomTemperatureBySpectralClass(ESpectralClass SpectralClass);

//O - класс: 30000 - 60000 К
//B - класс : 10000 - 30000 К
//A - класс : 7500 - 10000 К
//F - класс : 6000 - 7500 К
//G - класс : 5200 - 6000 К
//K - класс : 3700 - 5200 К
//M - класс : 2000 - 3700 К
private:
	const TMap<EStarClusterComposition, TMap<ESpectralClass, int>> StarClusterCompositionWeights =
	{
		{
			EStarClusterComposition::AllSpectral,
			{
				{ESpectralClass::O, 1}, // O-класс, очень горячие и яркие звезды. Очень редкие.
				{ESpectralClass::B, 4}, // B-класс, горячие, яркие звезды. Редкие.
				{ESpectralClass::A, 6}, // A-класс, белые или голубые звезды. Не очень распространены.
				{ESpectralClass::F, 15}, // F-класс, белые звезды. Умеренно распространены.
				{ESpectralClass::G, 22}, // G-класс, включая Солнце. Довольно распространены.
				{ESpectralClass::K, 25}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::M, 76}, // M-класс, красные звезды. Самый распространенный класс.
				{ESpectralClass::L, 5}, // L-класс, коричневые карлики. Редкие.
				{ESpectralClass::T, 4}, // T-класс, еще более холодные коричневые карлики. Редкие.
				{ESpectralClass::Y, 2}, // Y-класс, самые холодные объекты, считающиеся звездами. Очень редкие.
				{ESpectralClass::BH, 1}, // Black holes. Extremely rare.
				{ESpectralClass::NS, 1}, // Neutron stars. Extremely rare.
				{ESpectralClass::PS, 1}, // Pulsars. Extremely rare.
			}
		},
		{
			EStarClusterComposition::OnlyBlue,
			{
				{ESpectralClass::O, 90}, // O-класс, очень горячие и яркие звезды. Очень редкие.
				{ESpectralClass::B, 10}, // B-класс, горячие, яркие звезды. Редкие.
			}
		},
		{
			EStarClusterComposition::MostlyBlue,
			{
				{ESpectralClass::O, 50}, // O-класс, очень горячие и яркие звезды. Очень редкие.
				{ESpectralClass::B, 50}, // B-класс, горячие, яркие звезды. Редкие.
				{ESpectralClass::A, 5}, // A-класс, белые или голубые звезды. Не очень распространены.
				{ESpectralClass::F, 5}, // F-класс, белые звезды. Умеренно распространены.
				{ESpectralClass::G, 3}, // G-класс, включая Солнце. Довольно распространены.
				{ESpectralClass::K, 2}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::M, 1},
			}
		},
		{
			EStarClusterComposition::BlueWhite,
			{
				{ESpectralClass::O, 50}, // O-класс, очень горячие и яркие звезды. Очень редкие.
				{ESpectralClass::B, 50}, // B-класс, горячие, яркие звезды. Редкие.
				{ESpectralClass::A, 50}, // A-класс, белые или голубые звезды. Не очень распространены.
				{ESpectralClass::F, 50}, // F-класс, белые звезды. Умеренно распространены.
			}
		},
		{
			EStarClusterComposition::OnlyWhite,
			{
				{ESpectralClass::A, 50}, // A-класс, белые или голубые звезды. Не очень распространены.
				{ESpectralClass::F, 50}, // F-класс, белые звезды. Умеренно распространены.
			}
		},
		{
			EStarClusterComposition::MostlyWhite,
			{
				{ESpectralClass::A, 50}, // A-класс, белые или голубые звезды. Не очень распространены.
				{ESpectralClass::F, 50}, // F-класс, белые звезды. Умеренно распространены.
				{ESpectralClass::O, 5}, // O-класс, очень горячие и яркие звезды. Очень редкие.
				{ESpectralClass::B, 5},
				{ESpectralClass::G, 3}, // G-класс, включая Солнце. Довольно распространены.
				{ESpectralClass::K, 2}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::M, 1},
			}
		},
		{
			EStarClusterComposition::WhiteYellow,
			{
				{ESpectralClass::A, 50}, // A-класс, белые или голубые звезды. Не очень распространены.
				{ESpectralClass::F, 50}, // F-класс, белые звезды. Умеренно распространены.
				{ESpectralClass::G, 50}, // G-класс, включая Солнце. Довольно распространены.
			}
		},
		{
			EStarClusterComposition::OnlyYellow,
			{
				{ESpectralClass::F, 5}, // G-класс, включая Солнце. Довольно распространены.
				{ESpectralClass::G, 90}, // G-класс, включая Солнце. Довольно распространены.
				{ESpectralClass::K, 5}, // G-класс, включая Солнце. Довольно распространены.
			}
		},
		{
			EStarClusterComposition::MostlyYellow,
			{
				{ESpectralClass::A, 5}, // A-класс, белые или голубые звезды. Не очень распространены.
				{ESpectralClass::O, 5}, // O-класс, очень горячие и яркие звезды. Очень редкие.
				{ESpectralClass::B, 5},
				{ESpectralClass::F, 10}, // F-класс, белые звезды. Умеренно распространены.
				{ESpectralClass::G, 80}, // G-класс, включая Солнце. Довольно распространены.
				{ESpectralClass::K, 10}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::M, 5},
			}
		},
		{
			EStarClusterComposition::YellowOrange,
			{

				{ESpectralClass::G, 50}, // G-класс, включая Солнце. Довольно распространены.
				{ESpectralClass::K, 50}, // K-класс, оранжевые звезды. Достаточно обычны.

			}
		},
		{
			EStarClusterComposition::OnlyOrange,
			{
				{ESpectralClass::K, 100}, // K-класс, оранжевые звезды. Достаточно обычны.
			}
		},
		{
			EStarClusterComposition::MostlyOrange,
			{
				{ESpectralClass::G, 5}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::K, 90}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::M, 5}, // K-класс, оранжевые звезды. Достаточно обычны.
			}
		},
		{
			EStarClusterComposition::OrangeRed,
			{
				{ESpectralClass::K, 50}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::M, 50}, // K-класс, оранжевые звезды. Достаточно обычны.
			}
		},
		{
			EStarClusterComposition::OnlyRed,
			{
				{ESpectralClass::M, 100}, // K-класс, оранжевые звезды. Достаточно обычны.
			}
		},
		{
			EStarClusterComposition::MostlyRed,
			{
				{ESpectralClass::G, 5}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::K, 10}, // K-класс, оранжевые звезды. Достаточно обычны.
				{ESpectralClass::M, 85}, // K-класс, оранжевые звезды. Достаточно обычны.
			}
		},
	};



	const TMap<EStarClusterPopulation, TMap<EStellarType, int>> StarClusterPopulationWeights =
	{
		{
			EStarClusterPopulation::AllSequenses,
			{
				{EStellarType::HyperGiant, 1}, // Гипергиганты
				{EStellarType::SuperGiant, 3}, // Супергиганты
				{EStellarType::BrightGiant, 3}, // Белые карлики
				{EStellarType::Giant, 5}, // Гиганты
				{EStellarType::SubGiant, 5}, // Супергиганты
				{EStellarType::MainSequence, 80}, // Главная последовательность
				{EStellarType::SubDwarf, 5}, // Субкарлики
				{EStellarType::WhiteDwarf, 2}, // Белые карлики
				{EStellarType::BrownDwarf, 2}, // Коричневые карлики
				{EStellarType::Protostar, 1}, // Протозвезды
				{EStellarType::Neutron, 1}, // Нейтронные звезды
				{EStellarType::Pulsar, 1}, // Пульсары
				{EStellarType::BlackHole, 1} // Черные дыры
			}
		},
		{
			EStarClusterPopulation::MainSequence,
			{
				{EStellarType::MainSequence, 100}, // Главная последовательность
			}
		},
		{
			EStarClusterPopulation::Giants,
			{
				{EStellarType::HyperGiant, 1}, // Гипергиганты
				{EStellarType::SuperGiant, 5}, // Супергиганты
				{EStellarType::BrightGiant, 10}, // Белые карлики
				{EStellarType::Giant, 50}, // Гиганты
				{EStellarType::SubGiant, 30}, // Главная последовательность
			}
		},
		{
			EStarClusterPopulation::Dwarfs,
			{
				{EStellarType::SubDwarf, 50}, // Субкарлики
				{EStellarType::WhiteDwarf, 12}, // Белые карлики
				{EStellarType::BrownDwarf, 5}, // Коричневые карлики
				{EStellarType::Protostar, 5}, // Протозвезды
				{EStellarType::Neutron, 3}, // Нейтронные звезды
				{EStellarType::Pulsar, 2}, // Пульсары
				{EStellarType::BlackHole, 1}// Главная последовательность
			}
		},
		{
			EStarClusterPopulation::Protostars,
			{

				{EStellarType::BrownDwarf, 15}, // Коричневые карлики
				{EStellarType::Protostar, 50}, // Протозвезды
				{EStellarType::Neutron, 10}, // Нейтронные звезды
				{EStellarType::Pulsar, 10}, // Пульсары
			}
		},
	};

	const TMap<EStellarType, int> DefaultStarTypeWeights =
	{
		{EStellarType::HyperGiant, 1}, // Гипергиганты
		{EStellarType::SuperGiant, 3}, // Супергиганты
		{EStellarType::BrightGiant, 3}, // Белые карлики
		{EStellarType::Giant, 5}, // Гиганты
		{EStellarType::SubGiant, 5}, // Супергиганты
		{EStellarType::MainSequence, 80}, // Главная последовательность
		{EStellarType::SubDwarf, 5}, // Субкарлики
		{EStellarType::WhiteDwarf, 2}, // Коричневые карлики
		{EStellarType::BrownDwarf, 2}, // Коричневые карлики
		{EStellarType::Protostar, 1}, // Протозвезды
		{EStellarType::Neutron, 1}, // Нейтронные звезды
		{EStellarType::Pulsar, 1}, // Пульсары
		{EStellarType::BlackHole, 1} // Черные дыры
	};

	// Создаем TMap для цветов спектральных классов.
	const TMap<ESpectralClass, FString> SpectralClassColorMap =
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
	const TMap<ESpectralType, FString> SpectralTypeDescriptionMap =
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

	const TMap<ESpectralClass, TTuple<double, double>> MainSequenceMassRanges =
	{
		{ESpectralClass::O, TTuple<double, double>(16, 60)},
		{ESpectralClass::B, TTuple<double, double>(2.1, 16)},
		{ESpectralClass::A, TTuple<double, double>(1.5, 3)},
		{ESpectralClass::F, TTuple<double, double>(1.04, 1.6)},
		{ESpectralClass::G, TTuple<double, double>(0.8, 1.04)},
		{ESpectralClass::K, TTuple<double, double>(0.45, 0.8)},
		{ESpectralClass::M, TTuple<double, double>(0.08, 0.45)},
	};

	const TMap<ESpectralClass, TTuple<double, double>> MainSequenceRadiusRanges =
	{
		{ESpectralClass::O, TTuple<double, double>(6.6, 10)},
		{ESpectralClass::B, TTuple<double, double>(1.8, 6.6)},
		{ESpectralClass::A, TTuple<double, double>(1.4, 2.5)},
		{ESpectralClass::F, TTuple<double, double>(1.15, 1.4)},
		{ESpectralClass::G, TTuple<double, double>(0.96, 1.15)},
		{ESpectralClass::K, TTuple<double, double>(0.7, 0.96)},
		{ESpectralClass::M, TTuple<double, double>(0.3, 0.7)},
	};

	const TMap<ESpectralClass, TTuple<double, double>> StarTypeTemperatureRanges =
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

	// O.O
	TMap<EStellarType, FStarAttributeRanges> StarAttributeRanges =
	{
		{EStellarType::HyperGiant, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::SuperGiant, FStarAttributeRanges(
			{FLuminosityRange({50000.0, 90000.0}), FMassRange({15, 100}), FRadiusRange({30, 500}), FAgeRange({1.0e6, 1.0e7}), FAbsoluteMagnitudeRange({-4.0, -8.0})})},
		{EStellarType::BrightGiant, FStarAttributeRanges(
			{FLuminosityRange({650, 5000}), FMassRange({15, 70}), FRadiusRange({20, 100}), FAgeRange({1.0e10, 1.0e12}), FAbsoluteMagnitudeRange({-2.0, -5.0})})},
		{EStellarType::Giant, FStarAttributeRanges(
			{FLuminosityRange({10.0, 1000.0}), FMassRange({10, 500}), FRadiusRange({10.0, 50}), FAgeRange({1.0e8, 1.0e9}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::SubGiant, FStarAttributeRanges(
			{FLuminosityRange({5.0, 100.0}), FMassRange({5, 10}), FRadiusRange({2, 10}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-4.7, 3.2})})},
		{EStellarType::MainSequence, FStarAttributeRanges(
			{FLuminosityRange({0.00001, 100000.0}), FMassRange({0.1, 120}), FRadiusRange({0.1, 120}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::SubDwarf, FStarAttributeRanges(
			{FLuminosityRange({10, 100.0}), FMassRange({0.5, 2}), FRadiusRange({0.5, 2}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::WhiteDwarf, FStarAttributeRanges(
			{FLuminosityRange({0.00001, 0.01}), FMassRange({0.6, 1.4}), FRadiusRange({0.01, 0.2}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::BrownDwarf, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({0.005, 0.075}), FRadiusRange({0.01, 0.1}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::Neutron, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({100, 250}), FRadiusRange({40, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::Protostar, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({0.1, 150}), FRadiusRange({10, 20}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::Pulsar, FStarAttributeRanges(
			{FLuminosityRange({90000.0, 100000.0}), FMassRange({2, 200}), FRadiusRange({0.0001, 0.001}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		{EStellarType::BlackHole, FStarAttributeRanges(
			{FLuminosityRange({0.0, 0.0}), FMassRange({3, 100}), FRadiusRange({0.001, 1000}), FAgeRange({1.0e5, 1.0e6}), FAbsoluteMagnitudeRange({-8.0, -10.0})})},
		// остальные классы звезд...
	};
};
