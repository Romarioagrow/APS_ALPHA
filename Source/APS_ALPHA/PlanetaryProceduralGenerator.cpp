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

    FPlanetarySystemGenerationModel PlanetarySystem;
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
        PlanetarySystem.AmountOfPlanets = FinalPlanetCount;
        PlanetarySystem.PlanetarySystemType = EPlanetarySystemType::Unknown; ///


        UE_LOG(LogTemp, Warning, TEXT("MinPlanetCount: %d"), MinPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("MaxPlanetCount: %d"), MaxPlanetCount);
        UE_LOG(LogTemp, Warning, TEXT("FinalPlanetCount: %d"), FinalPlanetCount);



        // orbits

        //float MinOrbit = StarModel.Mass; // ����� ������ ��������� ������� �� ����� ������, ��������, ����� ����������������
        //float MaxOrbit = StarModel.Mass * 10; // ��������, ������������ ������ ��������������� ����� ������

        //// ���������� ������������� ����� � ����������� �� ���� ������
        //float OrbitCoefficient;
        //switch (StarModel.StellarClass)
        //{
        //case EStellarClass::MainSequence:
        //    OrbitCoefficient = 1.5f; // ��� ����� ������� ������������������ ������ ������������� � �������������� ����������
        //    break;
        //case EStellarClass::HyperGiant:
        //case EStellarClass::SuperGiant:
        //case EStellarClass::BrightGiant:
        //case EStellarClass::Giant:
        //    //OrbitCoefficient = 1.0f; // ��� ���������� ����� ������ �� ���������� ����������
        //    //OrbitCoefficient = FMath::Exp(FMath::Clamp(StarModel.Mass / 10, 0.1f, 2.0f)); // exponential function, clamped
        //    OrbitCoefficient = FMath::Max(1, FMath::Exp(FMath::Clamp(StarModel.Mass / 10, 0.1f, 2.0f))); // exponential function, clamped
        //    break;
        //case EStellarClass::WhiteDwarf://
        //case EStellarClass::BrownDwarf://
        //case EStellarClass::SubDwarf:

        //    OrbitCoefficient = FMath::LogX(2, StarModel.Mass); // log2(mass)
        //    break;
        //default:
        //    OrbitCoefficient = FMath::RandRange(0.5f, 2.0f); // ��� ���� ��������� ����� ������ ��������
        //    break;
        //}

        //// ������� ������ ��� �������� �������� ����� ������
        //TArray<float> PlanetOrbits;
        //float CurrentOrbit = MinOrbit;
        //for (int i = 0; i < FinalPlanetCount; ++i)
        //{
        //    PlanetOrbits.Add(CurrentOrbit);
        //    CurrentOrbit *= OrbitCoefficient;
        //    if (CurrentOrbit > MaxOrbit)
        //    {
        //        break; // ����������, ���� ������� ������ ��������� ������������ ����������
        //    }
        //}



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


        //float MinOrbit = StarModel.Mass; // ����� ������ ��������� ������� �� ����� ������, ��������, ����� ����������������
        //float MaxOrbit = StarModel.Mass * 10; //// 
        // ��������� ��������� ������������� ��� ����� �������
        //EOrbitDistributionType OrbitDistributionType = ChooseDistributionType(StarModel.StellarClass, StarModel.Mass, MinOrbit, MaxOrbit);//static_cast<EOrbitDistributionType>(FMath::RandRange(0, 2));
        EOrbitDistributionType OrbitDistributionType = ChooseOrbitDistribution(StarModel.StellarClass);//ChooseDistributionType(StarModel.StellarClass, StarModel.Mass, MinOrbit, MaxOrbit);//static_cast<EOrbitDistributionType>(FMath::RandRange(0, 2));
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
                OrbitDistributionValue = FMath::PerlinNoise1D(i * 0.1);//* 0.5 + 0.5; 
                break;
            //case EOrbitDistributionType::InnerOuter:
            case EOrbitDistributionType::InnerOuter:
            {

                if (i < FinalPlanetCount / 2.0) {
                    OrbitDistributionValue = FMath::RandRange(0.01, 0.5);
                }
                else {
                    OrbitDistributionValue = FMath::RandRange(0.5, 1.0);
                }
                break;

                /*if (i < FinalPlanetCount / 2.0) {
                    OrbitDistributionValue = static_cast<double>(i) / (FinalPlanetCount / 2.0);
                }
                else {
                    OrbitDistributionValue = 0.5 + static_cast<double>(i - FinalPlanetCount / 2.0) / (FinalPlanetCount / 2.0);
                }
                break;*/

                /*if (i < FinalPlanetCount / 2) {
                    OrbitDistributionValue = static_cast<double>(i) / (FinalPlanetCount / 2);
                }
                else {
                    OrbitDistributionValue = 0.5 + static_cast<double>(i - FinalPlanetCount / 2) / (FinalPlanetCount / 2);
                }
                break;*/
                
                /*double MeanOrbit = (MinOrbit + MaxOrbit) / 2.0;
                if (i < FinalPlanetCount / 2) {
                    OrbitDistributionValue = FMath::RandRange(MinOrbit, MeanOrbit);
                }
                else {
                    OrbitDistributionValue = FMath::RandRange(MeanOrbit, MaxOrbit);
                }
                OrbitRadii.Add(OrbitDistributionValue);
                continue;*/
                /*if (i < FinalPlanetCount / 2) {
                    double MeanOrbit = (MinOrbit + MaxOrbit) / 2;
                    OrbitDistributionValue = FMath::RandRange(MinOrbit, MeanOrbit);
                }
                else {
                    double MeanOrbit = (MinOrbit + MaxOrbit) / 2;
                    OrbitDistributionValue = FMath::RandRange(MeanOrbit, MaxOrbit);
                }
                break;*/
            }
            case EOrbitDistributionType::Dense:
                OrbitDistributionValue = FMath::RandRange(0.01, 0.5);
                break;
            }

            // ��������� ������� ������������� � ������ ��������� �����
            double OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
            OrbitRadii.Add(OrbitRadius);

        }


        //// �������� ������� ������������� �� ������ ���������� ����
        //TArray<double> OrbitRadii;
        //for (int i = 0; i < FinalPlanetCount; i++)
        //{
        //    double OrbitDistributionValue;
        //    switch (OrbitDistributionType)
        //    {
        //    case EOrbitDistributionType::Uniform:
        //        OrbitDistributionValue = FMath::RandRange(0.0, 1.0);
        //        break;
        //    case EOrbitDistributionType::Gaussian:
        //        OrbitDistributionValue = FMath::RandGauss(0.5, 0.15);
        //        break;
        //    case EOrbitDistributionType::Chaotic:
        //        OrbitDistributionValue = FMath::PerlinNoise1D(i * 0.1);
        //        break;
        //    }

        //    // ��������� ������� ������������� � ������ ��������� �����
        //    double OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
        //    OrbitRadii.Add(OrbitRadius);
        //}

        //// ��������� ������� �����, ����� �������������, ��� ��� ������ �������������
        OrbitRadii.Sort();


        


        // ������� ����������� � ������������ ������
        UE_LOG(LogTemp, Warning, TEXT("MinOrbit: %f, MaxOrbit: %f"), MinOrbit, MaxOrbit);

        // ��������� ��������� ����
        float HabitableZoneInner = sqrt(StarModel.Luminosity / 1.1);
        float HabitableZoneOuter = sqrt(StarModel.Luminosity / 0.53);
        // ������� ��������� ����
        UE_LOG(LogTemp, Warning, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

        // ������� ����������� ������
        //UE_LOG(LogTemp, Warning, TEXT("OrbitDistributionValue: %f"), OrbitDistributionValue);
        // ������� ��� ������ ������
        for (int i = 0; i < OrbitRadii.Num(); ++i)
        {
            UE_LOG(LogTemp, Warning, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
        }
	}
    else 
    {
		PlanetarySystem.AmountOfPlanets = 0;
		PlanetarySystem.PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
	}
    
    return PlanetarySystem;
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
