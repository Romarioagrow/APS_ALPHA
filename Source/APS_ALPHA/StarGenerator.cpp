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



    /*StarModel.Luminosity;
    StarModel.SurfaceTemperature;
    StarModel.Mass;
    StarModel.Radius;
    StarModel.Age;*/
 
    // Получите диапазоны атрибутов для данного StellarClass
    FStarAttributeRanges attributeRanges = StarAttributeRanges[ChosenStellarClass];

    // Генератор случайных чисел
    std::random_device rd;
    std::mt19937 generator(rd());

    // Генерируйте случайные значения для каждого параметра
    std::uniform_real_distribution<double> distLuminosity(attributeRanges.Luminosity.Key, attributeRanges.Luminosity.Value);
    StarModel.Luminosity = distLuminosity(generator);

    /*std::uniform_real_distribution<double> distTemperature(attributeRanges.SurfaceTemperatureRange[0], attributeRanges.SurfaceTemperatureRange[1]);
    StarModel.SurfaceTemperature = distTemperature(generator);*/

    std::uniform_real_distribution<double> distMass(attributeRanges.Mass.Key, attributeRanges.Mass.Value);
    StarModel.Mass = distMass(generator);

    std::uniform_real_distribution<double> distRadius(attributeRanges.Radius.Key, attributeRanges.Radius.Value);
    StarModel.Radius = distRadius(generator);

    std::uniform_real_distribution<double> distAge(attributeRanges.Age.Key, attributeRanges.Age.Value);
    StarModel.Age = distAge(generator);

    return StarModel;
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

TMap<ESpectralClass, TTuple<double, double>> StarTypeTemperatureRanges =
{
    
    //30 
    
    {ESpectralClass::O, TTuple<double, double>(3500, 30000)},
    {ESpectralClass::A, TTuple<double, double>(3500, 5500)},
    {ESpectralClass::B, TTuple<double, double>(3500, 20000)},
    {ESpectralClass::F, TTuple<double, double>(5000, 40000)},
    {ESpectralClass::G, TTuple<double, double>(600000, 600000)},
    {ESpectralClass::K, TTuple<double, double>(250, 1300)},
    {ESpectralClass::M, TTuple<double, double>(4000, 7500)},
    {ESpectralClass::L, TTuple<double, double>(3500, 35000)},
    {ESpectralClass::T, TTuple<double, double>(3500, 35000)},
    {ESpectralClass::Y, TTuple<double, double>(3500, 35000)},
   // {EStarStellarClass::Protostar, TTuple<double, double>(2000, 3000)},
    {ESpectralClass::NS, TTuple<double, double>(1000000, 1000000)},
    {ESpectralClass::BH, TTuple<double, double>(0, 0)} // Black holes do not have a definite temperature
};

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