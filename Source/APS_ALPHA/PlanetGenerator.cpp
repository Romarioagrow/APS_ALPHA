// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetGenerator.h"
#include "MoonGenerator.h"
#include "Star.h"


//class UMoonGenerator MoonGenerator;

UPlanetGenerator::UPlanetGenerator()
{
    //MoonGenerator = NewObject<UMoonGenerator>(this);
}

//void UPlanetGenerator::InitMoonGenerator()
//{
//    if (!MoonGenerator.IsValid())
//    {
//        MoonGenerator = NewObject<UMoonGenerator>(this);
//    }
//}

TArray<FMoonData> UPlanetGenerator::GenerateMoonsList(FPlanetGenerationModel PlanetModel)
{
    return TArray<FMoonData>();
}

void UPlanetGenerator::ConnectPlanetWithStar(APlanet* NewPlanet, AStar* NewStar)
{
    NewStar->AddPlanet(NewPlanet);
    NewPlanet->AttachToActor(NewStar, FAttachmentTransformRules::KeepRelativeTransform);
    NewPlanet->SetParentStar(NewStar);
}

APlanet* UPlanetGenerator::GeneratePlanet(FPlanetGenerationModel PlanetModel)
{
    return nullptr;
}

FPlanetGenerationModel UPlanetGenerator::GenerateRandomPlanetModel()
{



    
    // 
    //     // Параметры планеты
    //     FPlanetParams Params;
    //
    //     // Параметры планеты
    //     Params.Name = "Planet";
    //     Params.Radius = 1000;
    //     Params.Mass = 1000;
    //     Params.Temperature = 1000;
    //     Params.AngularVelocity = 1000;
    //     Params.AxialTilt = 1000;
    //     Params.OrbitalPeriod = 1000;
    //     Params.OrbitalVelocity = 1000;
    //



    return FPlanetGenerationModel();
}

void UPlanetGenerator::ApplyModel(APlanet* PlanetActor, FPlanetGenerationModel PlanetGenerationModel)
{
	//this.PlanetType

	PlanetActor->SetPlanetType(PlanetGenerationModel.PlanetType);
	PlanetActor->SetPlanetZone(PlanetGenerationModel.PlanetZone);
	PlanetActor->SetPlanetDensity(PlanetGenerationModel.PlanetDensity);
	PlanetActor->SetPlanetGravityStrength(PlanetGenerationModel.PlanetGravityStrength);
	PlanetActor->SetTemperature(PlanetGenerationModel.Temperature);
	PlanetActor->SetAmountOfMoons(PlanetGenerationModel.AmountOfMoons);
	PlanetActor->SetRadius(PlanetGenerationModel.Radius);
	PlanetActor->SetMass(PlanetGenerationModel.Mass);
    PlanetActor->SetOrbitDistance(PlanetGenerationModel.OrbitDistance);
    PlanetActor->SetMoonsList(PlanetGenerationModel.MoonsList);

}

//TArray<FMoonData> UPlanetGenerator::GenerateMoonsList(FPlanetGenerationModel PlanetModel)
//{
//    TArray<FMoonData> MoonsList {};
//    const int AmountOfMoons = PlanetModel.AmountOfMoons;
//
//    const double MinOrbitRadius = PlanetModel.MoonOrbitsRange.Key;
//    const double MaxOrbitRadius = PlanetModel.MoonOrbitsRange.Value;
//
//    float uniformDistance = (MaxOrbitRadius - MinOrbitRadius) / (AmountOfMoons + 1);
//
//    // 2. Создаем массив со всеми возможными орбитами
//    TArray<float> MoonOrbits;
//    MoonOrbits.Reserve(AmountOfMoons);
//
//    for (size_t i = 0; i < AmountOfMoons; i++)
//    {
//        MoonOrbits.Add(MinOrbitRadius + uniformDistance * (i + 1));
//    }
//
//    for (double MoonOrbit : MoonOrbits)
//    {
//        FMoonGenerationModel MoonModel;
//
//        EMoonType MoonType = GenerateMoonType(PlanetModel);
//        
//        // Вычисляем физические параметры луны
//        double MoonMass = CalculateMoonMass(MoonType);
//        double MoonRadius = CalculateMoonRadius(MoonType);
//
//        // Создаем модель луны
//        MoonModel.Mass = MoonMass;
//        MoonModel.Radius = MoonRadius;
//        MoonModel.Type = MoonType;
//        MoonModel.OrbitDistance = MoonOrbit;
//
//        // Создаем данные о луне
//        int MoonIndex = MoonOrbits.IndexOfByKey(MoonOrbit);
//        FMoonData MoonData(MoonIndex + 1, MoonOrbit, MoonModel);
//
//        // Добавляем данные о луне в список
//        MoonsList.Add(MoonData);
//    }
//
//    
//
//
//    
//    return MoonsList;
//    //return TArray<FMoonData>();
//}

//EMoonType UPlanetGenerator::GenerateMoonType(FPlanetGenerationModel PlanetModel)
//{
//    // Маппинг типов планет и диапазонов вероятностей для типов лун
//    //TMap<EPlanetType, TMap<EMoonType, FFloatRange>> PlanetMoonTypeProbabilities;
//
//    //// TODO: Заполните маппинг PlanetMoonTypeProbabilities на основе вашего знания о том, какие типы лун наиболее вероятно появляются у каждого типа планеты
//
//    //// Получаем диапазон вероятностей для данного типа планеты
//    //TMap<EMoonType, FFloatRange> MoonTypeProbabilities = PlanetMoonTypeProbabilities[PlanetModel.PlanetType];
//
//    //// Генерируем случайное число от 0 до 1
//    //float RandNum = FMath::FRand();
//
//    //// Ищем соответствующий тип луны
//    //for (const auto& Entry : MoonTypeProbabilities)
//    //{
//    //    if (Entry.Value.Contains(RandNum))
//    //    {
//    //        return Entry.Key;
//    //    }
//    //}
//
//    // Если ничего не найдено, возвращаем Rocky по умолчанию
//    return EMoonType::Rocky;
//}
//
//double UPlanetGenerator::CalculateMoonMass(EMoonType MoonType)
//{
//    // Генерируем случайное число
//    float randomValue = FMath::FRand();
//
//    // Скалистые, железные и вулканические луны обычно имеют большую плотность
//    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
//    {
//        return randomValue * 1e23; // масса в килограммах
//    }
//
//    // Ледяные и океанические луны обычно имеют меньшую плотность
//    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
//    {
//        return randomValue * 1e22; // масса в килограммах
//    }
//
//    // Газовые луны и захваченные астероиды могут иметь различную массу
//    else
//    {
//        return randomValue * 1e21; // масса в килограммах
//    }
//}
//
//double UPlanetGenerator::CalculateMoonRadius(EMoonType MoonType)
//{
//    // Генерируем случайное число
//    float randomValue = FMath::FRand();
//
//    // Скалистые, железные и вулканические луны обычно имеют большой радиус
//    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
//    {
//        return randomValue * 2000; // радиус в километрах
//    }
//
//    // Ледяные и океанические луны обычно имеют меньший радиус
//    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
//    {
//        return randomValue * 1500; // радиус в километрах
//    }
//
//    // Газовые луны и захваченные астероиды могут иметь различный радиус
//    else
//    {
//        return randomValue * 1000; // радиус в километрах
//    }
//}