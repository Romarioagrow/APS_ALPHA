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
    // Маппинг типов планет и диапазонов вероятностей для типов лун
    //TMap<EPlanetType, TMap<EMoonType, FFloatRange>> PlanetMoonTypeProbabilities;

    //// TODO: Заполните маппинг PlanetMoonTypeProbabilities на основе вашего знания о том, какие типы лун наиболее вероятно появляются у каждого типа планеты

    //// Получаем диапазон вероятностей для данного типа планеты
    //TMap<EMoonType, FFloatRange> MoonTypeProbabilities = PlanetMoonTypeProbabilities[PlanetModel.PlanetType];

    //// Генерируем случайное число от 0 до 1
    //float RandNum = FMath::FRand();

    //// Ищем соответствующий тип луны
    //for (const auto& Entry : MoonTypeProbabilities)
    //{
    //    if (Entry.Value.Contains(RandNum))
    //    {
    //        return Entry.Key;
    //    }
    //}

    // Если ничего не найдено, возвращаем Rocky по умолчанию
    return EMoonType::Rocky;
}

double UMoonGenerator::CalculateMoonMass(EMoonType MoonType)
{
    // Генерируем случайное число
    float randomValue = FMath::FRand();

    // Скалистые, железные и вулканические луны обычно имеют большую плотность
    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
    {
        return randomValue * 1e23; // масса в килограммах
    }

    // Ледяные и океанические луны обычно имеют меньшую плотность
    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
    {
        return randomValue * 1e22; // масса в килограммах
    }

    // Газовые луны и захваченные астероиды могут иметь различную массу
    else
    {
        return randomValue * 1e21; // масса в килограммах
    }
}

double UMoonGenerator::CalculateMoonRadius(EMoonType MoonType)
{
    // Генерируем случайное число
    float randomValue = FMath::FRand();

    // Скалистые, железные и вулканические луны обычно имеют большой радиус
    if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
    {
        return randomValue * 2000; // радиус в километрах
    }

    // Ледяные и океанические луны обычно имеют меньший радиус
    else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
    {
        return randomValue * 1500; // радиус в километрах
    }

    // Газовые луны и захваченные астероиды могут иметь различный радиус
    else
    {
        return randomValue * 1000; // радиус в километрах
    }
}