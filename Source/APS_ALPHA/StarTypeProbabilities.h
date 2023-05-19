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

TMap<EStarType, StarTypeProbabilities> StarSystemTypeProbabilities =
{
    {EStarType::MainSequence, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 25}, {EPlanetarySystemType::LargeSystem, 35}, {EPlanetarySystemType::ChaoticSystem, 25}, {EPlanetarySystemType::DenseSystem, 10}})},
    {EStarType::Giant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 10}, {EPlanetarySystemType::LargeSystem, 35}, {EPlanetarySystemType::ChaoticSystem, 25}, {EPlanetarySystemType::DenseSystem, 25}})},
    {EStarType::SuperGiant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 5}, {EPlanetarySystemType::LargeSystem, 40}, {EPlanetarySystemType::ChaoticSystem, 20}, {EPlanetarySystemType::DenseSystem, 30}})},
    {EStarType::WhiteDwarf, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 50}, {EPlanetarySystemType::SmallSystem, 25}, {EPlanetarySystemType::LargeSystem, 10}, {EPlanetarySystemType::ChaoticSystem, 10}, {EPlanetarySystemType::DenseSystem, 5}})},
    {EStarType::Neutron, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 80}, {EPlanetarySystemType::SmallSystem, 10}, {EPlanetarySystemType::LargeSystem, 5}, {EPlanetarySystemType::ChaoticSystem, 5}, {EPlanetarySystemType::DenseSystem, 0}})},
    {EStarType::BrownDwarf, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 60}, {EPlanetarySystemType::SmallSystem, 30}, {EPlanetarySystemType::LargeSystem, 5}, {EPlanetarySystemType::ChaoticSystem, 5}, {EPlanetarySystemType::DenseSystem, 0}})},
    {EStarType::RedDwarf, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 20}, {EPlanetarySystemType::SmallSystem, 40}, {EPlanetarySystemType::LargeSystem, 25}, {EPlanetarySystemType::ChaoticSystem, 10}, {EPlanetarySystemType::DenseSystem, 5}})},
    {EStarType::SubDwarf, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 50}, {EPlanetarySystemType::SmallSystem, 30}, {EPlanetarySystemType::LargeSystem, 10}, {EPlanetarySystemType::ChaoticSystem, 10}, {EPlanetarySystemType::DenseSystem, 0}})},
    {EStarType::HyperGiant, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 5}, {EPlanetarySystemType::SmallSystem, 5}, {EPlanetarySystemType::LargeSystem, 45}, {EPlanetarySystemType::ChaoticSystem, 20}, {EPlanetarySystemType::DenseSystem, 25}})},
    {EStarType::Protostar, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 70}, {EPlanetarySystemType::SmallSystem, 20}, {EPlanetarySystemType::LargeSystem, 5}, {EPlanetarySystemType::ChaoticSystem, 5}, {EPlanetarySystemType::DenseSystem, 0}})},
    {EStarType::Pulsar, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 90}, {EPlanetarySystemType::SmallSystem, 5}, {EPlanetarySystemType::LargeSystem, 2}, {EPlanetarySystemType::ChaoticSystem, 3}, {EPlanetarySystemType::DenseSystem, 0}})},
    {EStarType::BlackHole, StarTypeProbabilities({{EPlanetarySystemType::NoPlanetSystem, 95}, {EPlanetarySystemType::SmallSystem, 3}, {EPlanetarySystemType::LargeSystem, 1}, {EPlanetarySystemType::ChaoticSystem, 1}, {EPlanetarySystemType::DenseSystem, 0}})}
};