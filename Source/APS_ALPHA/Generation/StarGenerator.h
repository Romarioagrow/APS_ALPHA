#pragma once

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "APS_ALPHA/Core/Enums/SpectralType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "StarGenerator.generated.h"

class AStar;
struct FStarClusterModel;
struct FStarModel;
enum class EStarClusterComposition : uint8;

UCLASS()
class APS_ALPHA_API UStarGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UStarGenerator();

private:
	FRandomStream RandomStream;

public:
	const double SolarEmissiveLuminosity = 1.0;
	const double MinStarLuminosity = 100;
	
	
	const double MaxStarLuminosity = 500;

	void ApplySpectralMaterial(AStar* NewStar, TSharedPtr<FStarModel> StarModel);

	void GenerateStarModelByProbability(TSharedPtr<FStarModel> NewStarModel, TSharedPtr<FStarClusterModel> FStarClusterModel);

	double WienLaw(double temperature);

	FLinearColor WavelengthToRGB(double wavelength);

	FLinearColor TemperatureToColor(double temperature);

	FLinearColor GetStarColor(ESpectralClass spectralClass, int subclass);

	FLinearColor TemperatureToRGB(float Temperature);

	void ApplyModel(AStar* NewStar, TSharedPtr<FStarModel> StarModel);

	void GenerateStarModel(TSharedPtr<FStarModel> StarModel);

	void GenerateRandomStarModel(TSharedPtr<FStarModel> StarModel);

	FString CalculateNonMainSequenceStarAge(double StarMass);

	FString CalculateMainSequenceStarAge(double mass);

	ESpectralClass GenerateSpectralClassByProbability(TMap<ESpectralClass, int> StarSpectralClassProbabilities);

	EStellarType GenerateStellarTypeByRandomWeights(TMap<EStellarType, float> StarTypeProbabilities);

	EStellarType GenerateStellarTypeByRandomWeights();

	double CalculateEmission(float starLuminosity);

private:
	FString GetSpectralClassColor(ESpectralClass Class);

	FString GetSpectralTypeDescription(ESpectralType Type);

	FName GenerateFullSpectralClass(const TUniquePtr<FStarModel> StarModel);

	FName GenerateFullSpectralName(const TUniquePtr<FStarModel> StarModel);

	ESpectralType CalculateSpectralType(EStellarType StellarType, double Luminosity);

	int CalculateSpectralSubclass(double StarTemperature, ESpectralClass SpectralClass);

	double CalculateLuminosityByMass(double Mass);

	double RandomFromRange(TTuple<double, double> range);

	double RandomMass(ESpectralClass SpectralClass);

	double RandomRadius(ESpectralClass SpectralClass);

	double CalculateSurfaceTemperature(double Luminosity, double Radius);

	ESpectralClass ChooseSpectralClassByStellarClass(EStellarType StellarClass);

	ESpectralClass DetermineSpectralClassByTemperature(EStellarType StarType, double Temperature);

	double CalculateLuminosity(double Radius, double SurfaceTemperature);

	double GenerateRandomTemperatureBySpectralClass(ESpectralClass SpectralClass);

