// Fill out your copyright notice in the Description page of Project Settings.


#include "StarGenerator.h"
#include "StarType.h"
#include <random>
//#include <random>
//#include <numeric>

// Define the random number generator as a member variable of your class
//std::default_random_engine generator;

UStarGenerator::UStarGenerator()
{
}

//

void UStarGenerator::ApplyModel(AStar* NewStar, FStarGenerationModel StarModel) // NewStar, StarModel
{
    NewStar->SetLuminosity(StarModel.Luminosity);
    NewStar->SetSurfaceTemperature(StarModel.SurfaceTemperature);
    NewStar->SetStarType(StarModel.StellarClass);
    NewStar->SetStarSpectralClass(StarModel.SpectralClass);
    
    NewStar->SetMass(StarModel.Mass);
    NewStar->SetRadius(StarModel.Radius);
    NewStar->SetAge(StarModel.Age);
    //NewStar->SetAbsoluteMagnitude(StarModel.AbsoluteMagnitude);
}

FStarGenerationModel UStarGenerator::GenerateRandomStarModel()
{
    FStarGenerationModel StarModel;

    TMap<EStellarClass, int> StarTypeWeights = 
    {
        {EStellarClass::HyperGiant, 1}, // Гипергиганты
        {EStellarClass::SuperGiant, 2}, // Супергиганты
        {EStellarClass::BrightGiant, 3}, // Белые карлики
        {EStellarClass::Giant, 4}, // Гиганты
        {EStellarClass::MainSequence, 10}, // Главная последовательность
        {EStellarClass::SubDwarf, 5}, // Субкарлики
        {EStellarClass::Protostar, 1}, // Протозвезды
        {EStellarClass::BrownDwarf, 2}, // Коричневые карлики
        {EStellarClass::Neutron, 1}, // Нейтронные звезды
        {EStellarClass::Pulsar, 1}, // Пульсары
        {EStellarClass::BlackHole, 1} // Черные дыры
    };
    int TotalWeight = 0;
    for (auto const& Pair : StarTypeWeights) {
        TotalWeight += Pair.Value;
    }
    int RandomValue = FMath::RandRange(0, TotalWeight - 1);
    EStellarClass ChosenStellarClass{};
    for (auto const& pair : StarTypeWeights) {
        if (RandomValue < pair.Value) {
            ChosenStellarClass = pair.Key;
            break;
        }
        RandomValue -= pair.Value;
    }
    StarModel.StellarClass = ChosenStellarClass;



    //Определите ESpectralClass (спектральный класс) 
    StarModel.SpectralClass = ChooseSpectralClassByStellarClass(ChosenStellarClass);

    if (StarModel.StellarClass == EStellarClass::MainSequence)
    {
        // Get the mass from the spectral class
        double Mass = RandomMass(StarModel.SpectralClass);

        // Calculate the radius and luminosity using mass-radius-luminosity relation
        double Radius = pow(Mass, 0.8);  // Radius  Mass^0.8
        double Luminosity = pow(Mass, 3.5);  // Luminosity  Mass^3.5
        double SurfaceTemperature = RandomFromRange(StarTypeTemperatureRanges[StarModel.SpectralClass]);

        // Generate a star model
        StarModel.Mass = Mass;
        StarModel.Radius = Radius;
        StarModel.Luminosity = Luminosity;
        StarModel.SurfaceTemperature = SurfaceTemperature;
    }
	else 
	{
        if (!StarAttributeRanges.Contains(StarModel.StellarClass))
        {
            UE_LOG(LogTemp, Error, TEXT("Unknown stellar class: %d"), static_cast<int>(StarModel.SpectralClass));
            return StarModel;
        }
    
        FStarAttributeRanges& attributeRanges = StarAttributeRanges[StarModel.StellarClass];
        StarModel.Mass = FMath::RandRange(attributeRanges.Mass.Range.Get<0>(), attributeRanges.Mass.Range.Get<1>());
        StarModel.Radius = FMath::RandRange(attributeRanges.Radius.Range.Get<0>(), attributeRanges.Radius.Range.Get<1>());
        StarModel.SurfaceTemperature = GenerateRandomTemperatureBySpectralClass(StarModel.SpectralClass);//CalculateSurfaceTemperature(double Luminosity, double Radius)
        StarModel.Luminosity = CalculateLuminosity(StarModel.Radius, StarModel.SurfaceTemperature);
        StarModel.Age = FMath::RandRange(attributeRanges.Age.Range.Get<0>(), attributeRanges.Age.Range.Get<1>());
    }

    return StarModel;
}

