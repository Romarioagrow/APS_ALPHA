#pragma once
#include "CoreMinimal.h"

namespace APSFoliageLeafMaterial
{
    inline constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliageLeaf20260930");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliageLeaf20260930/MI_APS_PrototypeLeaf.MI_APS_PrototypeLeaf");
    // Prototype mesh hard-cull is 100m. Finish opacity fading 5m beforehand.
    inline constexpr float FadeStartCm = 8000.f;
    inline constexpr float FadeEndCm = 9500.f;
    // All owned grass roles inherit the same vendor radius-dependent mask,
    // including their billboard LOD. Match role AND exact source material so
    // unknown/authored slots never receive a generic replacement atlas.
    inline bool NeedsOwnedGrassFade(const FString& MeshName, const FString& MaterialPath)
    {
        if (MeshName == TEXT("SM_APS_Scatter_Grass") || MeshName == TEXT("SM_APS_Scatter_ColdGrass"))
            return MaterialPath == TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_Inst.MI_Grass_Inst")
                || MaterialPath == TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_InstLOD.MI_Grass_InstLOD");
        if (MeshName == TEXT("SM_APS_Scatter_DryGrass"))
            return MaterialPath == TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_Inst_2.MI_Grass_Inst_2")
                || MaterialPath == TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_InstLOD_2.MI_Grass_InstLOD_2");
        return false;
    }
    inline float Visibility(float DistanceCm)
    {
        if (!FMath::IsFinite(DistanceCm)) return 0;
        return 1.f - FMath::SmoothStep(FadeStartCm, FadeEndCm, FMath::Max(0.f, DistanceCm));
    }
}
