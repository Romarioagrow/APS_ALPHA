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
        //PlanetarySystemModel.HabitableZoneRadius = TPair<double, double>(HabitableZoneInner, HabitableZoneOuter);

        // ��������� �������� ����
        double ColdZoneInner = HabitableZoneOuter;
        double ColdZoneOuter = ColdZoneInner * 2; // ��������� �������
       // PlanetarySystemModel.ColdZoneRadius = TPair<double, double>(ColdZoneInner, ColdZoneOuter);

        // ��������� ������� ����
        double IceZoneInner = ColdZoneOuter;
        double IceZoneOuter = IceZoneInner * 2; // ��������� �������
       // PlanetarySystemModel.IceZoneRadius = TPair<double, double>(IceZoneInner, IceZoneOuter);

        // ��������� ������ ����
        double WarmZoneInner = HabitableZoneInner / 2; // ��������� �������
        double WarmZoneOuter = HabitableZoneInner;
       // PlanetarySystemModel.WarmZoneRadius = TPair<double, double>(WarmZoneInner, WarmZoneOuter);

        // ��������� ������� ����
        double HotZoneInner = WarmZoneInner / 2; // ��������� �������
        double HotZoneOuter = WarmZoneInner;
     //   PlanetarySystemModel.HotZoneRadius = TPair<double, double>(HotZoneInner, HotZoneOuter);

        // ��������� ���� ������� ��������
        double GasGiantsZoneInner = IceZoneOuter; // ��������� �������
        double GasGiantsZoneOuter = GasGiantsZoneInner * 2; // ��������� �������
      //  PlanetarySystemModel.GasGiantsZoneRadius = TPair<double, double>(GasGiantsZoneInner, GasGiantsZoneOuter);

        // ��������� ���� ����� ����������
        TArray<TPair<double, double>> AsteroidBeltZoneRadius;
        // ����� �� ������ �������� ������ ���� ���������� � ������

        // ��������� ���� ����� �������
        double KuiperBeltZoneInner = GasGiantsZoneOuter; // ��������� �������
        double KuiperBeltZoneOuter = KuiperBeltZoneInner * 2; // ��������� �������
        //PlanetarySystemModel.KuiperBeltZoneRadius = TPair<double, double>(KuiperBeltZoneInner, KuiperBeltZoneOuter);

        if (OrbitDistributionType == EOrbitDistributionType::InnerOuter)
        {
            // ��������� ���������� ����
            double InnerZoneInner = 0; // ���������� �� ������
            double InnerZoneOuter = HotZoneOuter; // ������������� �������� ������� ����
          //  PlanetarySystemModel.InnerZoneRadius = TPair<double, double>(InnerZoneInner, InnerZoneOuter);

            // ��������� ������� ����
            double OuterZoneInner = GasGiantsZoneOuter; // ���������� �� ������� ���� ������� ��������
            double OuterZoneOuter = OuterZoneInner * 2; // ��������� �������
          //  PlanetarySystemModel.OuterZoneRadius = TPair<double, double>(OuterZoneInner, OuterZoneOuter);
        }


        // ������� ��������� ����
        UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

        // ������� ����������� ������
        // ������� ��� ������ ������
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

//EOrbitDistributionType UPlanetarySystemGenerator::ChooseDistributionType(EStellarClass StellarClass, float StarMass, float OrbitRange)
//{
//    if (StellarClass == EStellarClass::Giant || StellarClass == EStellarClass::SuperGiant)
//    {
//        // ��� ���������� ����� �������� ��������� �������������
//        return EOrbitDistributionType::Chaotic;
//    }
//    else if (StarMass > 1.5f)
//    {
//        // ��� ����� ��������� ����� �������� �������� �������������
//        return EOrbitDistributionType::Gaussian;
//    }
//    else
//    {
//        // �� ���� ��������� ������� �������� ����������� �������������
//        return EOrbitDistributionType::Uniform;
//    }
//}

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
