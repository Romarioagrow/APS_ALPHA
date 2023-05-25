// Fill out your copyright notice in the Description page of Project Settings.


#include "MoonGenerator.h"

UMoonGenerator::UMoonGenerator()
{
}

FMoonGenerationModel UMoonGenerator::GenerateMoonModel()
{
	return FMoonGenerationModel();
}

FMoonGenerationModel UMoonGenerator::GenerateRandomMoonModel()
{
	
	return FMoonGenerationModel();
}

void UMoonGenerator::ApplyModel(AAstroActor* AstroActor, FGenerationModel GenerationModel)
{
}


EMoonType UMoonGenerator::GenerateMoonType(FPlanetGenerationModel PlanetModel)
{
    // ������� ����� ������ � ���������� ������������ ��� ����� ���
    //TMap<EPlanetType, TMap<EMoonType, FFloatRange>> PlanetMoonTypeProbabilities;

    //// TODO: ��������� ������� PlanetMoonTypeProbabilities �� ������ ������ ������ � ���, ����� ���� ��� �������� �������� ���������� � ������� ���� �������

    //// �������� �������� ������������ ��� ������� ���� �������
    //TMap<EMoonType, FFloatRange> MoonTypeProbabilities = PlanetMoonTypeProbabilities[PlanetModel.PlanetType];

    //// ���������� ��������� ����� �� 0 �� 1
    //float RandNum = FMath::FRand();

    //// ���� ��������������� ��� ����
    //for (const auto& Entry : MoonTypeProbabilities)
    //{
    //    if (Entry.Value.Contains(RandNum))
    //    {
    //        return Entry.Key;
    //    }
    //}

    // ���� ������ �� �������, ���������� Rocky �� ���������
    return EMoonType::Rocky;
}

double UMoonGenerator::CalculateMoonMass(EMoonType MoonType)
{
    // ���������� ��������� �����
    float randomValue = FMath::FRand();

    // ���������, �������� � ������������� ���� ������ ����� ������� ���������
    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
    {
        return randomValue * 1e23; // ����� � �����������
    }

    // ������� � ������������ ���� ������ ����� ������� ���������
    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
    {
        return randomValue * 1e22; // ����� � �����������
    }

    // ������� ���� � ����������� ��������� ����� ����� ��������� �����
    else
    {
        return randomValue * 1e21; // ����� � �����������
    }
}

double UMoonGenerator::CalculateMoonRadius(EMoonType MoonType)
{
    // ���������� ��������� �����
    float randomValue = FMath::FRand();

    // ���������, �������� � ������������� ���� ������ ����� ������� ������
    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
    {
        return randomValue * 2000; // ������ � ����������
    }

    // ������� � ������������ ���� ������ ����� ������� ������
    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
    {
        return randomValue * 1500; // ������ � ����������
    }

    // ������� ���� � ����������� ��������� ����� ����� ��������� ������
    else
    {
        return randomValue * 1000; // ������ � ����������
    }
}