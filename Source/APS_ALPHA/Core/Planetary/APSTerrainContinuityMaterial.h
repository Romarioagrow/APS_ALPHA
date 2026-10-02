#pragma once

#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "HAL/IConsoleManager.h"

// Versioned rollout validated in menu and live gameplay on the listed types.
// Unvalidated and future types keep the original stack; the cvar is a rollback.
namespace APSTerrainContinuityMaterial
{
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.M_APS_ContinuousTerrain");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra.MI_APS_ContinuousTerra");
    inline bool Allows(EPlanetType Type)
    {
        return Type == EPlanetType::Terrestrial || Type == EPlanetType::Frozen
            || Type == EPlanetType::Oasis || Type == EPlanetType::Ice
            || Type == EPlanetType::Tundra || Type == EPlanetType::Nordic
            || Type == EPlanetType::Rocky || Type == EPlanetType::Desert
            || Type == EPlanetType::Sand || Type == EPlanetType::HighMountain
            || Type == EPlanetType::Forest || Type == EPlanetType::Savanna
            || Type == EPlanetType::SuperEarth || Type == EPlanetType::Pangea;
    }
    inline bool Enabled()
    {
        const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(
            TEXT("aps.Surface.TerrainContinuity"));
        return Variable && Variable->GetInt() != 0;
    }
}
