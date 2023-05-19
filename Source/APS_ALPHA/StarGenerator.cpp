// Fill out your copyright notice in the Description page of Project Settings.


#include "StarGenerator.h"
#include "StarType.h"

UStarGenerator::UStarGenerator()
{
}

//

void UStarGenerator::ApplyModel(AStar* NewStar, FStarGenerationModel StarModel) // NewStar, StarModel
{
    NewStar->SetLuminosity(StarModel.Luminosity);
    NewStar->SetSurfaceTemperature(StarModel.SurfaceTemperature);
    NewStar->SetStarType(StarModel.StarType);
    NewStar->SetStarSpectralClass(StarModel.StarSpectralClass);
}

FStarGenerationModel UStarGenerator::GenerateRandomStarModel()
{
    FStarGenerationModel StarModel;

    TMap<EStarType, int> StarTypeWeights = 
    {
        {EStarType::MainSequence, 10}, // Главная последовательность
        {EStarType::Giant, 2}, // Гиганты
        {EStarType::SuperGiant, 1}, // Супергиганты
        {EStarType::WhiteDwarf, 5}, // Белые карлики
        {EStarType::Neutron, 1}, // Нейтронные звезды
        {EStarType::BrownDwarf, 3}, // Коричневые карлики
        {EStarType::RedDwarf, 15}, // Красные карлики
        {EStarType::SubDwarf, 5}, // Субкарлики
        {EStarType::HyperGiant, 1}, // Гипергиганты
        {EStarType::Protostar, 3}, // Протозвезды
        {EStarType::Pulsar, 1}, // Пульсары
        {EStarType::BlackHole, 1} // Черные дыры
    };

    int TotalWeight = 0;
    for (auto const& Pair : StarTypeWeights) {
        TotalWeight += Pair.Value;
    }
    int RandomValue = FMath::RandRange(0, TotalWeight - 1);

    EStarType ChosenStarType;
    for (auto const& pair : StarTypeWeights) {
        if (RandomValue < pair.Value) {
            ChosenStarType = pair.Key;
            break;
        }
        RandomValue -= pair.Value;
    }
    StarModel.StarType = ChosenStarType;

    // Generate temperature and spectral class
    StarModel.SurfaceTemperature = GenerateRandomTemperature(StarModel.StarType);
    StarModel.StarSpectralClass = DetermineSpectralClass(StarModel.StarType, StarModel.SurfaceTemperature);
    /*StarModel.Luminosity = GenerateRandomLuminosity(StarModel.StarType);
    StarModel.Mass = GenerateRandomMass(StarModel.StarType);
    StarModel.Radius = GenerateRandomRadius(StarModel.StarType);*/


    return StarModel;
}

EStarSpectralClass UStarGenerator::DetermineSpectralClass(EStarType StarType, double Temperature)
{
    switch (StarType) {
    case EStarType::WhiteDwarf:
        return EStarSpectralClass::WD;
    case EStarType::Neutron:
        return EStarSpectralClass::NS;
    case EStarType::BlackHole:
        return EStarSpectralClass::BH;
    case EStarType::BrownDwarf:
        if (Temperature > 1300) return EStarSpectralClass::L;
        else if (Temperature > 700) return EStarSpectralClass::T;
        else return EStarSpectralClass::Y;
    default:  // Для всех остальных типов звезд мы проверяем температуру
        if (Temperature > 30000) return EStarSpectralClass::O;
        else if (Temperature > 10000) return EStarSpectralClass::B;
        else if (Temperature > 7500) return EStarSpectralClass::A;
        else if (Temperature > 6000) return EStarSpectralClass::F;
        else if (Temperature > 5000) return EStarSpectralClass::G;
        else if (Temperature > 3500) return EStarSpectralClass::K;
        else return EStarSpectralClass::M;
    }
}

TMap<EStarType, TTuple<double, double>> StarTypeTemperatureRanges =
{
    {EStarType::MainSequence, TTuple<double, double>(2400, 52000)},
    {EStarType::Giant, TTuple<double, double>(3500, 5500)},
    {EStarType::SuperGiant, TTuple<double, double>(3500, 20000)},
    {EStarType::WhiteDwarf, TTuple<double, double>(5000, 40000)},
    {EStarType::Neutron, TTuple<double, double>(600000, 600000)},
    {EStarType::BrownDwarf, TTuple<double, double>(250, 1300)},
    {EStarType::RedDwarf, TTuple<double, double>(2400, 3700)},
    {EStarType::SubDwarf, TTuple<double, double>(4000, 7500)},
    {EStarType::HyperGiant, TTuple<double, double>(3500, 35000)},
    {EStarType::Protostar, TTuple<double, double>(2000, 3000)},
    {EStarType::Pulsar, TTuple<double, double>(1000000, 1000000)},
    {EStarType::BlackHole, TTuple<double, double>(0, 0)} // Black holes do not have a definite temperature
};

double UStarGenerator::GenerateRandomTemperature(EStarType StarType)
{
    TTuple<double, double> TemperatureRange = StarTypeTemperatureRanges[StarType];
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