// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetaryProceduralGenerator.h"
#include "AstroActor.h"
#include "StarTypeProbabilities.h"
#include "PlanetarySystem.h"
#include "PlanetarySystemGenerationModel.h"

void UPlanetarySystemGenerator::ApplyModel(APlanetarySystem* NewPlanetarySystem, FPlanetarySystemGenerationModel PlanetraySystemModel)
{
    NewPlanetarySystem->SetAmountOfPlanets(PlanetraySystemModel.AmountOfPlanets);
    NewPlanetarySystem->SetPlanetarySystemType(PlanetraySystemModel.PlanetarySystemType);
}




FPlanetarySystemGenerationModel UPlanetarySystemGenerator::GeneratePlanetraySystemModelByStar(FStarGenerationModel StarModel)
{
    // вычисл€ем веро€тность что будут планеты
    // находим макс и мин кол во планет
    // опредле€ем расположение орбит планет  

    FPlanetarySystemGenerationModel PlanetarySystem;
    // Ќаходим базовую веро€тность дл€ данного типа звезды
    PlanetProbability BaseProbability = BasePlanetProbabilities[StarModel.StellarClass];
    // ћодифицируем веро€тность на основе массы звезды.
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
        PlanetarySystem.AmountOfPlanets = FinalPlanetCount;
        PlanetarySystem.PlanetarySystemType = EPlanetarySystemType::Unknown; ///


        UE_LOG(LogTemp, Warning, TEXT("MinPlanetCount: %d"), MinPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("MaxPlanetCount: %d"), MaxPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("FinalPlanetCount: %d"), FinalPlanetCount);
	}
    else 
    {
		PlanetarySystem.AmountOfPlanets = 0;
		PlanetarySystem.PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
	}
    
    return PlanetarySystem;
}

int UPlanetarySystemGenerator::DetermineMaxPlanets(EStellarClass StellarClass, FStarGenerationModel StarModel )
{

    int MaxPlanets;

    // ” некоторых типов звезд вообще нет планет
    if (StellarClass == EStellarClass::WhiteDwarf || StellarClass == EStellarClass::Neutron) {
        return 0;
    }

    // «адаем базовое количество планет в зависимости от класса звезды
    switch (StellarClass) {
    case EStellarClass::HyperGiant:
    case EStellarClass::SuperGiant:
    case EStellarClass::Giant:
        MaxPlanets = 10; // Ѕазовое количество дл€ гигантских звезд
        break;
    case EStellarClass::MainSequence:
        MaxPlanets = 5; // Ѕазовое количество дл€ звезд главной последовательности
        break;
    default:
        MaxPlanets = 3; // Ѕазовое количество дл€ остальных типов звезд
        break;
    }

    // ћодифицируем количество планет в зависимости от массы и возраста звезды
    MaxPlanets = MaxPlanets + StarModel.Mass * 0.5 + 0.2 * 0.1;

    // ”бедимс€, что у нас есть хот€ бы одна планета, если это возможно
    /*if (MaxPlanets < 1 && StellarClass != EStellarClass::WhiteDwarf && StellarClass != EStellarClass::NeutronStar) {
        MaxPlanets = 1;
    }*/

    // ќграничим максимальное количество планет, чтобы оно было разумным
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

    // ѕолучаем диапазон значений дл€ данного типа системы
    FPlanetarySystemRanges SystemRanges = PlanetarySystemRanges[PlanetarySystemType];

    // √енерируем случайное значение в диапазоне
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

    // ¬еса дл€ каждого типа системы
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
