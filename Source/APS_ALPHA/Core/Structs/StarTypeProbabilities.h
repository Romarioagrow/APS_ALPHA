#pragma once
#include "PlanetarySystemType.h"
struct StarTypeProbabilities
{
    TMap<EPlanetarySystemType, int32> SystemTypeProbabilities;

    StarTypeProbabilities(TMap<EPlanetarySystemType, int32> Probabilities)
    {
        SystemTypeProbabilities = Probabilities;
    }
};

//TMap<EStellarClass, StarTypeProbabilities> StarSystemTypeProbabilities =
//{
//    {EStellarClass::HyperGiant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 5}, {EPlanetarySystemType::LargeSystem, 45}, {EPlanetarySystemType::ChaoticSystem, 20}, {EPlanetarySystemType::DenseSystem, 25}})},
//    {EStellarClass::SuperGiant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 5}, {EPlanetarySystemType::LargeSystem, 40}, {EPlanetarySystemType::ChaoticSystem, 20}, {EPlanetarySystemType::DenseSystem, 30}})},
//    {EStellarClass::BrightGiant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 5}, {EPlanetarySystemType::LargeSystem, 40}, {EPlanetarySystemType::ChaoticSystem, 20}, {EPlanetarySystemType::DenseSystem, 30}})},
//    {EStellarClass::Giant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 10}, {EPlanetarySystemType::LargeSystem, 35}, {EPlanetarySystemType::ChaoticSystem, 25}, {EPlanetarySystemType::DenseSystem, 25}})},
//    {EStellarClass::SubGiant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 10}, {EPlanetarySystemType::LargeSystem, 35}, {EPlanetarySystemType::ChaoticSystem, 25}, {EPlanetarySystemType::DenseSystem, 25}})},
//    {EStellarClass::MainSequence, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 25}, {EPlanetarySystemType::LargeSystem, 35}, {EPlanetarySystemType::ChaoticSystem, 25}, {EPlanetarySystemType::DenseSystem, 10}})},
//    {EStellarClass::SubDwarf, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 50}, {EPlanetarySystemType::SmallSystem, 30}, {EPlanetarySystemType::LargeSystem, 10}, {EPlanetarySystemType::ChaoticSystem, 10}, {EPlanetarySystemType::DenseSystem, 0}})},
//    {EStellarClass::WhiteDwarf, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 50}, {EPlanetarySystemType::SmallSystem, 25}, {EPlanetarySystemType::LargeSystem, 10}, {EPlanetarySystemType::ChaoticSystem, 10}, {EPlanetarySystemType::DenseSystem, 5}})},
//    {EStellarClass::BrownDwarf, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 60}, {EPlanetarySystemType::SmallSystem, 30}, {EPlanetarySystemType::LargeSystem, 5}, {EPlanetarySystemType::ChaoticSystem, 5}, {EPlanetarySystemType::DenseSystem, 0}})},
//    {EStellarClass::Neutron, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 80}, {EPlanetarySystemType::SmallSystem, 10}, {EPlanetarySystemType::LargeSystem, 5}, {EPlanetarySystemType::ChaoticSystem, 5}, {EPlanetarySystemType::DenseSystem, 0}})},
//    {EStellarClass::Protostar, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 70}, {EPlanetarySystemType::SmallSystem, 20}, {EPlanetarySystemType::LargeSystem, 5}, {EPlanetarySystemType::ChaoticSystem, 5}, {EPlanetarySystemType::DenseSystem, 0}})},
//    {EStellarClass::Pulsar, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 90}, {EPlanetarySystemType::SmallSystem, 5}, {EPlanetarySystemType::LargeSystem, 2}, {EPlanetarySystemType::ChaoticSystem, 3}, {EPlanetarySystemType::DenseSystem, 0}})},
//    {EStellarClass::BlackHole, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 95}, {EPlanetarySystemType::SmallSystem, 3}, {EPlanetarySystemType::LargeSystem, 1}, {EPlanetarySystemType::ChaoticSystem, 1}, {EPlanetarySystemType::DenseSystem, 0}})}
//};