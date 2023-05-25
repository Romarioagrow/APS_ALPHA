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
    //     // ��������� �������
    //     FPlanetParams Params;
    //
    //     // ��������� �������
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
//    // 2. ������� ������ �� ����� ���������� ��������
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
//        // ��������� ���������� ��������� ����
//        double MoonMass = CalculateMoonMass(MoonType);
//        double MoonRadius = CalculateMoonRadius(MoonType);
//
//        // ������� ������ ����
//        MoonModel.Mass = MoonMass;
//        MoonModel.Radius = MoonRadius;
//        MoonModel.Type = MoonType;
//        MoonModel.OrbitDistance = MoonOrbit;
//
//        // ������� ������ � ����
//        int MoonIndex = MoonOrbits.IndexOfByKey(MoonOrbit);
//        FMoonData MoonData(MoonIndex + 1, MoonOrbit, MoonModel);
//
//        // ��������� ������ � ���� � ������
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
//    // ������� ����� ������ � ���������� ������������ ��� ����� ���
//    //TMap<EPlanetType, TMap<EMoonType, FFloatRange>> PlanetMoonTypeProbabilities;
//
//    //// TODO: ��������� ������� PlanetMoonTypeProbabilities �� ������ ������ ������ � ���, ����� ���� ��� �������� �������� ���������� � ������� ���� �������
//
//    //// �������� �������� ������������ ��� ������� ���� �������
//    //TMap<EMoonType, FFloatRange> MoonTypeProbabilities = PlanetMoonTypeProbabilities[PlanetModel.PlanetType];
//
//    //// ���������� ��������� ����� �� 0 �� 1
//    //float RandNum = FMath::FRand();
//
//    //// ���� ��������������� ��� ����
//    //for (const auto& Entry : MoonTypeProbabilities)
//    //{
//    //    if (Entry.Value.Contains(RandNum))
//    //    {
//    //        return Entry.Key;
//    //    }
//    //}
//
//    // ���� ������ �� �������, ���������� Rocky �� ���������
//    return EMoonType::Rocky;
//}
//
//double UPlanetGenerator::CalculateMoonMass(EMoonType MoonType)
//{
//    // ���������� ��������� �����
//    float randomValue = FMath::FRand();
//
//    // ���������, �������� � ������������� ���� ������ ����� ������� ���������
//    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
//    {
//        return randomValue * 1e23; // ����� � �����������
//    }
//
//    // ������� � ������������ ���� ������ ����� ������� ���������
//    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
//    {
//        return randomValue * 1e22; // ����� � �����������
//    }
//
//    // ������� ���� � ����������� ��������� ����� ����� ��������� �����
//    else
//    {
//        return randomValue * 1e21; // ����� � �����������
//    }
//}
//
//double UPlanetGenerator::CalculateMoonRadius(EMoonType MoonType)
//{
//    // ���������� ��������� �����
//    float randomValue = FMath::FRand();
//
//    // ���������, �������� � ������������� ���� ������ ����� ������� ������
//    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
//    {
//        return randomValue * 2000; // ������ � ����������
//    }
//
//    // ������� � ������������ ���� ������ ����� ������� ������
//    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
//    {
//        return randomValue * 1500; // ������ � ����������
//    }
//
//    // ������� ���� � ����������� ��������� ����� ����� ��������� ������
//    else
//    {
//        return randomValue * 1000; // ������ � ����������
//    }
//}