double UStarGenerator::RandomFromRange(TTuple<double, double> Range)
{
    return FMath::RandRange(Range.Key, Range.Value);
}

double UStarGenerator::RandomMass(ESpectralClass SpectralClass)
{
    auto Range = MainSequenceMassRanges[SpectralClass];
    return FMath::FRandRange(Range.Get<0>(), Range.Get<1>());
}

double UStarGenerator::RandomRadius(ESpectralClass SpectralClass)
{
    auto Range = MainSequenceRadiusRanges[SpectralClass];
    return FMath::FRandRange(Range.Get<0>(), Range.Get<1>());
}


double UStarGenerator::CalculateLuminosity(double Radius, double SurfaceTemperature)
{
    const double StefanBoltzmannConstant = 5.67e-8;  // in W/(m^2 K^4)
    const double SolarRadius = 6.96e8;  // in meters
    const double SolarLuminosity = 3.828e26;  // in watts

    double Luminosity = 4.0 * PI * pow(Radius * SolarRadius, 2) * StefanBoltzmannConstant * pow(SurfaceTemperature, 4);

    // Convert to solar luminosity units
    Luminosity /= SolarLuminosity;

    // Check for anomalies
    if (Luminosity < 0.0001) 
    {   
        // Set a minimum luminosity
        Luminosity = FMath::FRandRange(0.00001, 0.000001);
    }  
    if (Luminosity > 10000) 
    { 
        // Set a maximum luminosity
        Luminosity =  FMath::FRandRange(100000, 200000);
    }  
    return Luminosity;
}

double UStarGenerator::CalculateSurfaceTemperature(double Luminosity, double Radius)
{
    const double StefanBoltzmannConstant = 5.67e-8;
    const double SolarLuminosity = 3.828e26; // in Watts
    const double SolarRadius = 6.9634e8; // in meters

    // Convert solar units to SI units
    Luminosity *= SolarLuminosity;
    Radius *= SolarRadius;
    double SurfaceTemperature = pow(Luminosity / (4.0 * PI * Radius * Radius * StefanBoltzmannConstant), 0.25);

    return SurfaceTemperature;
}


/*
Для начала, давайте определим, какие спектральные классы будут соответствовать каждому классу звезды(EStellarClass).
Для этого мы можем использовать следующую таблицу :

EStellarClass	ESpectralClass
Dwarf Star	M, K, G(можно добавить и другие классы, если вы хотите иметь карликовые звезды различных спектральных классов)
Main Sequence Star	O, B, A, F, G, K, M
Giant Star	K, M
Supergiant Star	O, B, A, F, G, K, M

Используя эту информацию, мы можем начать писать функцию, которая будет случайно выбирать спектральный класс для заданного класса звезды.

EStellarClass	ESpectralClass
HyperGiant	    O, B, A, F, G, K
SuperGiant	    O, B, A, F, G, K, M
BrightGiant	    O, B, A, F, G, K, M
Giant	        O, B, A, F, G, K, M
SubGiant	    O, B, A, F, G, K
MainSequence	O, B, A, F, G, K, M
SubDwarf	    O, B, A, F, G, K
WhiteDwarf	    WD 
BrownDwarf	    L, T, Y 

*/



