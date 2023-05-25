// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetaryProceduralGenerator.h"
#include "AstroActor.h"
//#include "OrbitDistributionType.h"
#include "StarTypeProbabilities.h"
#include "PlanetarySystem.h"
#include "PlanetarySystemGenerationModel.h"
#include "OrbitDistributionType.h"
#include "PlanetGenerationModel.h"

void UPlanetarySystemGenerator::ApplyModel(APlanetarySystem* NewPlanetarySystem, FPlanetarySystemGenerationModel PlanetraySystemModel)
{
    NewPlanetarySystem->SetAmountOfPlanets(PlanetraySystemModel.AmountOfPlanets);
    NewPlanetarySystem->SetPlanetarySystemType(PlanetraySystemModel.PlanetarySystemType);
    NewPlanetarySystem->SetOrbitDistributionType(PlanetraySystemModel.OrbitDistributionType);
    NewPlanetarySystem->SetStarFullSpectralName(PlanetraySystemModel.FullSpectralName);
    NewPlanetarySystem->SetPlanetsList(PlanetraySystemModel.PlanetsList);

}




FPlanetarySystemGenerationModel UPlanetarySystemGenerator::GeneratePlanetraySystemModelByStar(FStarGenerationModel StarModel, UPlanetGenerator* PlanetGenerator, UMoonGenerator* MoonGenerator)
{
    // вычисляем вероятность что будут планеты
    // находим макс и мин кол во планет
    // опредлеяем расположение орбит планет  

    FPlanetarySystemGenerationModel PlanetarySystemModel;
    PlanetarySystemModel.FullSpectralName = StarModel.FullSpectralName;

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
    UE_LOG(LogTemp, Warning, TEXT("FinalProbability: %f"), FinalProbability);
    bool HasPlanets = FMath::FRand() <= FinalProbability;

   // UE_LOG(LogTemp, Warning, TEXT("HasPlanets: "), HasPlanets);
    UE_LOG(LogTemp, Warning, TEXT("HasPlanets: %s"), HasPlanets ? TEXT("true") : TEXT("false"));
    
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
        EOrbitDistributionType OrbitDistributionType = ChooseOrbitDistribution(StarModel.StellarClass);
        PlanetarySystemModel.OrbitDistributionType = OrbitDistributionType;

        FString OrbitType = UEnum::GetValueAsString(OrbitDistributionType);
        UE_LOG(LogTemp, Warning, TEXT("Orbit Distribution Type: %s"), *OrbitType);

        //TArray<double> OrbitRadii;
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
                //OrbitRadii.Add(OrbitRadius);
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
        float HabitableZoneInner = sqrt(StarModel.Luminosity / 1.1);
        float HabitableZoneOuter = sqrt(StarModel.Luminosity / 0.53);

        // Star Dead zone
        double StarDeadZoneInner = 0; // Начинается от звезды
        double StarRadiusInAU = StarModel.Radius * 0.00465047;
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
            StarRadiusInAU = StarModel.Radius * 0.00465047;
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

                PlanetarySystemModel.InnerPlanetZoneRadius = FZoneRadius(InnerZoneInner, InnerZoneOuter);
                PlanetarySystemModel.OuterPlanetZoneRadius = FZoneRadius(OuterZoneInner, OuterZoneOuter);
            }
        }
        else
        {
            StarDeadZoneInner = 0;
            StarRadiusInAU = StarModel.Radius * 0.00465047;
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

        PlanetarySystemModel.DeadZoneRadius = FZoneRadius(StarDeadZoneInner, StarDeadZoneOuter);
        PlanetarySystemModel.HabitableZoneRadius = FZoneRadius(HabitableZoneInner, HabitableZoneOuter);
        PlanetarySystemModel.ColdZoneRadius = FZoneRadius(ColdZoneInner, ColdZoneOuter);
        PlanetarySystemModel.IceZoneRadius = FZoneRadius(IceZoneInner, IceZoneOuter);
        PlanetarySystemModel.WarmZoneRadius = FZoneRadius(WarmZoneInner, WarmZoneOuter);
        PlanetarySystemModel.HotZoneRadius = FZoneRadius(HotZoneInner, HotZoneOuter);
        PlanetarySystemModel.GasGiantsZoneRadius = FZoneRadius(GasGiantsZoneInner, GasGiantsZoneOuter);
        PlanetarySystemModel.KuiperBeltZoneRadius = FZoneRadius(KuiperBeltZoneInner, KuiperBeltZoneOuter);


        //PlanetarySystemModel.AmountOfPlanets = OrbitRadii.Num();

        // Выводим обитаемую зону
        UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

        // Выводим коэффициент орбиты
        // Выводим все орбиты планет
        for (int i = 0; i < OrbitRadii.Num(); ++i)
        {
            UE_LOG(LogTemp, Warning, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
        }

        UE_LOG(LogTemp, Warning, TEXT("Radius Dead Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel.DeadZoneRadius.InnerRadius, PlanetarySystemModel.DeadZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Hot Zone          - Inner: %f, Outer: %f AU"), PlanetarySystemModel.HotZoneRadius.InnerRadius, PlanetarySystemModel.HotZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Warm Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel.WarmZoneRadius.InnerRadius, PlanetarySystemModel.WarmZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Habitable Zone    - Inner: %f, Outer: %f AU"), PlanetarySystemModel.HabitableZoneRadius.InnerRadius, PlanetarySystemModel.HabitableZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Cold Zone         - Inner: %f, Outer: %f AU"), PlanetarySystemModel.ColdZoneRadius.InnerRadius, PlanetarySystemModel.ColdZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Ice Zone          - Inner: %f, Outer: %f AU"), PlanetarySystemModel.IceZoneRadius.InnerRadius, PlanetarySystemModel.IceZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Gas Giants Zone   - Inner: %f, Outer: %f AU"), PlanetarySystemModel.GasGiantsZoneRadius.InnerRadius, PlanetarySystemModel.GasGiantsZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Kuiper Belt Zone  - Inner: %f, Outer: %f AU"), PlanetarySystemModel.KuiperBeltZoneRadius.InnerRadius, PlanetarySystemModel.KuiperBeltZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Inner Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.InnerPlanetZoneRadius.InnerRadius, PlanetarySystemModel.InnerPlanetZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Outer Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.OuterPlanetZoneRadius.InnerRadius, PlanetarySystemModel.OuterPlanetZoneRadius.OuterRadius);
    


        /// TODO: To PanetGenerator
        // populate planets list by new PlanetModel

        for (double OrbitRadius : OrbitRadii)
        {
            int PlanetIndex = OrbitRadii.IndexOfByKey(OrbitRadius);

            FPlanetGenerationModel PlanetModel;// = GeneratePlanet(PlanetarySystemModel, OrbitRadius);
            PlanetModel.OrbitDistance = OrbitRadius;

            // planet temperature
            //double PlanetTemperature = 0;
            double PlanetTemperature = StarModel.SurfaceTemperature * sqrt(StarModel.Radius / (2 * OrbitRadius));
            PlanetModel.Temperature = PlanetTemperature; // to celestial body

            // Определите, в какой зоне находится планета.
            EPlanetaryZoneType PlanetZone = DeterminePlanetZone(OrbitRadius, PlanetarySystemModel);
            PlanetModel.PlanetZone = PlanetZone;

            EPlanetType PlanetType = DeterminePlanetType(PlanetZone);
            PlanetModel.PlanetType = PlanetType;

            FDensityRange PlanetDensityRange = GetPlanetDensityRange(PlanetType);
            double PlanetDensity = FMath::RandRange(PlanetDensityRange.MinDensity, PlanetDensityRange.MaxDensity);
            PlanetModel.PlanetDensity = PlanetDensity;

            // assuming that radius is random in the range 1 - 2 Earth radii (this can be adjusted)
            FRadiusRange PlanetRadiusRange = GetPlanetRadiusRange(PlanetType);
            double PlanetRadius = FMath::RandRange(PlanetRadiusRange.MinRadius, PlanetRadiusRange.MaxRadius);
            PlanetModel.Radius = PlanetRadius;

            // now calculate mass based on density and radius
            PlanetModel.Mass = PlanetDensity * (4.0 / 3.0) * PI * FMath::Pow(PlanetRadius, 3);

            const double EARTH_RADIUS_KM = 6371.0; // радиус Земли в километрах
            double RadiusKM = PlanetRadius * EARTH_RADIUS_KM;
            PlanetModel.RadiusKM = RadiusKM;

            // for Gravity
            PlanetModel.PlanetGravityStrength = PlanetModel.Mass / FMath::Pow(PlanetModel.Radius, 2);
            PlanetModel.AmountOfMoons = CalculateMoons(PlanetModel.Mass, PlanetType);

//            PlanetarySystemModel.PlanetsList.Add(PlanetData);



            // Moons orbits
            // Гипотетические параметры планеты и звезды
            double planetMass = PlanetModel.Mass; // масса планеты
            double starMass = StarModel.Mass; // масса звезды
            double planetToStarDistance = PlanetModel.Radius; // среднее расстояние от планеты до звезды
            double planetRadius = PlanetModel.Radius; // радиус планеты
            //double planetAtmosphereHeight = PlanetModel.AtmosphereHeight; // высота атмосферы планеты
            double planetAtmosphereHeight = PlanetModel.Radius / 10; // высота атмосферы планеты

            // Минимальное расстояние - радиус планеты плюс высота атмосферы
            const double MinOrbitRadius = planetRadius + planetAtmosphereHeight;

            // Максимальное расстояние - радиус сферы Хилла
            // Предполагаем, что орбита планеты почти круговая, т.е. эксцентриситет близок к 0
            double eccentricity = 0;
            double RadiusHill = planetToStarDistance * (1 - eccentricity) * pow(planetMass / (3 * starMass), 1.0 / 3);
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

            PlanetModel.MoonOrbitsRange = OrbitRadiusPair;
            UE_LOG(LogTemp, Warning, TEXT("Planet Moons Orbit Radius    - Min: %f, Max: %f x"), OrbitRadiusPair.Key, OrbitRadiusPair.Value);

            //InitMoonGenerator();
            //FMoonGenerationModel MoonModel = MoonGenerator->GenerateMoonModel();

            
            const int AmountOfMoons = PlanetModel.AmountOfMoons;
            TArray<FMoonData> MoonsList {};
            TArray<float> MoonOrbits;

            const double MoonMinOrbitRadius = PlanetModel.MoonOrbitsRange.Key;
            const double MoonMaxOrbitRadius = PlanetModel.MoonOrbitsRange.Value;
            const double uniformDistance = (MoonMaxOrbitRadius - MoonMinOrbitRadius) / (AmountOfMoons + 1);

            // 2. Создаем массив со всеми возможными орбитами
            MoonOrbits.Reserve(AmountOfMoons);

            for (size_t m = 0; m < AmountOfMoons; m++)
            {
                MoonOrbits.Add(MoonMinOrbitRadius + uniformDistance * (m + 1));
            }

            for (double MoonOrbit : MoonOrbits)
            {
                FMoonGenerationModel MoonModel;

                EMoonType MoonType = MoonGenerator->GenerateMoonType(PlanetModel);

                // Вычисляем физические параметры луны
                double MoonMass = MoonGenerator->CalculateMoonMass(MoonType);
                double MoonRadius = MoonGenerator->CalculateMoonRadius(MoonType);

                // Создаем модель луны
                MoonModel.Mass = MoonMass;
                MoonModel.Radius = MoonRadius;
                MoonModel.Type = MoonType;
                MoonModel.OrbitDistance = MoonOrbit;

                // Создаем данные о луне
                int MoonIndex = MoonOrbits.IndexOfByKey(MoonOrbit);
                FMoonData MoonData(MoonIndex + 1, MoonOrbit, MoonModel);

                // Добавляем данные о луне в список
                MoonsList.Add(MoonData);


            }


            PlanetModel.MoonsList = MoonsList;
            FPlanetData PlanetData = FPlanetData(PlanetIndex, OrbitRadius, PlanetModel);
            PlanetarySystemModel.PlanetsList.Add(PlanetData);

            //MoonsList.Reset();
            //MoonOrbits.Reset();
        }

        //MoonsList.Reset();
    }
    else 
    {
		PlanetarySystemModel.AmountOfPlanets = 0;
		PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
	}


    OrbitRadii.Reset();

    return PlanetarySystemModel;
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
    else if (BaseMoonCount > 15)
    {

        return 
            PlanetType == EPlanetType::GasGiant ||
            PlanetType == EPlanetType::IceGiant ||
			PlanetType == EPlanetType::HotGiant ? FMath::RandRange(0, 15) : FMath::RandRange(0, 5);

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

EPlanetaryZoneType UPlanetarySystemGenerator::DeterminePlanetZone(double OrbitRadius, FPlanetarySystemGenerationModel PlanetarySystemModel)
{
    // TODO: To Struct
    /*for (FZone Zone : Zones)
    {
        if (OrbitRadius >= Zone.Radius.InnerRadius && OrbitRadius <= Zone.Radius.OuterRadius)
        {
            return Zone.ZoneType;
        }
    }*/

    if (OrbitRadius >= PlanetarySystemModel.DeadZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.DeadZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::DeadZone;
    }

    if (OrbitRadius >= PlanetarySystemModel.HotZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.HotZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::HotZone;
    }
    
    if (OrbitRadius >= PlanetarySystemModel.WarmZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.WarmZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::WarmZone;
    }

    if (OrbitRadius >= PlanetarySystemModel.HabitableZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.HabitableZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::HabitableZone;
    }

    if (OrbitRadius >= PlanetarySystemModel.ColdZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.ColdZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::ColdZone;
    }
    
    if (OrbitRadius >= PlanetarySystemModel.IceZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.IceZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::IceZone;
    }
    
    if (OrbitRadius >= PlanetarySystemModel.GasGiantsZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.GasGiantsZoneRadius.OuterRadius)
    {
        return EPlanetaryZoneType::GasGiantsZone;
    }
    
    if (OrbitRadius >= PlanetarySystemModel.KuiperBeltZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel.KuiperBeltZoneRadius.OuterRadius)
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


/// TODO:
//FPlanetarySystemGenerationModel UPlanetarySystemGenerator::GenerateRandomPlanetraySystemModelByStar(FStarGenerationModel StarModel)
//{
//    FPlanetarySystemGenerationModel PlanetarySystemModel;
//    EStellarClass StellarClass = StarModel.StellarClass;
//    EPlanetarySystemType PlanetarySystemType = GetRandomWithProbability(StarSystemTypeProbabilities[StellarClass].SystemTypeProbabilities);
//    PlanetarySystemModel.PlanetarySystemType = PlanetarySystemType;
//
//    // Получаем диапазон значений для данного типа системы
//    FPlanetarySystemRanges SystemRanges = PlanetarySystemRanges[PlanetarySystemType];
//
//    // Генерируем случайное значение в диапазоне
//    int AmountOfPlanets = FMath::RandRange(SystemRanges.AmountOfPlanetsRange.Get<0>(), SystemRanges.AmountOfPlanetsRange.Get<1>());
//    PlanetarySystemModel.AmountOfPlanets = AmountOfPlanets;
//
//    double DistanceBetweenPlanets = FMath::RandRange(SystemRanges.DistanceBetweenPlanetsRange.Get<0>(), SystemRanges.DistanceBetweenPlanetsRange.Get<1>());
//    PlanetarySystemModel.DistanceBetweenPlanets = DistanceBetweenPlanets;
//
//    return PlanetarySystemModel;
//}


/// TODO:
//FPlanetarySystemGenerationModel UPlanetarySystemGenerator::GenerateRandomPlanetraySystemModel()
//{
//    FPlanetarySystemGenerationModel PlanetarySystemModel;
//
//    int32 RandomValue = FMath::RandRange(0, 100);
//
//    // Веса для каждого типа системы
//    int32 NoPlanetSystemWeight = 10; // 10%
//    int32 SmallSystemWeight = 30; // 30%
//    int32 LargeSystemWeight = 25; // 25%
//    int32 ChaoticSystemWeight = 20; // 20%
//    int32 DenseSystemWeight = 15; // 15%
//
//    if (RandomValue < NoPlanetSystemWeight)
//    {
//        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
//        PlanetarySystemModel.AmountOfPlanets = 0;
//        PlanetarySystemModel.DistanceBetweenPlanets = 0.0;
//    }
//    else if (RandomValue < NoPlanetSystemWeight + SmallSystemWeight)
//    {
//        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::SmallSystem;
//        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(1, 5);
//        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(1.0, 3.0);
//    }
//    else if (RandomValue < NoPlanetSystemWeight + SmallSystemWeight + LargeSystemWeight)
//    {
//        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::LargeSystem;
//        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(6, 12);
//        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(1.0, 5.0);
//    }
//    else if (RandomValue < NoPlanetSystemWeight + SmallSystemWeight + LargeSystemWeight + ChaoticSystemWeight)
//    {
//        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::ChaoticSystem;
//        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(5, 12);
//        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.1, 2.0);
//    }
//    else
//    {
//        PlanetarySystemModel.PlanetarySystemType = EPlanetarySystemType::DenseSystem;
//        PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(3, 10);
//        PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.2, 1.0);
//    }
//
//    return PlanetarySystemModel;
//}