//O - �����: 30000 - 60000 �
//B - ����� : 10000 - 30000 �
//A - ����� : 7500 - 10000 �
//F - ����� : 6000 - 7500 �
//G - ����� : 5200 - 6000 �
//K - ����� : 3700 - 5200 �
//M - ����� : 2000 - 3700 �
	const TMap<EStarClusterComposition, TMap<ESpectralClass, int>> StarClusterCompositionWeights =
	{
		{
			EStarClusterComposition::AllSpectral,
			{
				{ESpectralClass::O, 1}, // O-�����, ����� ������� � ����� ������. ����� ������.
				{ESpectralClass::B, 4}, // B-�����, �������, ����� ������. ������.
				{ESpectralClass::A, 6}, // A-�����, ����� ��� ������� ������. �� ����� ��������������.
				{ESpectralClass::F, 15}, // F-�����, ����� ������. �������� ��������������.
				{ESpectralClass::G, 22}, // G-�����, ������� ������. �������� ��������������.
				{ESpectralClass::K, 25}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::M, 76}, // M-�����, ������� ������. ����� ���������������� �����.
				{ESpectralClass::L, 5}, // L-�����, ���������� �������. ������.
				{ESpectralClass::T, 4}, // T-�����, ��� ����� �������� ���������� �������. ������.
				{ESpectralClass::Y, 2}, // Y-�����, ����� �������� �������, ����������� ��������. ����� ������.
				{ESpectralClass::BH, 1}, // Black holes. Extremely rare.
				{ESpectralClass::NS, 1}, // Neutron stars. Extremely rare.
				{ESpectralClass::PS, 1}, // Pulsars. Extremely rare.
			}
		},
		{
			EStarClusterComposition::OnlyBlue,
			{
				{ESpectralClass::O, 90}, // O-�����, ����� ������� � ����� ������. ����� ������.
				{ESpectralClass::B, 10}, // B-�����, �������, ����� ������. ������.
			}
		},
		{
			EStarClusterComposition::MostlyBlue,
			{
				{ESpectralClass::O, 50}, // O-�����, ����� ������� � ����� ������. ����� ������.
				{ESpectralClass::B, 50}, // B-�����, �������, ����� ������. ������.
				{ESpectralClass::A, 5}, // A-�����, ����� ��� ������� ������. �� ����� ��������������.
				{ESpectralClass::F, 5}, // F-�����, ����� ������. �������� ��������������.
				{ESpectralClass::G, 3}, // G-�����, ������� ������. �������� ��������������.
				{ESpectralClass::K, 2}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::M, 1},
			}
		},
		{
			EStarClusterComposition::BlueWhite,
			{
				{ESpectralClass::O, 50}, // O-�����, ����� ������� � ����� ������. ����� ������.
				{ESpectralClass::B, 50}, // B-�����, �������, ����� ������. ������.
				{ESpectralClass::A, 50}, // A-�����, ����� ��� ������� ������. �� ����� ��������������.
				{ESpectralClass::F, 50}, // F-�����, ����� ������. �������� ��������������.
			}
		},
		{
			EStarClusterComposition::OnlyWhite,
			{
				{ESpectralClass::A, 50}, // A-�����, ����� ��� ������� ������. �� ����� ��������������.
				{ESpectralClass::F, 50}, // F-�����, ����� ������. �������� ��������������.
			}
		},
		{
			EStarClusterComposition::MostlyWhite,
			{
				{ESpectralClass::A, 50}, // A-�����, ����� ��� ������� ������. �� ����� ��������������.
				{ESpectralClass::F, 50}, // F-�����, ����� ������. �������� ��������������.
				{ESpectralClass::O, 5}, // O-�����, ����� ������� � ����� ������. ����� ������.
				{ESpectralClass::B, 5},
				{ESpectralClass::G, 3}, // G-�����, ������� ������. �������� ��������������.
				{ESpectralClass::K, 2}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::M, 1},
			}
		},
		{
			EStarClusterComposition::WhiteYellow,
			{
				{ESpectralClass::A, 50}, // A-�����, ����� ��� ������� ������. �� ����� ��������������.
				{ESpectralClass::F, 50}, // F-�����, ����� ������. �������� ��������������.
				{ESpectralClass::G, 50}, // G-�����, ������� ������. �������� ��������������.
			}
		},
		{
			EStarClusterComposition::OnlyYellow,
			{
				{ESpectralClass::F, 5}, // G-�����, ������� ������. �������� ��������������.
				{ESpectralClass::G, 90}, // G-�����, ������� ������. �������� ��������������.
				{ESpectralClass::K, 5}, // G-�����, ������� ������. �������� ��������������.
			}
		},
		{
			EStarClusterComposition::MostlyYellow,
			{
				{ESpectralClass::A, 5}, // A-�����, ����� ��� ������� ������. �� ����� ��������������.
				{ESpectralClass::O, 5}, // O-�����, ����� ������� � ����� ������. ����� ������.
				{ESpectralClass::B, 5},
				{ESpectralClass::F, 10}, // F-�����, ����� ������. �������� ��������������.
				{ESpectralClass::G, 80}, // G-�����, ������� ������. �������� ��������������.
				{ESpectralClass::K, 10}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::M, 5},
			}
		},
		{
			EStarClusterComposition::YellowOrange,
			{

				{ESpectralClass::G, 50}, // G-�����, ������� ������. �������� ��������������.
				{ESpectralClass::K, 50}, // K-�����, ��������� ������. ���������� ������.

			}
		},
		{
			EStarClusterComposition::OnlyOrange,
			{
				{ESpectralClass::K, 100}, // K-�����, ��������� ������. ���������� ������.
			}
		},
		{
			EStarClusterComposition::MostlyOrange,
			{
				{ESpectralClass::G, 5}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::K, 90}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::M, 5}, // K-�����, ��������� ������. ���������� ������.
			}
		},
		{
			EStarClusterComposition::OrangeRed,
			{
				{ESpectralClass::K, 50}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::M, 50}, // K-�����, ��������� ������. ���������� ������.
			}
		},
		{
			EStarClusterComposition::OnlyRed,
			{
				{ESpectralClass::M, 100}, // K-�����, ��������� ������. ���������� ������.
			}
		},
		{
			EStarClusterComposition::MostlyRed,
			{
				{ESpectralClass::G, 5}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::K, 10}, // K-�����, ��������� ������. ���������� ������.
				{ESpectralClass::M, 85}, // K-�����, ��������� ������. ���������� ������.
			}
		},
	};



	const TMap<EStarClusterPopulation, TMap<EStellarType, float>> StarClusterPopulationWeights =
	{
		{
			EStarClusterPopulation::AllSequenses,
			{
				{EStellarType::HyperGiant, 0.001f}, // ������������
				{EStellarType::SuperGiant, 0.01f}, // ������������
				{EStellarType::BrightGiant, 0.1f}, // ����� �������
				{EStellarType::Giant, 1.0f}, // �������
				{EStellarType::SubGiant, 10.0f}, // ����������
				{EStellarType::MainSequence, 1000.0f}, // ������� ������������������
				{EStellarType::SubDwarf, 0.1f}, // ����������
				{EStellarType::WhiteDwarf, 1.0f}, // ����� �������
				{EStellarType::BrownDwarf, 0.01f}, // ���������� �������
				{EStellarType::Protostar, 0.01f}, // �����������
				{EStellarType::Neutron, 0.001f}, // ���������� ������
				{EStellarType::Pulsar, 0.001f}, // ��������
				{EStellarType::BlackHole, 0.001f}
			}
		},
		//{
		//	EStarClusterPopulation::AllSequenses,
		//	{
		//		{EStellarType::HyperGiant, 1}, // ������������
		//		{EStellarType::SuperGiant, 3}, // ������������
		//		{EStellarType::BrightGiant, 3}, // ����� �������
		//		{EStellarType::Giant, 5}, // �������
		//		{EStellarType::SubGiant, 5}, // ������������
		//		{EStellarType::MainSequence, 80}, // ������� ������������������
		//		{EStellarType::SubDwarf, 5}, // ����������
		//		{EStellarType::WhiteDwarf, 2}, // ����� �������
		//		{EStellarType::BrownDwarf, 2}, // ���������� �������
		//		{EStellarType::Protostar, 1}, // �����������
		//		{EStellarType::Neutron, 1}, // ���������� ������
		//		{EStellarType::Pulsar, 1}, // ��������
		//		{EStellarType::BlackHole, 1} // ������ ����
		//	}
		//},
		{
			EStarClusterPopulation::MainSequence,
			{
				{EStellarType::MainSequence, 100.f}, // ������� ������������������
			}
		},
		{
			EStarClusterPopulation::Giants,
			{
				{EStellarType::HyperGiant, 1.f}, // ������������
				{EStellarType::SuperGiant, 5.f}, // ������������
				{EStellarType::BrightGiant, 10.f}, // ����� �������
				{EStellarType::Giant, 50.f}, // �������
				{EStellarType::SubGiant, 30.f}, // ������� ������������������
			}
		},
		{
			EStarClusterPopulation::Dwarfs,
			{
				{EStellarType::SubDwarf, 50.f}, // ����������
				{EStellarType::WhiteDwarf, 12.f}, // ����� �������
				{EStellarType::BrownDwarf, 5.f}, // ���������� �������
				{EStellarType::Protostar, 5.f}, // �����������
				{EStellarType::Neutron, 3.f}, // ���������� ������
				{EStellarType::Pulsar, 2.f}, // ��������
				{EStellarType::BlackHole, 1.f}// ������� ������������������
			}
		},
		{
			EStarClusterPopulation::Protostars,
			{

				{EStellarType::BrownDwarf, 15.f}, // ���������� �������
				{EStellarType::Protostar, 50.f}, // �����������
				{EStellarType::Neutron, 10.f}, // ���������� ������
				{EStellarType::Pulsar, 10.f}, // ��������
			}
		},
	};

	//const TMap<EStellarType, int> DefaultStarTypeWeights =
	//{
	//	{EStellarType::HyperGiant, 0}, // ������������
	//	{EStellarType::SuperGiant, 0}, // ������������
	//	{EStellarType::BrightGiant, 0}, // ����� �������
	//	{EStellarType::Giant, 0}, // �������
	//	{EStellarType::SubGiant, 0}, // ����������
	//	{EStellarType::MainSequence, 90}, // ������� ������������������
	//	{EStellarType::SubDwarf, 0}, // ����������
	//	{EStellarType::WhiteDwarf, 0}, // ����� �������
	//	{EStellarType::BrownDwarf, 0}, // ���������� �������
	//	{EStellarType::Protostar, 0}, // �����������
	//	{EStellarType::Neutron, 0}, // ���������� ������
	//	{EStellarType::Pulsar, 0}, // ��������
	//	{EStellarType::BlackHole, 0} // ������ ����
	//};
	
	const TMap<EStellarType, float> DefaultStarTypeWeights =
	{
		{EStellarType::HyperGiant, 0.001f}, // ������������
		{EStellarType::SuperGiant, 0.01f}, // ������������
		{EStellarType::BrightGiant, 0.1f}, // ����� �������
		{EStellarType::Giant, 1.0f}, // �������
		{EStellarType::SubGiant, 10.0f}, // ����������
		{EStellarType::MainSequence, 1000.0f}, // ������� ������������������
		{EStellarType::SubDwarf, 0.1f}, // ����������
		{EStellarType::WhiteDwarf, 1.0f}, // ����� �������
		{EStellarType::BrownDwarf, 0.01f}, // ���������� �������
		{EStellarType::Protostar, 0.01f}, // �����������
		{EStellarType::Neutron, 0.001f}, // ���������� ������
		{EStellarType::Pulsar, 0.001f}, // ��������
		{EStellarType::BlackHole, 0.001f}
	};

	//const TMap<EStellarType, int> DefaultStarTypeWeights =
	//{
	//	{EStellarType::HyperGiant, 1}, // ������������
	//	{EStellarType::SuperGiant, 1}, // ������������
	//	{EStellarType::BrightGiant, 1}, // ����� �������
	//	{EStellarType::Giant, 3}, // �������
	//	{EStellarType::SubGiant, 3}, // ����������
	//	{EStellarType::MainSequence, 90}, // ������� ������������������
	//	{EStellarType::SubDwarf, 1}, // ����������
	//	{EStellarType::WhiteDwarf, 1}, // ����� �������
	//	{EStellarType::BrownDwarf, 1}, // ���������� �������
	//	{EStellarType::Protostar, 1}, // �����������
	//	{EStellarType::Neutron, 1}, // ���������� ������
	//	{EStellarType::Pulsar, 1}, // ��������
	//	{EStellarType::BlackHole, 1} // ������ ����
	//};

	//const TMap<EStellarType, int> DefaultStarTypeWeights =
	//{
	//	{EStellarType::HyperGiant, 1}, // ������������
	//	{EStellarType::SuperGiant, 3}, // ������������
	//	{EStellarType::BrightGiant, 3}, // ����� �������
	//	{EStellarType::Giant, 5}, // �������
	//	{EStellarType::SubGiant, 5}, // ������������
	//	{EStellarType::MainSequence, 80}, // ������� ������������������
	//	{EStellarType::SubDwarf, 5}, // ����������
	//	{EStellarType::WhiteDwarf, 2}, // ���������� �������
	//	{EStellarType::BrownDwarf, 2}, // ���������� �������
	//	{EStellarType::Protostar, 1}, // �����������
	//	{EStellarType::Neutron, 1}, // ���������� ������
	//	{EStellarType::Pulsar, 1}, // ��������
	//	{EStellarType::BlackHole, 1} // ������ ����
	//};

	// ������� TMap ��� ������ ������������ �������.
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

	// ������� TMap ��� �������� ������������ �����.
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
		// �������� ��������� ����, ���� ��� ����.
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
	};
};