ESpectralClass UStarGenerator::ChooseSpectralClassByStellarClass(EStellarClass StellarClass)
{
    // Массивы спектральных классов для каждого стеллярного класса
    // Spectral Classes O, B, A, F, G, K, M
    const TArray<ESpectralClass> Spectral_OM = { ESpectralClass::O, ESpectralClass::B, ESpectralClass::A, ESpectralClass::F, ESpectralClass::G, ESpectralClass::K, ESpectralClass::M };
    // Spectral Classes O, B, A, F, G, K
    const TArray<ESpectralClass> Spectral_OK = { ESpectralClass::O, ESpectralClass::B, ESpectralClass::A, ESpectralClass::F, ESpectralClass::G, ESpectralClass::K };
    // Spectral Classes L, T, Y
    const TArray<ESpectralClass> Spectral_LY = { ESpectralClass::L, ESpectralClass::T, ESpectralClass::Y };

    // Веса для спектральных классов, исходя из предположительной частоты встречаемости
    const TArray<int> Weights_OM = { 3, 13, 22, 30, 20, 10, 2 }; // Weights for O, B, A, F, G, K, M
    const TArray<int> Weights_OK = { 3, 13, 22, 30, 20, 12 }; // Weights for O, B, A, F, G, K
    const TArray<int> Weights_LY = { 20, 50, 30 }; // Weights for L, T, Y


    // Определение выборки и весов
    const TArray<ESpectralClass>* SpectralArray;
    const TArray<int>* WeightsArray;

    switch (StellarClass)
    {
    case EStellarClass::HyperGiant:
    case EStellarClass::SuperGiant:
    case EStellarClass::BrightGiant:
    case EStellarClass::Giant:
    case EStellarClass::MainSequence:
        SpectralArray = &Spectral_OM;
        WeightsArray = &Weights_OM;
        break;
    case EStellarClass::SubGiant:
    case EStellarClass::SubDwarf:
        SpectralArray = &Spectral_OK;
        WeightsArray = &Weights_OK;
        break;
    case EStellarClass::WhiteDwarf:
    case EStellarClass::BrownDwarf:
        SpectralArray = &Spectral_LY;
        WeightsArray = &Weights_LY;
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("Invalid StellarClass!"));
        return ESpectralClass::O;  // Возвращает O класс по умолчанию или можно выбрать другой класс
    }

    // Создание кумулятивного массива весов
    TArray<int> CumulativeWeights;
    int TotalWeight = 0;
    for (int i = 0; i < WeightsArray->Num(); ++i)
    {
        TotalWeight += (*WeightsArray)[i];
        CumulativeWeights.Add(TotalWeight);
    }

    // Генерация случайного числа
    int RandWeight = FMath::RandRange(0, TotalWeight - 1);

    // Нахождение соответствующего спектрального класса
    for (int i = 0; i < CumulativeWeights.Num(); ++i)
    {
        if (RandWeight < CumulativeWeights[i])
        {
            return (*SpectralArray)[i];
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to choose spectral class!"));
    return ESpectralClass::M;

}

ESpectralClass UStarGenerator::DetermineSpectralClassByTemperature(EStellarClass StarType, double Temperature)
{
    switch (StarType) {
    /*case EStarStellarClass::WhiteDwarf:
        return EStarSpectralClass::WD;*/
    case EStellarClass::Neutron:
        return ESpectralClass::NS;
    case EStellarClass::BlackHole:
        return ESpectralClass::BH;
    case EStellarClass::BrownDwarf:
        if (Temperature > 1300) return ESpectralClass::L;
        else if (Temperature > 700) return ESpectralClass::T;
        else return ESpectralClass::Y;
    default:  // Для всех остальных типов звезд мы проверяем температуру
        if (Temperature > 30000) return ESpectralClass::O;
        else if (Temperature > 10000) return ESpectralClass::B;
        else if (Temperature > 7500) return ESpectralClass::A;
        else if (Temperature > 6000) return ESpectralClass::F;
        else if (Temperature > 5000) return ESpectralClass::G;
        else if (Temperature > 3500) return ESpectralClass::K;
        else return ESpectralClass::M;
    }
}

double UStarGenerator::GenerateRandomTemperatureBySpectralClass(ESpectralClass SpectralClass)
{
    TTuple<double, double> TemperatureRange = StarTypeTemperatureRanges[SpectralClass];
    return FMath::RandRange(TemperatureRange.Get<0>(), TemperatureRange.Get<1>());
}

//double UStarGenerator::GenerateRandomLuminosity(EStarType StarType)
//{
//    TTuple<double, double> LuminosityRange = StarTypeLuminosityRanges[StarType];
//    return FMath::RandRange(LuminosityRange.Get<0>(), LuminosityRange.Get<1>());
//}
//
//double UStarGenerator::GenerateRandomMass(EStarType StarType)
//{
//    TTuple<double, double> MassRange = StarTypeMassRanges[StarType];
//    return FMath::RandRange(MassRange.Get<0>(), MassRange.Get<1>());
//}
//
//double UStarGenerator::GenerateRandomRadius(EStarType StarType)
//{
//    TTuple<double, double> RadiusRange = StarTypeRadiusRanges[StarType];
//    return FMath::RandRange(RadiusRange.Get<0>(), RadiusRange.Get<1>());
//}