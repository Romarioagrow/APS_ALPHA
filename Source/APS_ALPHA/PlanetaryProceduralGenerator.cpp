// Fill out your copyright notice in the Description page of Project Settings.

#include "PlanetaryProceduralGenerator.h"
#include "AstroActor.h"
#include "StarTypeProbabilities.h"
#include "PlanetarySystem.h"
#include "PlanetarySystemGenerationModel.h"
#include "OrbitDistributionType.h"
#include "PlanetGenerationModel.h"
#include <cmath>

void UPlanetarySystemGenerator::ApplyModel(APlanetarySystem* NewPlanetarySystem, TSharedPtr<FPlanetarySystemModel> PlanetraySystemModel)
{
    NewPlanetarySystem->SetAmountOfPlanets(PlanetraySystemModel->AmountOfPlanets);
    NewPlanetarySystem->SetPlanetarySystemType(PlanetraySystemModel->PlanetarySystemType);
    NewPlanetarySystem->SetOrbitDistributionType(PlanetraySystemModel->OrbitDistributionType);
    NewPlanetarySystem->SetStarFullSpectralName(PlanetraySystemModel->FullSpectralName);
    NewPlanetarySystem->SetPlanetsList(PlanetraySystemModel->PlanetsList);
    NewPlanetarySystem->HotZoneRadius = PlanetraySystemModel->HotZoneRadius;
    NewPlanetarySystem->WarmZoneRadius = PlanetraySystemModel->WarmZoneRadius;
    NewPlanetarySystem->ColdZoneRadius = PlanetraySystemModel->ColdZoneRadius;
    NewPlanetarySystem->IceZoneRadius = PlanetraySystemModel->IceZoneRadius;
    NewPlanetarySystem->InnerPlanetZoneRadius = PlanetraySystemModel->InnerPlanetZoneRadius;
    NewPlanetarySystem->HabitableZoneRadius = PlanetraySystemModel->HabitableZoneRadius;
    NewPlanetarySystem->GasGiantsZoneRadius = PlanetraySystemModel->GasGiantsZoneRadius;

}


