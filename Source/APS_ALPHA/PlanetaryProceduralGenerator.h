// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "PlanetarySystemGenerationModel.h"
#include "StarGenerationModel.h"
#include "PlanetarySystem.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "PlanetaryProceduralGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UPlanetarySystemGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	
    void ApplyModel(APlanetarySystem* NewPlanetarySystem, FPlanetarySystemGenerationModel PlanetraySystemModel);

    FPlanetarySystemGenerationModel GenerateRandomPlanetraySystemModelByStar(FStarGenerationModel StarModel);

    int DetermineMaxPlanets(EStellarClass StellarClass, double StarMass);

    int DetermineMaxPlanets(EStellarClass StellarClass, FStarGenerationModel StarModel);

    FPlanetarySystemGenerationModel GeneratePlanetraySystemModelByStar(FStarGenerationModel StarModel);

	FPlanetarySystemGenerationModel GenerateRandomPlanetraySystemModel(); 
    

typedef float PlanetProbability;

private:
    TMap<EStellarClass, int> BasePlanetCount = {
        {EStellarClass::HyperGiant, 15},
        {EStellarClass::SuperGiant, 10},
        {EStellarClass::BrightGiant, 10},
        {EStellarClass::Giant, 7},
        {EStellarClass::SubGiant, 4},
        {EStellarClass::MainSequence, 10},
        {EStellarClass::SubDwarf, 3},
        {EStellarClass::WhiteDwarf, 2},
        {EStellarClass::BrownDwarf, 0},
        {EStellarClass::Neutron, 1},
        {EStellarClass::Protostar, 2},
        {EStellarClass::Pulsar, 1},
        {EStellarClass::BlackHole, 0},
        
    //HyperGiant		UMETA(DisplayName = "Hypergiant"), // Гипергиганты (экстремально большие звезды)
    //SuperGiant		UMETA(DisplayName = "Super Giant"), // Супергиганты (наибольшие звезды)
    //BrightGiant		UMETA(DisplayName = "Bright Giant"), // BrightGiant
    //Giant			UMETA(DisplayName = "Giant"), // Гиганты (большие звезды)
    //SubGiant		UMETA(DisplayName = "SubGiant"), // SubGiant 
    //MainSequence	UMETA(DisplayName = "Main Sequence"), // Главная последовательность (как наше Солнце)
    //SubDwarf		UMETA(DisplayName = "Subdwarf"), // Субкарлики (звезды с малой яркостью и размером)
    //WhiteDwarf		UMETA(DisplayName = "White Dwarf"), // Белые карлики (маленькие, горячие звезды)
    //BrownDwarf		UMETA(DisplayName = "Brown Dwarf"), // Коричневые карлики (не достаточно массы для начала термоядерных реакций)
    //Neutron			UMETA(DisplayName = "Neutron Star"), // Нейтронные звезды (остатки взрывов суперновых, очень плотные)
    //Protostar		UMETA(DisplayName = "Protostar"), // Протозвезды (молодые звезды, которые еще не начали основную термоядерную реакцию)
    //Pulsar			UMETA(DisplayName = "Pulsar"), // Пульсары (вращающиеся нейтронные звезды, которые излучают импульсы радиоволн)
    //BlackHole		UMETA(DisplayName = "Black Hole"), // Черные дыры (объекты с такой сильной гравитацией, что даже свет не может уйти)
    //Unknown			UMETA(DisplayName = "Black Hole")
    //// Добавьте остальные классы звезд по аналогии...
};

    TMap<EStellarClass, PlanetProbability> BasePlanetProbabilities = 
    {
        {EStellarClass::HyperGiant, 0.7f},
        {EStellarClass::SuperGiant, 0.7f},
        {EStellarClass::BrightGiant, 0.7f},
        {EStellarClass::Giant, 0.65f},
        {EStellarClass::SubGiant, 0.6f},
        {EStellarClass::MainSequence, 0.7f},
        {EStellarClass::SubDwarf, 0.3f},
        {EStellarClass::WhiteDwarf, 0.1f},
        {EStellarClass::BrownDwarf, 0.05f},
        {EStellarClass::Neutron, 0.01f},
        {EStellarClass::Protostar, 0.6f},
        {EStellarClass::Pulsar, 0.01f},
        {EStellarClass::BlackHole, 0.01f}
    };



    template <typename T>
    T GetRandomWithProbability(const TMap<T, int32>& Probabilities)
    {
        int32 TotalWeight = 0;
        for (auto const& Pair : Probabilities) {
            TotalWeight += Pair.Value;
        }

        int32 RandomValue = FMath::RandRange(0, TotalWeight - 1);

        for (auto const& Pair : Probabilities) {
            if (RandomValue < Pair.Value) {
                return Pair.Key;
            }
            RandomValue -= Pair.Value;
        }

        // В случае ошибки возвращаем первый элемент в словаре
        return Probabilities.CreateConstIterator().Key();
    }

    struct FPlanetarySystemRanges
    {
        TTuple<int, int> AmountOfPlanetsRange;
        TTuple<double, double> DistanceBetweenPlanetsRange;

        FPlanetarySystemRanges(TTuple<int, int> Planets, TTuple<double, double> Distance)
            : AmountOfPlanetsRange(Planets), DistanceBetweenPlanetsRange(Distance) {}
    };

    TMap<EPlanetarySystemType, FPlanetarySystemRanges> PlanetarySystemRanges =
    {
        {EPlanetarySystemType::NoPlanetSystem, FPlanetarySystemRanges(TTuple<int, int>(0, 0), TTuple<double, double>(0.0, 0.0))},
        {EPlanetarySystemType::SmallSystem, FPlanetarySystemRanges(TTuple<int, int>(1, 5), TTuple<double, double>(1.0, 3.0))},
        {EPlanetarySystemType::LargeSystem, FPlanetarySystemRanges(TTuple<int, int>(6, 12), TTuple<double, double>(1.0, 5.0))},
        {EPlanetarySystemType::ChaoticSystem, FPlanetarySystemRanges(TTuple<int, int>(5, 12), TTuple<double, double>(0.1, 2.0))},
        {EPlanetarySystemType::DenseSystem, FPlanetarySystemRanges(TTuple<int, int>(3, 10), TTuple<double, double>(0.2, 1.0))}
    };


};
