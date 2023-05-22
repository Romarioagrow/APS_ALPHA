// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetaryProceduralGenerator.h"
#include "AstroActor.h"
//#include "OrbitDistributionType.h"
#include "StarTypeProbabilities.h"
#include "PlanetarySystem.h"
#include "PlanetarySystemGenerationModel.h"
#include "OrbitDistributionType.h"

void UPlanetarySystemGenerator::ApplyModel(APlanetarySystem* NewPlanetarySystem, FPlanetarySystemGenerationModel PlanetraySystemModel)
{
    NewPlanetarySystem->SetAmountOfPlanets(PlanetraySystemModel.AmountOfPlanets);
    NewPlanetarySystem->SetPlanetarySystemType(PlanetraySystemModel.PlanetarySystemType);
}




FPlanetarySystemGenerationModel UPlanetarySystemGenerator::GeneratePlanetraySystemModelByStar(FStarGenerationModel StarModel)
{
    // вычисляем вероятность что будут планеты
    // находим макс и мин кол во планет
    // опредлеяем расположение орбит планет  

    FPlanetarySystemGenerationModel PlanetarySystemModel;
    // Находим базовую вероятность для данного типа звезды
    PlanetProbability BaseProbability = BasePlanetProbabilities[StarModel.StellarClass];
    // Модифицируем вероятность на основе массы звезды.
    PlanetProbability MassModifier;
    if (StarModel.StellarClass == EStellarClass::MainSequence)
    {
        MassModifier = 1 / (1 + FMath::Exp(-StarModel.Mass));
    }
    else
    {
        MassModifier = 1 / (1 + FMath::Exp(-StarModel.Mass / 10));
    }

    PlanetProbability FinalProbability = BaseProbability * MassModifier;
    UE_LOG(LogTemp, Warning, TEXT("\nFinalProbability: %f"), FinalProbability);
    bool HasPlanets = FMath::FRand() <= FinalProbability;

    // Выводим информацию о звезде
    UE_LOG(LogTemp, Warning, TEXT("Star Information:"));
    UE_LOG(LogTemp, Warning, TEXT("Spectral Class: %s"), *UEnum::GetValueAsString(StarModel.SpectralClass));
    UE_LOG(LogTemp, Warning, TEXT("Stellar Class: %s"), *UEnum::GetValueAsString(StarModel.StellarClass));
    UE_LOG(LogTemp, Warning, TEXT("Mass: %f Solar Masses"), StarModel.Mass);
    UE_LOG(LogTemp, Warning, TEXT("Radius: %f Solar Radii"), StarModel.Radius);

    if (HasPlanets) 
    {
        /*
        try
        {
        }
        catch()
        {
        }
        */
        const int32 MaxPlanetsAllowed = 20;
        int32 MinPlanetCount = 1;
        int32 MaxPlanetCount = FMath::Min(MaxPlanetsAllowed, FMath::Max(1, FMath::RoundToInt(StarModel.Mass * BasePlanetCount[StarModel.StellarClass] * MassModifier)));
    
        if (StarModel.StellarClass == EStellarClass::MainSequence && StarModel.SpectralClass == ESpectralClass::M)
        {
            MaxPlanetCount = 5;
        }
    
        int32 FinalPlanetCount = FMath::RandRange(MinPlanetCount, MaxPlanetCount);
        PlanetarySystemModel.AmountOfPlanets = FinalPlanetCount;
        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::Unknown; ///

        UE_LOG(LogTemp, Warning, TEXT("MinPlanetCount: %d"), MinPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("MaxPlanetCount: %d"), MaxPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("FinalPlanetCount: %d"), FinalPlanetCount);


        // Это могут быть константы или переменные, зависящие от стелларного класса
        double MinOrbitScalingFactor = 1.0f;
        double MaxOrbitScalingFactor = 10.0f;

        if (StarModel.StellarClass == EStellarClass::HyperGiant)
        {
            MaxOrbitScalingFactor = 5.0f; // Уменьшаем максимальную орбиту для гипергигантов
        }
        else if (StarModel.StellarClass == EStellarClass::SuperGiant)
        {
            MaxOrbitScalingFactor = 6.0f; // Уменьшаем максимальную орбиту для сверхгигантов
        }

        double MinOrbit = StarModel.Mass * MinOrbitScalingFactor;
        double MaxOrbit = StarModel.Mass * MaxOrbitScalingFactor;


        // Подбираем случайное распределение для нашей системы
        EOrbitDistributionType OrbitDistributionType = ChooseOrbitDistribution(StarModel.StellarClass);//ChooseDistributionType(StarModel.StellarClass, StarModel.Mass, MinOrbit, MaxOrbit);//static_cast<EOrbitDistributionType>(FMath::RandRange(0, 2));
        PlanetarySystemModel.OrbitDistributionType = OrbitDistributionType;

        FString OrbitType = UEnum::GetValueAsString(OrbitDistributionType);
        UE_LOG(LogTemp, Warning, TEXT("Orbit Distribution Type: %s"), *OrbitType);
        
        TArray<double> OrbitRadii;
        for (int i = 0; i < FinalPlanetCount; i++)
        {
            double OrbitDistributionValue;
            switch (OrbitDistributionType)
            {
            case EOrbitDistributionType::Uniform:
                OrbitDistributionValue = FMath::RandRange(0.0, 1.0);
                break;
            case EOrbitDistributionType::Gaussian:
                //OrbitDistributionValue = FMath::RandGauss(0.5, 0.15);
                OrbitDistributionValue = RandGauss() * 0.15 + 0.5;
                break;
            case EOrbitDistributionType::Chaotic:
                OrbitDistributionValue = FMath::PerlinNoise1D(i * 0.1);
                break;
            case EOrbitDistributionType::InnerOuter:
            {

                if (i < FinalPlanetCount / 2.0) {
                    OrbitDistributionValue = FMath::RandRange(0.01, 0.5);
                }
                else {
                    OrbitDistributionValue = FMath::RandRange(0.5, 1.0);
                }
                break;
            }
            case EOrbitDistributionType::Dense:
                OrbitDistributionValue = FMath::RandRange(0.01, 0.5);
                break;
            }

            // Применяем функцию распределения к нашему диапазону орбит
            double OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
            OrbitRadii.Add(OrbitRadius);
        }

        OrbitRadii.Sort();

        // Выводим минимальную и максимальную орбиту
        UE_LOG(LogTemp, Warning, TEXT("MinOrbit: %f, MaxOrbit: %f"), MinOrbit, MaxOrbit);

        // Вычисляем обитаемую зону
        float HabitableZoneInner = sqrt(StarModel.Luminosity / 1.1);
        float HabitableZoneOuter = sqrt(StarModel.Luminosity / 0.53);
        //PlanetarySystemModel.HabitableZoneRadius = TPair<double, double>(HabitableZoneInner, HabitableZoneOuter);

        // Вычисляем холодную зону
        double ColdZoneInner = HabitableZoneOuter;
        double ColdZoneOuter = ColdZoneInner * 2; // Примерная формула
       // PlanetarySystemModel.ColdZoneRadius = TPair<double, double>(ColdZoneInner, ColdZoneOuter);

        // Вычисляем ледяную зону
        double IceZoneInner = ColdZoneOuter;
        double IceZoneOuter = IceZoneInner * 2; // Примерная формула
       // PlanetarySystemModel.IceZoneRadius = TPair<double, double>(IceZoneInner, IceZoneOuter);

        // Вычисляем теплую зону
        double WarmZoneInner = HabitableZoneInner / 2; // Примерная формула
        double WarmZoneOuter = HabitableZoneInner;
       // PlanetarySystemModel.WarmZoneRadius = TPair<double, double>(WarmZoneInner, WarmZoneOuter);

        // Вычисляем горячую зону
        double HotZoneInner = WarmZoneInner / 2; // Примерная формула
        double HotZoneOuter = WarmZoneInner;
     //   PlanetarySystemModel.HotZoneRadius = TPair<double, double>(HotZoneInner, HotZoneOuter);

        // Вычисляем зону газовых гигантов
        double GasGiantsZoneInner = IceZoneOuter; // Примерная формула
        double GasGiantsZoneOuter = GasGiantsZoneInner * 2; // Примерная формула
      //  PlanetarySystemModel.GasGiantsZoneRadius = TPair<double, double>(GasGiantsZoneInner, GasGiantsZoneOuter);

        // Вычисляем зону пояса астероидов
        TArray<TPair<double, double>> AsteroidBeltZoneRadius;
        // Здесь вы можете добавить каждый пояс астероидов в массив

        // Вычисляем зону пояса Койпера
        double KuiperBeltZoneInner = GasGiantsZoneOuter; // Примерная формула
        double KuiperBeltZoneOuter = KuiperBeltZoneInner * 2; // Примерная формула
        //PlanetarySystemModel.KuiperBeltZoneRadius = TPair<double, double>(KuiperBeltZoneInner, KuiperBeltZoneOuter);

        if (OrbitDistributionType == EOrbitDistributionType::InnerOuter)
        {
            // Вычисляем внутреннюю зону
            double InnerZoneInner = 0; // Начинается от звезды
            double InnerZoneOuter = HotZoneOuter; // Заканчивается границей горячей зоны
          //  PlanetarySystemModel.InnerZoneRadius = TPair<double, double>(InnerZoneInner, InnerZoneOuter);

            // Вычисляем внешнюю зону
            double OuterZoneInner = GasGiantsZoneOuter; // Начинается от границы зоны газовых гигантов
            double OuterZoneOuter = OuterZoneInner * 2; // Примерная формула
          //  PlanetarySystemModel.OuterZoneRadius = TPair<double, double>(OuterZoneInner, OuterZoneOuter);
        }


        // Выводим обитаемую зону
        UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

        // Выводим коэффициент орбиты
        // Выводим все орбиты планет
        for (int i = 0; i < OrbitRadii.Num(); ++i)
        {
            UE_LOG(LogTemp, Warning, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
        }

       /* UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.HabitableZoneRadius.Key, PlanetarySystemModel.HabitableZoneRadius.Value);
        UE_LOG(LogTemp, Warning, TEXT("Cold Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.ColdZoneRadius.Key, PlanetarySystemModel.ColdZoneRadius.Value);
        UE_LOG(LogTemp, Warning, TEXT("Ice Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.IceZoneRadius.Key, PlanetarySystemModel.IceZoneRadius.Value);
        UE_LOG(LogTemp, Warning, TEXT("Warm Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.WarmZoneRadius.Key, PlanetarySystemModel.WarmZoneRadius.Value);
        UE_LOG(LogTemp, Warning, TEXT("Hot Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.HotZoneRadius.Key, PlanetarySystemModel.HotZoneRadius.Value);
        UE_LOG(LogTemp, Warning, TEXT("Gas Giants Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.GasGiantsZoneRadius.Key, PlanetarySystemModel.GasGiantsZoneRadius.Value);

        for (const auto& AsteroidBelt : AsteroidBeltZoneRadius)
        {
            UE_LOG(LogTemp, Warning, TEXT("Asteroid Belt: Inner = %f, Outer = %f"), PlanetarySystemModel.AsteroidBeltZoneRadius.Key, PlanetarySystemModel.AsteroidBeltZoneRadius.Value);
        }

        UE_LOG(LogTemp, Warning, TEXT("Kuiper Belt Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.KuiperBeltZoneRadius.Key, PlanetarySystemModel.KuiperBeltZoneRadius.Value);
        UE_LOG(LogTemp, Warning, TEXT("Inner Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.InnerZoneRadius.Key, PlanetarySystemModel.InnerZoneRadius.Value);
        UE_LOG(LogTemp, Warning, TEXT("Outer Zone: Inner = %f, Outer = %f"), PlanetarySystemModel.OuterZoneRadius.Key, PlanetarySystemModel.OuterZoneRadius.Value);*/
	}
    else 
    {
		PlanetarySystemModel.AmountOfPlanets = 0;
		PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
	}


    
    return PlanetarySystemModel;
}

double UPlanetarySystemGenerator::RandGauss()
{
    double U = FMath::FRand();
    double V = FMath::FRand();
    double X = sqrt(-2.0 * log(U)) * cos(2.0 * PI * V);
    return X;
}

EOrbitDistributionType UPlanetarySystemGenerator::ChooseDistributionType(EStellarClass StellarClass, float StarMass, float MinOrbit, float MaxOrbit)
{
    EOrbitDistributionType OrbitDistributionType;

    if (StellarClass == EStellarClass::MainSequence)
    {
        return ChooseOrbitDistribution(StellarClass);

	}
	else

    if (StellarClass == EStellarClass::WhiteDwarf || StellarClass == EStellarClass::SubDwarf)
    {
        // Если у нас маленькая звезда, то предпочтем Dense
        OrbitDistributionType = EOrbitDistributionType::Dense;
    }
    else if (StellarClass == EStellarClass::Giant || StellarClass == EStellarClass::SuperGiant || StellarClass == EStellarClass::HyperGiant)
    {
        // Если у нас большая звезда, то предпочтем InnerOuter
        OrbitDistributionType = EOrbitDistributionType::InnerOuter;
    }
    else
    {
        // Если у нас звезда среднего размера, будем использовать разные типы распределения в зависимости от массы
        if (StarMass < 0.5f)
        {
            OrbitDistributionType = EOrbitDistributionType::Uniform;
        }
        else if (StarMass < 1.0f)
        {
            OrbitDistributionType = EOrbitDistributionType::Gaussian;
        }
        else if (MaxOrbit - MinOrbit < 5.0f) // Проверим, насколько близко друг к другу минимальная и максимальная орбиты
        {
            OrbitDistributionType = EOrbitDistributionType::Dense; // Если орбиты близки, будем использовать плотное распределение
        }
        else
        {
            OrbitDistributionType = EOrbitDistributionType::Chaotic;
        }
    }
    return OrbitDistributionType;
}


EOrbitDistributionType UPlanetarySystemGenerator::ChooseOrbitDistribution(EStellarClass StellarClass)
{
    // Получаем мапу вероятностей для данного класса звезды
    auto probabilities = StellarOrbitDistributions[StellarClass];

    // Генерируем случайное число в диапазоне от 0 до 1
    float randomValue = FMath::FRand();

    // Идем по всем парам ключ-значение в словаре вероятностей
    for (auto& keyValue : probabilities)
    {
        // Уменьшаем случайное число на вероятность текущего типа
        randomValue -= keyValue.Value;

        // Если случайное число стало меньше нуля, выбираем текущий тип
        if (randomValue < 0)
        {
            return keyValue.Key;
        }
    }

    // Возвращаем последний тип, если что-то пошло не так
    return probabilities.end().Key();
}

//EOrbitDistributionType UPlanetarySystemGenerator::ChooseDistributionType(EStellarClass StellarClass, float StarMass, float OrbitRange)
//{
//    if (StellarClass == EStellarClass::Giant || StellarClass == EStellarClass::SuperGiant)
//    {
//        // Для гигантских звезд выбираем хаотичное распределение
//        return EOrbitDistributionType::Chaotic;
//    }
//    else if (StarMass > 1.5f)
//    {
//        // Для более массивных звезд выбираем гауссово распределение
//        return EOrbitDistributionType::Gaussian;
//    }
//    else
//    {
//        // Во всех остальных случаях выбираем равномерное распределение
//        return EOrbitDistributionType::Uniform;
//    }
//}

int UPlanetarySystemGenerator::DetermineMaxPlanets(EStellarClass StellarClass, FStarGenerationModel StarModel )
{

    int MaxPlanets;

    // У некоторых типов звезд вообще нет планет
    if (StellarClass == EStellarClass::WhiteDwarf || StellarClass == EStellarClass::Neutron) {
        return 0;
    }

    // Задаем базовое количество планет в зависимости от класса звезды
    switch (StellarClass) {
    case EStellarClass::HyperGiant:
    case EStellarClass::SuperGiant:
    case EStellarClass::Giant:
        MaxPlanets = 10; // Базовое количество для гигантских звезд
        break;
    case EStellarClass::MainSequence:
        MaxPlanets = 5; // Базовое количество для звезд главной последовательности
        break;
    default:
        MaxPlanets = 3; // Базовое количество для остальных типов звезд
        break;
    }

    // Модифицируем количество планет в зависимости от массы и возраста звезды
    MaxPlanets = MaxPlanets + StarModel.Mass * 0.5 + 0.2 * 0.1;

    // Убедимся, что у нас есть хотя бы одна планета, если это возможно
    /*if (MaxPlanets < 1 && StellarClass != EStellarClass::WhiteDwarf && StellarClass != EStellarClass::NeutronStar) {
        MaxPlanets = 1;
    }*/

    // Ограничим максимальное количество планет, чтобы оно было разумным
    if (MaxPlanets > 20) {
        MaxPlanets = 20;
    }

    return MaxPlanets;
}

FPlanetarySystemGenerationModel UPlanetarySystemGenerator::GenerateRandomPlanetraySystemModelByStar(FStarGenerationModel StarModel)
{
    FPlanetarySystemGenerationModel PlanetarySystemModel;
    EStellarClass StellarClass = StarModel.StellarClass;
    EPlanetarySystemType PlanetarySystemType = GetRandomWithProbability(StarSystemTypeProbabilities[StellarClass].SystemTypeProbabilities);
    PlanetarySystemModel.PlanetarySystemType = PlanetarySystemType;

    // Получаем диапазон значений для данного типа системы
    FPlanetarySystemRanges SystemRanges = PlanetarySystemRanges[PlanetarySystemType];

    // Генерируем случайное значение в диапазоне
    int AmountOfPlanets = FMath::RandRange(SystemRanges.AmountOfPlanetsRange.Get<0>(), SystemRanges.AmountOfPlanetsRange.Get<1>());
    PlanetarySystemModel.AmountOfPlanets = AmountOfPlanets;

    double DistanceBetweenPlanets = FMath::RandRange(SystemRanges.DistanceBetweenPlanetsRange.Get<0>(), SystemRanges.DistanceBetweenPlanetsRange.Get<1>());
    PlanetarySystemModel.DistanceBetweenPlanets = DistanceBetweenPlanets;

    return PlanetarySystemModel;
}


FPlanetarySystemGenerationModel UPlanetarySystemGenerator::GenerateRandomPlanetraySystemModel()
{
    FPlanetarySystemGenerationModel PlanetarySystemModel;

    int32 RandomValue = FMath::RandRange(0, 100);

    // Веса для каждого типа системы
    int32 NoPlanetSystemWeight = 10; // 10%
    int32 SmallSystemWeight = 30; // 30%
    int32 LargeSystemWeight = 25; // 25%
    int32 ChaoticSystemWeight = 20; // 20%
    int32 DenseSystemWeight = 15; // 15%

    if (RandomValue < NoPlanetSystemWeight)
    {
        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
        PlanetarySystemModel.AmountOfPlanets = 0;
        PlanetarySystemModel.DistanceBetweenPlanets = 0.0;
    }
    else if (RandomValue < NoPlanetSystemWeight + SmallSystemWeight)
    {
        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::SmallSystem;
        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(1, 5);
        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(1.0, 3.0);
    }
    else if (RandomValue < NoPlanetSystemWeight + SmallSystemWeight + LargeSystemWeight)
    {
        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::LargeSystem;
        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(6, 12);
        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(1.0, 5.0);
    }
    else if (RandomValue < NoPlanetSystemWeight + SmallSystemWeight + LargeSystemWeight + ChaoticSystemWeight)
    {
        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::ChaoticSystem;
        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(5, 12);
        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.1, 2.0);
    }
    else
    {
        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::DenseSystem;
        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(3, 10);
        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.2, 1.0);
    }

    return PlanetarySystemModel;
}