void UPlanetarySystemGenerator::GenerateCustomPlanetraySystemModel(
    TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel,
    TSharedPtr<FStarModel> StarModel,
    UPlanetGenerator* PlanetGenerator,
    UMoonGenerator* MoonGenerator
)
{
    int32 FinalPlanetCount = PlanetarySystemModel->AmountOfPlanets; //FMath::RandRange(MinPlanetCount, MaxPlanetCount);

    // Это могут быть константы или переменные, зависящие от стелларного класса
    double MinOrbitScalingFactor = 1.0f;
    double MaxOrbitScalingFactor = 10.0f;

    if (StarModel->StellarType == EStellarType::HyperGiant)
    {
        MaxOrbitScalingFactor = 5.0f; // Уменьшаем максимальную орбиту для гипергигантов
    }
    else if (StarModel->StellarType == EStellarType::SuperGiant)
    {
        MaxOrbitScalingFactor = 6.0f; // Уменьшаем максимальную орбиту для сверхгигантов
    }

    double MinOrbit = StarModel->Mass * MinOrbitScalingFactor;
    double MaxOrbit = StarModel->Mass * MaxOrbitScalingFactor;
    StarModel->MinOrbit = MinOrbit;
    StarModel->MaxOrbit = MaxOrbit;
    UE_LOG(LogTemp, Warning, TEXT("MAX Orbit: %f"), MaxOrbit);


    // Подбираем случайное распределение для нашей системы
    EOrbitDistributionType OrbitDistributionType = PlanetarySystemModel->OrbitDistributionType;//ChooseOrbitDistribution(StarModel->StellarType);

    FString OrbitType = UEnum::GetValueAsString(OrbitDistributionType);
    UE_LOG(LogTemp, Warning, TEXT("Orbit Distribution Type: %s"), *OrbitType);


    /// TODO: To base method
    for (int i = 0; i < FinalPlanetCount; i++)
    {
        double OrbitDistributionValue;
        double OrbitRadius;
        switch (OrbitDistributionType)
        {
        case EOrbitDistributionType::Uniform:
            OrbitDistributionValue = FMath::RandRange(0.1, 1.0);
            // next orbit - PlanetAffection Zone + Star Radius
            break;
        case EOrbitDistributionType::Gaussian:
            OrbitDistributionValue = RandGauss();
            // Overlap Fix
            break;
        case EOrbitDistributionType::Chaotic:
        {
            OrbitDistributionValue = FMath::RandRange(MinOrbit, MaxOrbit);
            OrbitRadius = OrbitDistributionValue;
            break;
        }
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
            OrbitDistributionValue = FMath::RandRange(0.05, 0.5);
            break;
        }

        if (OrbitDistributionType != EOrbitDistributionType::Chaotic)
        {
            OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
        }

        // Применяем функцию распределения к нашему диапазону орбит
        OrbitRadii.Add(OrbitRadius);
    }

    OrbitRadii.Sort();
    UE_LOG(LogTemp, Warning, TEXT("OrbitRadii Num: %d "), OrbitRadii.Num());

    // Выводим минимальную и максимальную орбиту
    UE_LOG(LogTemp, Warning, TEXT("MinOrbit: %f, MaxOrbit: %f"), MinOrbit, MaxOrbit);

    // Вычисляем обитаемую зону
    float HabitableZoneInner = sqrt(StarModel->Luminosity / 1.1);
    float HabitableZoneOuter = sqrt(StarModel->Luminosity / 0.53);

    // Star Dead zone
    double StarDeadZoneInner = 0; // Начинается от звезды
    double StarRadiusInAU = StarModel->Radius * 0.00465047;
    double StarDeadZoneOuter = StarRadiusInAU * 2; // Заканчивается на расстоянии, равном двойному радиусу звезды в AU

    // Zones
    double HotZoneInner = 0;
    double HotZoneOuter = 0;
    double WarmZoneInner = 0;
    double WarmZoneOuter = 0;
    double ColdZoneInner = 0;
    double ColdZoneOuter = 0;
    double IceZoneInner = 0;
    double IceZoneOuter = 0;
    double GasGiantsZoneInner = 0;
    double GasGiantsZoneOuter = 0;
    double KuiperBeltZoneInner = 0;
    double KuiperBeltZoneOuter = 0;
    double InnerZoneInner = 0; // Начинается от звезды
    double InnerZoneOuter = 0; // Заканчивается границей горячей зоны
    double OuterZoneInner = 0; // Начинается от границы зоны газовых гигантов
    double OuterZoneOuter = 0; // Примерная формула

    if (HabitableZoneOuter < MaxOrbit)
    {
        // Вычисляем зону StarDeadZone
        StarDeadZoneInner = 0; // Начинается от звезды
        StarRadiusInAU = StarModel->Radius * 0.00465047;
        StarDeadZoneOuter = StarRadiusInAU * 2; // Заканчивается на расстоянии, равном двойному радиусу звезды в AU

        // Вычисляем горячую зону
        HotZoneInner = StarDeadZoneOuter;
        HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

        // Вычисляем теплую зону
        WarmZoneInner = HotZoneOuter;
        WarmZoneOuter = HabitableZoneInner;

        // Вычисляем холодную зону
        ColdZoneInner = HabitableZoneOuter;//FMath::Max(HabitableZoneOuter, MinOrbit);
        ColdZoneOuter = (ColdZoneInner * 2 > MaxOrbit) ? MaxOrbit : ColdZoneInner * 2;

        // Вычисляем ледяную зону
        IceZoneInner = ColdZoneOuter;//FMath::Max(ColdZoneOuter, MinOrbit);
        IceZoneOuter = (IceZoneInner * 2 > MaxOrbit) ? MaxOrbit : IceZoneInner * 2;

        // Вычисляем зону газовых гигантов
        GasGiantsZoneInner = FMath::Max(IceZoneOuter, MinOrbit);
        GasGiantsZoneOuter = (GasGiantsZoneInner * 2 > MaxOrbit) ? MaxOrbit : GasGiantsZoneInner * 2;

        // Вычисляем зону пояса Койпера
        KuiperBeltZoneInner = FMath::Max(GasGiantsZoneOuter, MinOrbit);
        KuiperBeltZoneOuter = (KuiperBeltZoneInner * 2 > MaxOrbit) ? MaxOrbit : KuiperBeltZoneInner * 2;


        if (OrbitDistributionType == EOrbitDistributionType::InnerOuter)
        {
            // Вычисляем внутреннюю зону
            InnerZoneInner = StarDeadZoneOuter; //0; // Начинается от звезды
            InnerZoneOuter = HotZoneOuter; // Заканчивается границей горячей зоны

            // Вычисляем внешнюю зону
            OuterZoneInner = GasGiantsZoneOuter; // Начинается от границы зоны газовых гигантов
            OuterZoneOuter = OuterZoneInner * 2; // Примерная формула

            PlanetarySystemModel->InnerPlanetZoneRadius = FZoneRadius(InnerZoneInner, InnerZoneOuter);
            PlanetarySystemModel->OuterPlanetZoneRadius = FZoneRadius(OuterZoneInner, OuterZoneOuter);
        }
    }
    else
    {
        StarDeadZoneInner = 0;
        StarRadiusInAU = StarModel->Radius * 0.00465047;
        StarDeadZoneOuter = StarRadiusInAU * 2;
        HotZoneInner = StarDeadZoneOuter;
        HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

        WarmZoneInner = HotZoneOuter;
        WarmZoneOuter = HabitableZoneInner;

        ColdZoneInner = WarmZoneOuter;
        ColdZoneOuter = FMath::Min(WarmZoneOuter * 2, MaxOrbit);

        if (ColdZoneOuter < MaxOrbit)
        {
            IceZoneInner = ColdZoneOuter;
            IceZoneOuter = FMath::Min(ColdZoneOuter * 2, MaxOrbit);
        }

        if (IceZoneOuter < MaxOrbit)
        {
            GasGiantsZoneInner = IceZoneOuter;
            GasGiantsZoneOuter = FMath::Min(IceZoneOuter * 2, MaxOrbit);
        }

        if (GasGiantsZoneOuter < MaxOrbit)
        {
            KuiperBeltZoneInner = GasGiantsZoneOuter;
            KuiperBeltZoneOuter = FMath::Min(GasGiantsZoneOuter * 2, MaxOrbit);
        }
    }

    PlanetarySystemModel->DeadZoneRadius = FZoneRadius(StarDeadZoneInner, StarDeadZoneOuter);
    PlanetarySystemModel->HabitableZoneRadius = FZoneRadius(HabitableZoneInner, HabitableZoneOuter);
    PlanetarySystemModel->ColdZoneRadius = FZoneRadius(ColdZoneInner, ColdZoneOuter);
    PlanetarySystemModel->IceZoneRadius = FZoneRadius(IceZoneInner, IceZoneOuter);
    PlanetarySystemModel->WarmZoneRadius = FZoneRadius(WarmZoneInner, WarmZoneOuter);
    PlanetarySystemModel->HotZoneRadius = FZoneRadius(HotZoneInner, HotZoneOuter);
    PlanetarySystemModel->GasGiantsZoneRadius = FZoneRadius(GasGiantsZoneInner, GasGiantsZoneOuter);
    PlanetarySystemModel->KuiperBeltZoneRadius = FZoneRadius(KuiperBeltZoneInner, KuiperBeltZoneOuter);

    // Выводим обитаемую зону
    UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

    // Выводим коэффициент орбиты
    // Выводим все орбиты планет
    for (int i = 0; i < OrbitRadii.Num(); ++i)
    {
        UE_LOG(LogTemp, Warning, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
    }

    UE_LOG(LogTemp, Warning, TEXT("Radius Dead Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel->DeadZoneRadius.InnerRadius, PlanetarySystemModel->DeadZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Hot Zone          - Inner: %f, Outer: %f AU"), PlanetarySystemModel->HotZoneRadius.InnerRadius, PlanetarySystemModel->HotZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Warm Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel->WarmZoneRadius.InnerRadius, PlanetarySystemModel->WarmZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Habitable Zone    - Inner: %f, Outer: %f AU"), PlanetarySystemModel->HabitableZoneRadius.InnerRadius, PlanetarySystemModel->HabitableZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Cold Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel->ColdZoneRadius.InnerRadius, PlanetarySystemModel->ColdZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Ice Zone          - Inner: %f, Outer: %f AU"), PlanetarySystemModel->IceZoneRadius.InnerRadius, PlanetarySystemModel->IceZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Gas Giants Zone   - Inner: %f, Outer: %f AU"), PlanetarySystemModel->GasGiantsZoneRadius.InnerRadius, PlanetarySystemModel->GasGiantsZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Kuiper Belt Zone  - Inner: %f, Outer: %f AU"), PlanetarySystemModel->KuiperBeltZoneRadius.InnerRadius, PlanetarySystemModel->KuiperBeltZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Inner Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel->InnerPlanetZoneRadius.InnerRadius, PlanetarySystemModel->InnerPlanetZoneRadius.OuterRadius);
    UE_LOG(LogTemp, Warning, TEXT("Radius Outer Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel->OuterPlanetZoneRadius.InnerRadius, PlanetarySystemModel->OuterPlanetZoneRadius.OuterRadius);

    /// TODO: To GeneratePlanetOrbits
    {
        for (double OrbitRadius : OrbitRadii)
        {
            int PlanetIndex = OrbitRadii.IndexOfByKey(OrbitRadius);

            //FPlanetModel PlanetModel; /// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);
            TSharedPtr<FPlanetModel> PlanetModel = MakeShared<FPlanetModel>(); /// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);


            PlanetModel->OrbitDistance = OrbitRadius;

            // planet temperature
            double PlanetTemperature = StarModel->SurfaceTemperature * sqrt(StarModel->Radius / (2 * OrbitRadius));
            PlanetModel->Temperature = PlanetTemperature; // to celestial body

            // Определите, в какой зоне находится планета.
            EPlanetaryZoneType PlanetZone = DeterminePlanetZone(OrbitRadius, PlanetarySystemModel);
            PlanetModel->PlanetZone = PlanetZone;

            EPlanetType PlanetType = DeterminePlanetType(PlanetZone);
            PlanetModel->PlanetType = PlanetType;

            FDensityRange PlanetDensityRange = GetPlanetDensityRange(PlanetType);
            double PlanetDensity = FMath::RandRange(PlanetDensityRange.MinDensity, PlanetDensityRange.MaxDensity);
            PlanetModel->PlanetDensity = PlanetDensity;

            // assuming that radius is random in the range 1 - 2 Earth radii (this can be adjusted)
            FRadiusRange PlanetRadiusRange = GetPlanetRadiusRange(PlanetType);
            double PlanetRadius = FMath::RandRange(PlanetRadiusRange.MinRadius, PlanetRadiusRange.MaxRadius);
            PlanetModel->Radius = PlanetRadius;

            // now calculate mass based on density and radius
            PlanetModel->Mass = PlanetDensity * (4.0 / 3.0) * PI * FMath::Pow(PlanetRadius, 3);

            const double EARTH_RADIUS_KM = 6371.0; // радиус Земли в километрах
            double RadiusKM = PlanetRadius * EARTH_RADIUS_KM;
            PlanetModel->RadiusKM = RadiusKM;

            // for Gravity
            PlanetModel->PlanetGravityStrength = PlanetModel->Mass / FMath::Pow(PlanetModel->Radius, 2);
            PlanetModel->AmountOfMoons = CalculateMoons(PlanetModel->Mass, PlanetType);

            // Moons orbits
            double PlanetMass = PlanetModel->Mass;
            double StarMass = StarModel->Mass;
            double PlanetToStarDistance = PlanetModel->Radius;
            double planetRadius = PlanetModel->Radius;

            double PlanetAtmosphereHeight = PlanetModel->RadiusKM / 30; // высота атмосферы планеты
            PlanetModel->AtmosphereHeight = PlanetAtmosphereHeight;

            // Минимальное расстояние - радиус планеты плюс высота атмосферы
            const double MinOrbitRadius = planetRadius + PlanetAtmosphereHeight;
            double Eccentricity = 0;
            double RadiusHill = PlanetToStarDistance * (1 - Eccentricity) * pow(PlanetMass / (3 * StarMass), 1.0 / 3);
            const double MaxOrbitRadius = RadiusHill;
            TPair<double, double> OrbitRadiusPair;
            if (MinOrbitRadius > MaxOrbitRadius)
            {
                OrbitRadiusPair.Key = MaxOrbitRadius;
                OrbitRadiusPair.Value = MinOrbitRadius;
            }
            else
            {
                OrbitRadiusPair.Key = MinOrbitRadius;
                OrbitRadiusPair.Value = MaxOrbitRadius;
            }

            PlanetModel->MoonOrbitsRange = OrbitRadiusPair;
            UE_LOG(LogTemp, Warning, TEXT("Planet Moons Orbit Radius    - Min: %f, Max: %f x"), OrbitRadiusPair.Key, OrbitRadiusPair.Value);

            const int AmountOfMoons = PlanetModel->AmountOfMoons;
            TArray<TSharedPtr<FMoonData>> MoonsList {};
            TArray<double> MoonOrbits;
            MoonOrbits.Reserve(AmountOfMoons);

            if (PlanetModel->PlanetType == EPlanetType::GasGiant
                || PlanetModel->PlanetType == EPlanetType::IceGiant
                || PlanetModel->PlanetType == EPlanetType::HotGiant) {
                // Распределение орбит от 1 до 10 радиусов планеты
                for (int i = 0; i < AmountOfMoons; i++) {
                    double orbitRadius = FMath::RandRange(PlanetRadius * 1.0, PlanetRadius * 10.0);
                    orbitRadius /= 40;
                    MoonOrbits.Add(orbitRadius);
                }
            }
            else {
                double a = 1.5;
                double d = 1.4;
                // Коэффициенты закона Тициуса-Боде для остальных планет
                for (int i = 0; i < AmountOfMoons; i++) {
                    double MoonOrbitRadius = a + d * pow(2, i);
                    MoonOrbitRadius = FMath::RandRange(MoonOrbitRadius * 0.9, MoonOrbitRadius * 1.3);
                    MoonOrbits.Add(MoonOrbitRadius);
                }
            }
            MoonOrbits.Sort();

            for (double MoonOrbit : MoonOrbits)
            {
                //FMoonGenerationModel MoonModel;
                TSharedPtr<FMoonModel> MoonModel = MakeShared<FMoonModel>();

                EMoonType MoonType = MoonGenerator->GenerateMoonType(PlanetModel);

                // Вычисляем физические параметры луны
                double MoonMass = MoonGenerator->CalculateRandomMoonMass();
                double MoonDensity = MoonGenerator->CalculateRandomMoonDensity(MoonType);
                double MoonRadius = MoonGenerator->CalculateMoonRadius(MoonDensity, MoonMass);
                double MoonGravity = MoonMass / FMath::Pow(MoonRadius, 2); /// TODO: to MoonGenerator->CalculateGravitationalForce(PlanetModel.Mass, MoonMass, MoonOrbit);

                // Создаем модель луны
                MoonModel->Type = MoonType;
                MoonModel->Mass = MoonMass;
                MoonModel->Radius = MoonRadius;
                MoonModel->RadiusKM = MoonModel->Radius * 6371.0;
                MoonModel->MoonDensity = MoonDensity; // TODO: To Parent 
                MoonModel->MoonGravity = MoonGravity;
                MoonModel->OrbitDistance = MoonOrbit;
                MoonModel->MoonAtmosphereHeight = MoonModel->RadiusKM / 30;

                // Создаем данные о луне
                int MoonIndex = MoonOrbits.IndexOfByKey(MoonOrbit);
                TSharedPtr<FMoonData> MoonData = MakeShared<FMoonData>(MoonIndex + 1, MoonOrbit, MoonModel);

                // Добавляем данные о луне в список
                MoonsList.Add(MoonData);
            }


            //// Вычисление позиций точек Лагранжа (это упрощенные формулы, в реальности они сложнее)
            //FVector L1_Position = FVector(OrbitRadius * (1 - pow(PlanetMass / 3, 1.0 / 3.0)), 0, 0);
            //FVector L2_Position = FVector(OrbitRadius * (1 + pow(PlanetMass / 3, 1.0 / 3.0)), 0, 0);
            //FVector L3_Position = FVector(-OrbitRadius * (1 + 5 * PlanetMass / 12), 0, 0);
            //FVector L4_Position = FVector(OrbitRadius * cos(PI / 3), OrbitRadius * sin(PI / 3), 0);
            //FVector L5_Position = FVector(OrbitRadius * cos(PI / 3), -OrbitRadius * sin(PI / 3), 0);
            //PlanetModel->LagrangePoints.Add(L1_Position);
            //PlanetModel->LagrangePoints.Add(L2_Position);
            //PlanetModel->LagrangePoints.Add(L3_Position);
            //PlanetModel->LagrangePoints.Add(L4_Position);
            //PlanetModel->LagrangePoints.Add(L5_Position);

            for (int32 i = 0; i <= (int32)EOrbitHeight::VeryHighOrbit; ++i)
            {
                FOrbitInfo OrbitInfo{};
                OrbitInfo.OrbitHeightType = (EOrbitHeight)i;
                OrbitInfo.OrbitHeight = PlanetGenerator->CalculateOrbitHeight(OrbitInfo.OrbitHeightType, PlanetRadius);
                PlanetModel->Orbits.Add(OrbitInfo);
            }

            PlanetModel->MoonsList = MoonsList;
            TSharedPtr<FPlanetData> PlanetData = MakeShared<FPlanetData>(PlanetIndex, OrbitRadius, PlanetModel);
            PlanetarySystemModel->PlanetsList.Add(PlanetData);
        }
    }
}



void UPlanetarySystemGenerator::GeneratePlanetraySystemModelByStar(TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel, TSharedPtr<FStarModel> StarModel, UPlanetGenerator* PlanetGenerator, UMoonGenerator* MoonGenerator)
{
    // вычисляем вероятность что будут планеты
    // находим макс и мин кол во планет
    // опредлеяем расположение орбит планет  

    //FPlanetarySystemModel PlanetarySystemModel;
    PlanetarySystemModel->FullSpectralName = StarModel->FullSpectralName;

    // Находим базовую вероятность для данного типа звезды
    PlanetProbability BaseProbability = BasePlanetProbabilities[StarModel->StellarType];
    // Модифицируем вероятность на основе массы звезды.
    PlanetProbability MassModifier;
    if (StarModel->StellarType == EStellarType::MainSequence)
    {
        MassModifier = 1 / (1 + FMath::Exp(-StarModel->Mass));
    }
    else
    {
        MassModifier = 1 / (1 + FMath::Exp(-StarModel->Mass / 10));
    }

    PlanetProbability FinalProbability = BaseProbability * MassModifier;
    UE_LOG(LogTemp, Warning, TEXT("FinalProbability: %f"), FinalProbability);
    bool HasPlanets = true;//FMath::FRand() <= FinalProbability;
    //bool HasPlanets = false;//FMath::FRand() <= FinalProbability;

    // Выводим информацию о звезде
    UE_LOG(LogTemp, Warning, TEXT("HasPlanets: %s"), HasPlanets ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("Star Information:"));
    UE_LOG(LogTemp, Warning, TEXT("Spectral Class: %s"), *UEnum::GetValueAsString(StarModel->SpectralClass));
    UE_LOG(LogTemp, Warning, TEXT("Stellar Class: %s"), *UEnum::GetValueAsString(StarModel->StellarType));
    UE_LOG(LogTemp, Warning, TEXT("Mass: %f Solar Masses"), StarModel->Mass);
    UE_LOG(LogTemp, Warning, TEXT("Radius: %f Solar Radii"), StarModel->Radius);

    if (HasPlanets)
    {
        const int32 MaxPlanetsAllowed = 20;
        int32 MinPlanetCount = 1;
        int32 MaxPlanetCount = FMath::Min(MaxPlanetsAllowed, FMath::Max(1, FMath::RoundToInt(StarModel->Mass * BasePlanetCount[StarModel->StellarType] * MassModifier)));

        if (StarModel->StellarType == EStellarType::MainSequence && StarModel->SpectralClass == ESpectralClass::M)
        {
            MaxPlanetCount = 5;
        }

        int32 FinalPlanetCount = FMath::RandRange(MinPlanetCount, MaxPlanetCount);
        PlanetarySystemModel->AmountOfPlanets = FinalPlanetCount;
        PlanetarySystemModel->PlanetarySystemType = EPlanetarySystemType::Unknown; ///

        UE_LOG(LogTemp, Warning, TEXT("MinPlanetCount: %d"), MinPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("MaxPlanetCount: %d"), MaxPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("FinalPlanetCount: %d"), FinalPlanetCount);

        // Это могут быть константы или переменные, зависящие от стелларного класса
        double MinOrbitScalingFactor = 1.0f;
        double MaxOrbitScalingFactor = 10.0f;

        if (StarModel->StellarType == EStellarType::HyperGiant)
        {
            MaxOrbitScalingFactor = 5.0f; // Уменьшаем максимальную орбиту для гипергигантов
        }
        else if (StarModel->StellarType == EStellarType::SuperGiant)
        {
            MaxOrbitScalingFactor = 6.0f; // Уменьшаем максимальную орбиту для сверхгигантов
        }

        double MinOrbit = StarModel->Mass * MinOrbitScalingFactor;
        double MaxOrbit = StarModel->Mass * MaxOrbitScalingFactor;

        // Подбираем случайное распределение для нашей системы
        EOrbitDistributionType OrbitDistributionType = ChooseOrbitDistribution(StarModel->StellarType);
        PlanetarySystemModel->OrbitDistributionType = OrbitDistributionType;

        FString OrbitType = UEnum::GetValueAsString(OrbitDistributionType);
        UE_LOG(LogTemp, Warning, TEXT("Orbit Distribution Type: %s"), *OrbitType);

        for (int i = 0; i < FinalPlanetCount; i++)
        {
            double OrbitDistributionValue;
            double OrbitRadius;
            switch (OrbitDistributionType)
            {
            case EOrbitDistributionType::Uniform:
                OrbitDistributionValue = FMath::RandRange(0.0, 1.0);
                break;
            case EOrbitDistributionType::Gaussian:
                OrbitDistributionValue = RandGauss();
                break;
            case EOrbitDistributionType::Chaotic:
            {
                OrbitDistributionValue = FMath::RandRange(MinOrbit, MaxOrbit);
                OrbitRadius = OrbitDistributionValue;
                break;
            }
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

            if (OrbitDistributionType != EOrbitDistributionType::Chaotic)
            {
                OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
            }

            // Применяем функцию распределения к нашему диапазону орбит
            OrbitRadii.Add(OrbitRadius);
        }

        OrbitRadii.Sort();
        UE_LOG(LogTemp, Warning, TEXT("OrbitRadii Num: %d "), OrbitRadii.Num());

        // Выводим минимальную и максимальную орбиту
        UE_LOG(LogTemp, Warning, TEXT("MinOrbit: %f, MaxOrbit: %f"), MinOrbit, MaxOrbit);

        // Вычисляем обитаемую зону
        float HabitableZoneInner = sqrt(StarModel->Luminosity / 1.1);
        float HabitableZoneOuter = sqrt(StarModel->Luminosity / 0.53);

        // Star Dead zone
        double StarDeadZoneInner = 0; // Начинается от звезды
        double StarRadiusInAU = StarModel->Radius * 0.00465047;
        double StarDeadZoneOuter = StarRadiusInAU * 2; // Заканчивается на расстоянии, равном двойному радиусу звезды в AU

        // Zones
        double HotZoneInner = 0;
        double HotZoneOuter = 0;
        double WarmZoneInner = 0;
        double WarmZoneOuter = 0;
        double ColdZoneInner = 0;
        double ColdZoneOuter = 0;
        double IceZoneInner = 0;
        double IceZoneOuter = 0;
        double GasGiantsZoneInner = 0;
        double GasGiantsZoneOuter = 0;
        double KuiperBeltZoneInner = 0;
        double KuiperBeltZoneOuter = 0;
        double InnerZoneInner = 0; // Начинается от звезды
        double InnerZoneOuter = 0; // Заканчивается границей горячей зоны
        double OuterZoneInner = 0; // Начинается от границы зоны газовых гигантов
        double OuterZoneOuter = 0; // Примерная формула

        if (HabitableZoneOuter < MaxOrbit)
        {
            // Вычисляем зону StarDeadZone
            StarDeadZoneInner = 0; // Начинается от звезды
            StarRadiusInAU = StarModel->Radius * 0.00465047;
            StarDeadZoneOuter = StarRadiusInAU * 2; // Заканчивается на расстоянии, равном двойному радиусу звезды в AU

            // Вычисляем горячую зону
            HotZoneInner = StarDeadZoneOuter;
            HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

            // Вычисляем теплую зону
            WarmZoneInner = HotZoneOuter;
            WarmZoneOuter = HabitableZoneInner;

            // Вычисляем холодную зону
            ColdZoneInner = HabitableZoneOuter;//FMath::Max(HabitableZoneOuter, MinOrbit);
            ColdZoneOuter = (ColdZoneInner * 2 > MaxOrbit) ? MaxOrbit : ColdZoneInner * 2;

            // Вычисляем ледяную зону
            IceZoneInner = ColdZoneOuter;//FMath::Max(ColdZoneOuter, MinOrbit);
            IceZoneOuter = (IceZoneInner * 2 > MaxOrbit) ? MaxOrbit : IceZoneInner * 2;

            // Вычисляем зону газовых гигантов
            GasGiantsZoneInner = FMath::Max(IceZoneOuter, MinOrbit);
            GasGiantsZoneOuter = (GasGiantsZoneInner * 2 > MaxOrbit) ? MaxOrbit : GasGiantsZoneInner * 2;

            // Вычисляем зону пояса Койпера
            KuiperBeltZoneInner = FMath::Max(GasGiantsZoneOuter, MinOrbit);
            KuiperBeltZoneOuter = (KuiperBeltZoneInner * 2 > MaxOrbit) ? MaxOrbit : KuiperBeltZoneInner * 2;


            if (OrbitDistributionType == EOrbitDistributionType::InnerOuter)
            {
                // Вычисляем внутреннюю зону
                InnerZoneInner = StarDeadZoneOuter; //0; // Начинается от звезды
                InnerZoneOuter = HotZoneOuter; // Заканчивается границей горячей зоны

                // Вычисляем внешнюю зону
                OuterZoneInner = GasGiantsZoneOuter; // Начинается от границы зоны газовых гигантов
                OuterZoneOuter = OuterZoneInner * 2; // Примерная формула

                PlanetarySystemModel->InnerPlanetZoneRadius = FZoneRadius(InnerZoneInner, InnerZoneOuter);
                PlanetarySystemModel->OuterPlanetZoneRadius = FZoneRadius(OuterZoneInner, OuterZoneOuter);
            }
        }
        else
        {
            StarDeadZoneInner = 0;
            StarRadiusInAU = StarModel->Radius * 0.00465047;
            StarDeadZoneOuter = StarRadiusInAU * 2;
            HotZoneInner = StarDeadZoneOuter;
            HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

            WarmZoneInner = HotZoneOuter;
            WarmZoneOuter = HabitableZoneInner;

            ColdZoneInner = WarmZoneOuter;
            ColdZoneOuter = FMath::Min(WarmZoneOuter * 2, MaxOrbit);

            if (ColdZoneOuter < MaxOrbit)
            {
                IceZoneInner = ColdZoneOuter;
                IceZoneOuter = FMath::Min(ColdZoneOuter * 2, MaxOrbit);
            }

            if (IceZoneOuter < MaxOrbit)
            {
                GasGiantsZoneInner = IceZoneOuter;
                GasGiantsZoneOuter = FMath::Min(IceZoneOuter * 2, MaxOrbit);
            }

            if (GasGiantsZoneOuter < MaxOrbit)
            {
                KuiperBeltZoneInner = GasGiantsZoneOuter;
                KuiperBeltZoneOuter = FMath::Min(GasGiantsZoneOuter * 2, MaxOrbit);
            }
        }

        PlanetarySystemModel->DeadZoneRadius = FZoneRadius(StarDeadZoneInner, StarDeadZoneOuter);
        PlanetarySystemModel->HabitableZoneRadius = FZoneRadius(HabitableZoneInner, HabitableZoneOuter);
        PlanetarySystemModel->ColdZoneRadius = FZoneRadius(ColdZoneInner, ColdZoneOuter);
        PlanetarySystemModel->IceZoneRadius = FZoneRadius(IceZoneInner, IceZoneOuter);
        PlanetarySystemModel->WarmZoneRadius = FZoneRadius(WarmZoneInner, WarmZoneOuter);
        PlanetarySystemModel->HotZoneRadius = FZoneRadius(HotZoneInner, HotZoneOuter);
        PlanetarySystemModel->GasGiantsZoneRadius = FZoneRadius(GasGiantsZoneInner, GasGiantsZoneOuter);
        PlanetarySystemModel->KuiperBeltZoneRadius = FZoneRadius(KuiperBeltZoneInner, KuiperBeltZoneOuter);

        // Выводим обитаемую зону
        UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

        // Выводим коэффициент орбиты
        // Выводим все орбиты планет
        for (int i = 0; i < OrbitRadii.Num(); ++i)
        {
            UE_LOG(LogTemp, Warning, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
        }

        UE_LOG(LogTemp, Warning, TEXT("Radius Dead Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel->DeadZoneRadius.InnerRadius, PlanetarySystemModel->DeadZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Hot Zone          - Inner: %f, Outer: %f AU"), PlanetarySystemModel->HotZoneRadius.InnerRadius, PlanetarySystemModel->HotZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Warm Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel->WarmZoneRadius.InnerRadius, PlanetarySystemModel->WarmZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Habitable Zone    - Inner: %f, Outer: %f AU"), PlanetarySystemModel->HabitableZoneRadius.InnerRadius, PlanetarySystemModel->HabitableZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Cold Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel->ColdZoneRadius.InnerRadius, PlanetarySystemModel->ColdZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Ice Zone          - Inner: %f, Outer: %f AU"), PlanetarySystemModel->IceZoneRadius.InnerRadius, PlanetarySystemModel->IceZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Gas Giants Zone   - Inner: %f, Outer: %f AU"), PlanetarySystemModel->GasGiantsZoneRadius.InnerRadius, PlanetarySystemModel->GasGiantsZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Kuiper Belt Zone  - Inner: %f, Outer: %f AU"), PlanetarySystemModel->KuiperBeltZoneRadius.InnerRadius, PlanetarySystemModel->KuiperBeltZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Inner Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel->InnerPlanetZoneRadius.InnerRadius, PlanetarySystemModel->InnerPlanetZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Outer Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel->OuterPlanetZoneRadius.InnerRadius, PlanetarySystemModel->OuterPlanetZoneRadius.OuterRadius);

        /// TODO: To GeneratePlanetOrbits
        // populate planets list by new PlanetModel
        for (double OrbitRadius : OrbitRadii)
        {
            int PlanetIndex = OrbitRadii.IndexOfByKey(OrbitRadius);

            //FPlanetModel PlanetModel; /// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);
            TSharedPtr<FPlanetModel> PlanetModel = MakeShared<FPlanetModel>(); /// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);
            
            
            PlanetModel->OrbitDistance = OrbitRadius;

            // planet temperature
            double PlanetTemperature = StarModel->SurfaceTemperature * sqrt(StarModel->Radius / (2 * OrbitRadius));
            PlanetModel->Temperature = PlanetTemperature; // to celestial body

            // Определите, в какой зоне находится планета.
            EPlanetaryZoneType PlanetZone = DeterminePlanetZone(OrbitRadius, PlanetarySystemModel);
            PlanetModel->PlanetZone = PlanetZone;

            EPlanetType PlanetType = DeterminePlanetType(PlanetZone);
            PlanetModel->PlanetType = PlanetType;

            FDensityRange PlanetDensityRange = GetPlanetDensityRange(PlanetType);
            double PlanetDensity = FMath::RandRange(PlanetDensityRange.MinDensity, PlanetDensityRange.MaxDensity);
            PlanetModel->PlanetDensity = PlanetDensity;

            // assuming that radius is random in the range 1 - 2 Earth radii (this can be adjusted)
            FRadiusRange PlanetRadiusRange = GetPlanetRadiusRange(PlanetType);
            double PlanetRadius = FMath::RandRange(PlanetRadiusRange.MinRadius, PlanetRadiusRange.MaxRadius);
            PlanetModel->Radius = PlanetRadius;

            // now calculate mass based on density and radius
            PlanetModel->Mass = PlanetDensity * (4.0 / 3.0) * PI * FMath::Pow(PlanetRadius, 3);

            const double EARTH_RADIUS_KM = 6371.0; // радиус Земли в километрах
            double RadiusKM = PlanetRadius * EARTH_RADIUS_KM;
            PlanetModel->RadiusKM = RadiusKM;

            // for Gravity
            PlanetModel->PlanetGravityStrength = PlanetModel->Mass / FMath::Pow(PlanetModel->Radius, 2);
            PlanetModel->AmountOfMoons = CalculateMoons(PlanetModel->Mass, PlanetType);

            // Moons orbits
            double PlanetMass = PlanetModel->Mass; 
            double StarMass = StarModel->Mass;
            double PlanetToStarDistance = PlanetModel->Radius; 
            double planetRadius = PlanetModel->Radius;

            //const double EARTH_RADIUS_KM = 6371.0; // радиус Земли в километрах
            //const double EARTH_MASS = 1.0; // масса Земли (в данном случае массы планеты измеряются в единицах массы Земли)
           // const double ATMOSPHERE_HEIGHT_COEFFICIENT = 100.0;
            //double massFactor = FMath::Clamp(PlanetModel->Mass / EARTH_MASS, 0.1, 5.0);
            //double PlanetAtmosphereHeight = (PlanetModel->Radius ) * massFactor * ATMOSPHERE_HEIGHT_COEFFICIENT;
            /// TODO: PlanetAtmosphere //double planetAtmosphereHeight = PlanetModel.AtmosphereHeight; // высота атмосферы планеты
            double PlanetAtmosphereHeight = PlanetModel->RadiusKM / 30; // высота атмосферы планеты
            PlanetModel->AtmosphereHeight = PlanetAtmosphereHeight;

            // Минимальное расстояние - радиус планеты плюс высота атмосферы
            const double MinOrbitRadius = planetRadius + PlanetAtmosphereHeight;
            double eccentricity = 0;
            double RadiusHill = PlanetToStarDistance * (1 - eccentricity) * pow(PlanetMass / (3 * StarMass), 1.0 / 3);
            const double MaxOrbitRadius = RadiusHill;
            TPair<double, double> OrbitRadiusPair;
            if (MinOrbitRadius > MaxOrbitRadius) 
            {
                OrbitRadiusPair.Key = MaxOrbitRadius;
                OrbitRadiusPair.Value = MinOrbitRadius;
            }
            else 
            {
                OrbitRadiusPair.Key = MinOrbitRadius;
                OrbitRadiusPair.Value = MaxOrbitRadius;
            }

            PlanetModel->MoonOrbitsRange = OrbitRadiusPair;
            UE_LOG(LogTemp, Warning, TEXT("Planet Moons Orbit Radius    - Min: %f, Max: %f x"), OrbitRadiusPair.Key, OrbitRadiusPair.Value);

            const int AmountOfMoons = PlanetModel->AmountOfMoons;
            TArray<TSharedPtr<FMoonData>> MoonsList {};
            TArray<float> MoonOrbits;
            // Создаем массив со всеми возможными орбитами
            MoonOrbits.Reserve(AmountOfMoons);

            if (PlanetModel->PlanetType == EPlanetType::GasGiant
                || PlanetModel->PlanetType == EPlanetType::IceGiant
                || PlanetModel->PlanetType == EPlanetType::HotGiant) {
                // Распределение орбит от 1 до 10 радиусов планеты
                for (int i = 0; i < AmountOfMoons; i++) {
                    double orbitRadius = FMath::RandRange(PlanetRadius * 1.0, PlanetRadius * 10.0);
                    orbitRadius /= 40;
                    MoonOrbits.Add(orbitRadius);
                }
            }
            else {
                double a = 1.5;
                double d = 1.4;
                // Коэффициенты закона Тициуса-Боде для остальных планет
                for (int i = 0; i < AmountOfMoons; i++) {
                    double orbitRadius = a + d * pow(2, i);
                    orbitRadius = FMath::RandRange(orbitRadius * 0.9, orbitRadius * 1.3);
                    MoonOrbits.Add(orbitRadius);
                }
            }
            MoonOrbits.Sort();

            for (double MoonOrbit : MoonOrbits)
            {
                //FMoonGenerationModel MoonModel;
                TSharedPtr<FMoonModel> MoonModel = MakeShared<FMoonModel>();


                EMoonType MoonType = MoonGenerator->GenerateMoonType(PlanetModel);

                // Вычисляем физические параметры луны
                double MoonMass = MoonGenerator->CalculateRandomMoonMass();
                double MoonDensity = MoonGenerator->CalculateRandomMoonDensity(MoonType);
                double MoonRadius = MoonGenerator->CalculateMoonRadius(MoonDensity, MoonMass);
                double MoonGravity = MoonMass / FMath::Pow(MoonRadius, 2); /// TODO: to MoonGenerator->CalculateGravitationalForce(PlanetModel.Mass, MoonMass, MoonOrbit);

                // Создаем модель луны
                MoonModel->Type = MoonType;
                MoonModel->Mass = MoonMass;
                MoonModel->Radius = MoonRadius;
                MoonModel->RadiusKM = MoonModel->Radius * 6371.0;
                MoonModel->MoonDensity = MoonDensity; // TODO: To Parent 
                MoonModel->MoonGravity = MoonGravity;
                MoonModel->OrbitDistance = MoonOrbit;
                MoonModel->MoonAtmosphereHeight = MoonModel->RadiusKM / 30;

                // Создаем данные о луне
                int MoonIndex = MoonOrbits.IndexOfByKey(MoonOrbit);
                TSharedPtr<FMoonData> MoonData = MakeShared<FMoonData>(MoonIndex + 1, MoonOrbit, MoonModel);

                // Добавляем данные о луне в список
                MoonsList.Add(MoonData);
            }


            //// Вычисление позиций точек Лагранжа (это упрощенные формулы, в реальности они сложнее)
            //FVector L1_Position = FVector(OrbitRadius * (1 - pow(PlanetMass / 3, 1.0 / 3.0)), 0, 0);
            //FVector L2_Position = FVector(OrbitRadius * (1 + pow(PlanetMass / 3, 1.0 / 3.0)), 0, 0);
            //FVector L3_Position = FVector(-OrbitRadius * (1 + 5 * PlanetMass / 12), 0, 0);
            //FVector L4_Position = FVector(OrbitRadius * cos(PI / 3), OrbitRadius * sin(PI / 3), 0);
            //FVector L5_Position = FVector(OrbitRadius * cos(PI / 3), -OrbitRadius * sin(PI / 3), 0);
            //PlanetModel->LagrangePoints.Add(L1_Position);
            //PlanetModel->LagrangePoints.Add(L2_Position);
            //PlanetModel->LagrangePoints.Add(L3_Position);
            //PlanetModel->LagrangePoints.Add(L4_Position);
            //PlanetModel->LagrangePoints.Add(L5_Position);

            for (int32 i = 0; i <= (int32)EOrbitHeight::VeryHighOrbit; ++i)
            {
                FOrbitInfo OrbitInfo{};
                OrbitInfo.OrbitHeightType = (EOrbitHeight)i;
                OrbitInfo.OrbitHeight = PlanetGenerator->CalculateOrbitHeight(OrbitInfo.OrbitHeightType, PlanetRadius);
                PlanetModel->Orbits.Add(OrbitInfo);
            }

            PlanetModel->MoonsList = MoonsList;
            TSharedPtr<FPlanetData> PlanetData = MakeShared<FPlanetData>(PlanetIndex, OrbitRadius, PlanetModel);
            PlanetarySystemModel->PlanetsList.Add(PlanetData);
        }
    }
    else 
    {
		PlanetarySystemModel->AmountOfPlanets = 0;
		PlanetarySystemModel->PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
	}

    OrbitRadii.Reset();
    //return PlanetarySystemModel;
}


void UPlanetarySystemGenerator::GeneratePlanetOrbits()
{

}



int UPlanetarySystemGenerator::CalculateMoons(double PlanetMass, EPlanetType PlanetType)
{
    // Calculate the base number of moons based on the planet mass
    int BaseMoonCount = PlanetMass / 10.0;

    // Generate a random number between 0 and 1
    double RandomNumber = FMath::FRand();

    // Apply the chance to have no moons
    if (RandomNumber < 0.3)
    {
        return 0;
    }
    else if (BaseMoonCount > 10)
    {
        return 
            PlanetType == EPlanetType::GasGiant ||
            PlanetType == EPlanetType::IceGiant ||
			PlanetType == EPlanetType::HotGiant ? FMath::RandRange(0, 10) : FMath::RandRange(0, 5);
    }
    {
        // Otherwise, the number of moons is the base moon count plus a random number
        return BaseMoonCount + FMath::RandRange(0, 3);
    }
}

FRadiusRange UPlanetarySystemGenerator::GetPlanetRadiusRange(EPlanetType PlanetType)
{
    if (PlanetRadiusRanges.Contains(PlanetType))
    {
        return PlanetRadiusRanges[PlanetType];
    }

    // Возвращаем значения по умолчанию или выбрасываем ошибку
    return FRadiusRange(0.2, 12.6);
}

FDensityRange UPlanetarySystemGenerator::GetPlanetDensityRange(EPlanetType PlanetType) 
{
    if (PlanetDensityRanges.Contains(PlanetType))
    {
        return PlanetDensityRanges[PlanetType];
    }

    // default value if planet type is not in map
    return FDensityRange(5.0, 5.0);
}

EPlanetaryZoneType UPlanetarySystemGenerator::DeterminePlanetZone(double OrbitRadius, TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel )
{
    /// TODO: To Struct
    /*for (FZone Zone : Zones)
    {
        if (OrbitRadius >= Zone.Radius.InnerRadius && OrbitRadius <= Zone.Radius.OuterRadius)
        {
            return Zone.ZoneType;
        }
    }*/

    if (OrbitRadius >= PlanetarySystemModel->DeadZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->DeadZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::DeadZone;
    }
    if (OrbitRadius >= PlanetarySystemModel->HotZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->HotZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::HotZone;
    }
    if (OrbitRadius >= PlanetarySystemModel->WarmZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->WarmZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::WarmZone;
    }
    if (OrbitRadius >= PlanetarySystemModel->HabitableZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->HabitableZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::HabitableZone;
    }
    if (OrbitRadius >= PlanetarySystemModel->ColdZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->ColdZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::ColdZone;
    }
    if (OrbitRadius >= PlanetarySystemModel->IceZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->IceZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::IceZone;
    }
    if (OrbitRadius >= PlanetarySystemModel->GasGiantsZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->GasGiantsZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::GasGiantsZone;
    }
    if (OrbitRadius >= PlanetarySystemModel->KuiperBeltZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->KuiperBeltZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::KuiperBeltZone;
    }

    // Если мы не нашли подходящую зону, возвращаем Unknown.
    return EPlanetaryZoneType::Unknown;
}

EPlanetType UPlanetarySystemGenerator::DeterminePlanetType(EPlanetaryZoneType PlanetZone)
{
    // Получите массив вероятностей для типов планет в этой зоне.
    TArray<FPlanetTypeProbability> PlanetTypeProbabilities = ZonePlanetProbabilities[PlanetZone]; // try catch missed types, crashed 

    // Генерируйте случайное число от 0 до 1.
    double RandomNumber = FMath::RandRange(0.0f, 1.0f);

    // Пройдите через массив вероятностей, подсчитывая сумму, пока не достигнете числа, которое больше или равно случайному числу.
    double CumulativeProbability = 0.0f;
    for (FPlanetTypeProbability PlanetTypeProbability : PlanetTypeProbabilities)
    {
        CumulativeProbability += PlanetTypeProbability.Probability;
        if (RandomNumber <= CumulativeProbability)
        {
            return PlanetTypeProbability.PlanetType;
        }
    }

    // Если вы не нашли соответствующего типа планеты (что не должно произойти, если вероятности правильно нормированы), верните Unknown.
    return EPlanetType::Unknown;
}

double UPlanetarySystemGenerator::RandGauss()
{
    double U = FMath::FRand();
    double V = FMath::FRand();
    double X = sqrt(-2.0 * log(U)) * cos(2.0 * PI * V);
    return X * 0.15 + 0.5;
}

EOrbitDistributionType UPlanetarySystemGenerator::ChooseDistributionType(EStellarType StellarClass, float StarMass, float MinOrbit, float MaxOrbit)
{
    EOrbitDistributionType OrbitDistributionType;

    if (StellarClass == EStellarType::MainSequence)
    {
        return ChooseOrbitDistribution(StellarClass);
	}
	else if (StellarClass == EStellarType::WhiteDwarf || StellarClass == EStellarType::SubDwarf)
    {
        // Если у нас маленькая звезда, то предпочтем Dense
        OrbitDistributionType = EOrbitDistributionType::Dense;
    }
    else if (StellarClass == EStellarType::Giant || StellarClass == EStellarType::SuperGiant || StellarClass == EStellarType::HyperGiant)
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


EOrbitDistributionType UPlanetarySystemGenerator::ChooseOrbitDistribution(EStellarType StellarClass)
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

void UPlanetarySystemGenerator::SetAstroLocation(int StarNumber, APlanetarySystem* NewPlanetarySystem)
{
   // NewPlanetarySystem->Get
}

void UPlanetarySystemGenerator::GenerateCustomPlanetarySystem()
{
}

int UPlanetarySystemGenerator::DetermineMaxPlanets(EStellarType StellarClass, FStarModel StarModel )
{
    int MaxPlanets;

    // У некоторых типов звезд вообще нет планет
    if (StellarClass == EStellarType::WhiteDwarf || StellarClass == EStellarType::Neutron) {
        return 0;
    }

    // Задаем базовое количество планет в зависимости от класса звезды
    switch (StellarClass) {
    case EStellarType::HyperGiant:
    case EStellarType::SuperGiant:
    case EStellarType::Giant:
        MaxPlanets = 10; // Базовое количество для гигантских звезд
        break;
    case EStellarType::MainSequence:
        MaxPlanets = 5; // Базовое количество для звезд главной последовательности
        break;
    default:
        MaxPlanets = 3; // Базовое количество для остальных типов звезд
        break;
    }

    // Модифицируем количество планет в зависимости от массы и возраста звезды
    MaxPlanets = MaxPlanets + StarModel.Mass * 0.5 + 0.2 * 0.1;

    // Ограничим максимальное количество планет, чтобы оно было разумным
    if (MaxPlanets > 20) {
        MaxPlanets = 20;
    }

    return MaxPlanets;
}

