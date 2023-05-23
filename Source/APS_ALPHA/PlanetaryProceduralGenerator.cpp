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
    // ��������� ����������� ��� ����� �������
    // ������� ���� � ��� ��� �� ������
    // ���������� ������������ ����� ������  

    FPlanetarySystemGenerationModel PlanetarySystemModel;
    // ������� ������� ����������� ��� ������� ���� ������
    PlanetProbability BaseProbability = BasePlanetProbabilities[StarModel.StellarClass];
    // ������������ ����������� �� ������ ����� ������.
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

    // ������� ���������� � ������
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


        // ��� ����� ���� ��������� ��� ����������, ��������� �� ����������� ������
        double MinOrbitScalingFactor = 1.0f;
        double MaxOrbitScalingFactor = 10.0f;

        if (StarModel.StellarClass == EStellarClass::HyperGiant)
        {
            MaxOrbitScalingFactor = 5.0f; // ��������� ������������ ������ ��� �������������
        }
        else if (StarModel.StellarClass == EStellarClass::SuperGiant)
        {
            MaxOrbitScalingFactor = 6.0f; // ��������� ������������ ������ ��� �������������
        }

        double MinOrbit = StarModel.Mass * MinOrbitScalingFactor;
        double MaxOrbit = StarModel.Mass * MaxOrbitScalingFactor;

        // ��������� ��������� ������������� ��� ����� �������
        EOrbitDistributionType OrbitDistributionType = ChooseOrbitDistribution(StarModel.StellarClass);
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
                OrbitDistributionValue = RandGauss();
                break;
            case EOrbitDistributionType::Chaotic:
            {
                OrbitDistributionValue = FMath::RandRange(MinOrbit, MaxOrbit);
                double OrbitRadius = OrbitDistributionValue;
                OrbitRadii.Add(OrbitRadius);
                continue;
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

            // ��������� ������� ������������� � ������ ��������� �����
            double OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
            OrbitRadii.Add(OrbitRadius);
        }

        OrbitRadii.Sort();

        // ������� ����������� � ������������ ������
        UE_LOG(LogTemp, Warning, TEXT("MinOrbit: %f, MaxOrbit: %f"), MinOrbit, MaxOrbit);

        // ��������� ��������� ����
        float HabitableZoneInner = sqrt(StarModel.Luminosity / 1.1);
        float HabitableZoneOuter = sqrt(StarModel.Luminosity / 0.53);

        // Star Dead zone
        double StarDeadZoneInner = 0; // ���������� �� ������
        double StarRadiusInAU = StarModel.Radius * 0.00465047;
        double StarDeadZoneOuter = StarRadiusInAU * 2; // ������������� �� ����������, ������ �������� ������� ������ � AU

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
        double InnerZoneInner = 0; // ���������� �� ������
        double InnerZoneOuter = 0; // ������������� �������� ������� ����
        double OuterZoneInner = 0; // ���������� �� ������� ���� ������� ��������
        double OuterZoneOuter = 0; // ��������� �������

        if (HabitableZoneOuter < MaxOrbit)
        {
            // ��������� ���� StarDeadZone
            StarDeadZoneInner = 0; // ���������� �� ������
            StarRadiusInAU = StarModel.Radius * 0.00465047;
            StarDeadZoneOuter = StarRadiusInAU * 2; // ������������� �� ����������, ������ �������� ������� ������ � AU

            // ��������� ������� ����
            HotZoneInner = StarDeadZoneOuter;
            HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

            // ��������� ������ ����
            WarmZoneInner = HotZoneOuter;
            WarmZoneOuter = HabitableZoneInner;

            // ��������� �������� ����
            ColdZoneInner = HabitableZoneOuter;//FMath::Max(HabitableZoneOuter, MinOrbit);
            ColdZoneOuter = (ColdZoneInner * 2 > MaxOrbit) ? MaxOrbit : ColdZoneInner * 2;

            // ��������� ������� ����
            IceZoneInner = ColdZoneOuter;//FMath::Max(ColdZoneOuter, MinOrbit);
            IceZoneOuter = (IceZoneInner * 2 > MaxOrbit) ? MaxOrbit : IceZoneInner * 2;

            // ��������� ���� ������� ��������
            GasGiantsZoneInner = FMath::Max(IceZoneOuter, MinOrbit);
            GasGiantsZoneOuter = (GasGiantsZoneInner * 2 > MaxOrbit) ? MaxOrbit : GasGiantsZoneInner * 2;

            // ��������� ���� ����� �������
            KuiperBeltZoneInner = FMath::Max(GasGiantsZoneOuter, MinOrbit);
            KuiperBeltZoneOuter = (KuiperBeltZoneInner * 2 > MaxOrbit) ? MaxOrbit : KuiperBeltZoneInner * 2;


            if (OrbitDistributionType == EOrbitDistributionType::InnerOuter)
            {
                // ��������� ���������� ����
                InnerZoneInner = StarDeadZoneOuter; //0; // ���������� �� ������
                InnerZoneOuter = HotZoneOuter; // ������������� �������� ������� ����

                // ��������� ������� ����
                OuterZoneInner = GasGiantsZoneOuter; // ���������� �� ������� ���� ������� ��������
                OuterZoneOuter = OuterZoneInner * 2; // ��������� �������

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

        // ������� ��������� ����
        UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

        // ������� ����������� ������
        // ������� ��� ������ ������
        for (int i = 0; i < OrbitRadii.Num(); ++i)
        {
            UE_LOG(LogTemp, Warning, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
        }

        UE_LOG(LogTemp, Warning, TEXT("Radius Dead Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.DeadZoneRadius.InnerRadius, PlanetarySystemModel.DeadZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Hot Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.HotZoneRadius.InnerRadius, PlanetarySystemModel.HotZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Warm Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.WarmZoneRadius.InnerRadius, PlanetarySystemModel.WarmZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Habitable Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.HabitableZoneRadius.InnerRadius, PlanetarySystemModel.HabitableZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Cold Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.ColdZoneRadius.InnerRadius, PlanetarySystemModel.ColdZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Ice Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.IceZoneRadius.InnerRadius, PlanetarySystemModel.IceZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Gas Giants Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.GasGiantsZoneRadius.InnerRadius, PlanetarySystemModel.GasGiantsZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Kuiper Belt Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.KuiperBeltZoneRadius.InnerRadius, PlanetarySystemModel.KuiperBeltZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Inner Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.InnerPlanetZoneRadius.InnerRadius, PlanetarySystemModel.InnerPlanetZoneRadius.OuterRadius);
        UE_LOG(LogTemp, Warning, TEXT("Radius Outer Planet Zone - Inner: %f, Outer: %f AU"), PlanetarySystemModel.OuterPlanetZoneRadius.InnerRadius, PlanetarySystemModel.OuterPlanetZoneRadius.OuterRadius);
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
        // ���� � ��� ��������� ������, �� ���������� Dense
        OrbitDistributionType = EOrbitDistributionType::Dense;
    }
    else if (StellarClass == EStellarClass::Giant || StellarClass == EStellarClass::SuperGiant || StellarClass == EStellarClass::HyperGiant)
    {
        // ���� � ��� ������� ������, �� ���������� InnerOuter
        OrbitDistributionType = EOrbitDistributionType::InnerOuter;
    }
    else
    {
        // ���� � ��� ������ �������� �������, ����� ������������ ������ ���� ������������� � ����������� �� �����
        if (StarMass < 0.5f)
        {
            OrbitDistributionType = EOrbitDistributionType::Uniform;
        }
        else if (StarMass < 1.0f)
        {
            OrbitDistributionType = EOrbitDistributionType::Gaussian;
        }
        else if (MaxOrbit - MinOrbit < 5.0f) // ��������, ��������� ������ ���� � ����� ����������� � ������������ ������
        {
            OrbitDistributionType = EOrbitDistributionType::Dense; // ���� ������ ������, ����� ������������ ������� �������������
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
    // �������� ���� ������������ ��� ������� ������ ������
    auto probabilities = StellarOrbitDistributions[StellarClass];

    // ���������� ��������� ����� � ��������� �� 0 �� 1
    float randomValue = FMath::FRand();

    // ���� �� ���� ����� ����-�������� � ������� ������������
    for (auto& keyValue : probabilities)
    {
        // ��������� ��������� ����� �� ����������� �������� ����
        randomValue -= keyValue.Value;

        // ���� ��������� ����� ����� ������ ����, �������� ������� ���
        if (randomValue < 0)
        {
            return keyValue.Key;
        }
    }

    // ���������� ��������� ���, ���� ���-�� ����� �� ���
    return probabilities.end().Key();
}

int UPlanetarySystemGenerator::DetermineMaxPlanets(EStellarClass StellarClass, FStarGenerationModel StarModel )
{

    int MaxPlanets;

    // � ��������� ����� ����� ������ ��� ������
    if (StellarClass == EStellarClass::WhiteDwarf || StellarClass == EStellarClass::Neutron) {
        return 0;
    }

    // ������ ������� ���������� ������ � ����������� �� ������ ������
    switch (StellarClass) {
    case EStellarClass::HyperGiant:
    case EStellarClass::SuperGiant:
    case EStellarClass::Giant:
        MaxPlanets = 10; // ������� ���������� ��� ���������� �����
        break;
    case EStellarClass::MainSequence:
        MaxPlanets = 5; // ������� ���������� ��� ����� ������� ������������������
        break;
    default:
        MaxPlanets = 3; // ������� ���������� ��� ��������� ����� �����
        break;
    }

    // ������������ ���������� ������ � ����������� �� ����� � �������� ������
    MaxPlanets = MaxPlanets + StarModel.Mass * 0.5 + 0.2 * 0.1;

    // ��������, ��� � ��� ���� ���� �� ���� �������, ���� ��� ��������
    /*if (MaxPlanets < 1 && StellarClass != EStellarClass::WhiteDwarf && StellarClass != EStellarClass::NeutronStar) {
        MaxPlanets = 1;
    }*/

    // ��������� ������������ ���������� ������, ����� ��� ���� ��������
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

    // �������� �������� �������� ��� ������� ���� �������
    FPlanetarySystemRanges SystemRanges = PlanetarySystemRanges[PlanetarySystemType];

    // ���������� ��������� �������� � ���������
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

    // ���� ��� ������� ���� �������
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
