#pragma once

#include "APSSharedTerrainMaterial.h"

// Candidate only. No automatic legacy fallback and no change to Water/Lava.
// Both mode callers must explicitly opt in after shoreline/render validation.
namespace APSSharedAmmoniaMaterial
{
    inline const TCHAR* MasterPath()
    {
        return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/M_APS_SharedAmmonia.M_APS_SharedAmmonia");
    }
    inline const TCHAR* TemplatePath()
    {
        return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedAmmonia.MI_APS_SharedAmmonia");
    }
    inline bool IsSharedStack(UMaterialInterface* Material)
    {
        return IsValid(Material) && IsValid(Material->GetMaterial())
            && Material->GetMaterial()->GetPathName() == MasterPath();
    }
    inline bool WriteFrame(UMaterialInstanceDynamic* Material,
        USceneComponent* Frame, double PresentationScale)
    {
        return IsSharedStack(Material)
            && APSSharedTerrainMaterial::WritePhysicalFrame(Material, Frame, PresentationScale);
    }
    inline bool BindFrame(UMaterialInstanceDynamic* Material,
        USceneComponent* Frame, double PresentationScale)
    {
        check(IsInGameThread());
        if (!WriteFrame(Material, Frame, PresentationScale)) return false;
        if (Frame->TransformUpdated.IsBoundToObject(Material)) return true;
        const TWeakObjectPtr<UMaterialInstanceDynamic> WeakMaterial(Material);
        const TWeakObjectPtr<USceneComponent> WeakFrame(Frame);
        Frame->TransformUpdated.AddWeakLambda(Material,
            [WeakMaterial, PresentationScale](USceneComponent* Updated,
                EUpdateTransformFlags, ETeleportType)
            {
                WriteFrame(WeakMaterial.Get(), Updated, PresentationScale);
            });
        FCoreDelegates::PostWorldOriginOffset.AddWeakLambda(Material,
            [WeakMaterial, WeakFrame, PresentationScale](UWorld* World, FIntVector, FIntVector)
            {
                USceneComponent* Current = WeakFrame.Get();
                if (IsValid(Current) && Current->GetWorld() == World)
                    WriteFrame(WeakMaterial.Get(), Current, PresentationScale);
            });
        return true;
    }
    inline UMaterialInstanceDynamic* Create(UObject* Outer, USceneComponent* Frame,
        double PresentationScale, bool bPlanetPresentation, bool bManualPlanet,
        EAPSPlanetLiquidType LiquidType)
    {
        if (bManualPlanet || LiquidType != EAPSPlanetLiquidType::Ammonia) return nullptr;
        UMaterialInstance* Template = LoadObject<UMaterialInstance>(nullptr, TemplatePath());
        if (!IsSharedStack(Template) || Template->GetBlendMode() != BLEND_Masked) return nullptr;
        UMaterialInstanceDynamic* Result = UMaterialInstanceDynamic::Create(Template, Outer);
        if (!IsValid(Result)) return nullptr;
        Result->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), bPlanetPresentation ? 1.0f : 0.0f);
        // All authored chemistry/style/textures are inherited identically.
        return BindFrame(Result, Frame, PresentationScale) ? Result : nullptr;
    }
}
