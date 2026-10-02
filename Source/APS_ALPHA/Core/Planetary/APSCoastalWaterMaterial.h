#pragma once
#include "APSSharedTerrainMaterial.h"
#include "APSShoreWaterMaterial.h"

// Versioned Water only. Identity checks do not depend on the rollout switch:
// a live instance must remain valid until its owning profile is recreated.
namespace APSCoastalWaterMaterial
{
    inline constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/WaterV1");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/WaterV1/M_APS_CoastalWater.M_APS_CoastalWater");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/WaterV1/MI_APS_CoastalWater.MI_APS_CoastalWater");
    inline bool Allows(const FAPSResolvedPlanetSurfaceProfile& P)
    {
        return P.LiquidType == EAPSPlanetLiquidType::Water
            && (P.PlanetType == EPlanetType::Water || P.PlanetType == EPlanetType::Terrestrial || P.PlanetType == EPlanetType::Oasis)
            && FMath::IsFinite(P.LandCoverage) && P.LandCoverage >= 0 && P.LandCoverage < .995f;
    }
    inline bool Enabled()
    {
        const auto* V = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.CoastalWater"));
        return V && V->GetInt() != 0;
    }
    inline bool EnabledFor(const FAPSResolvedPlanetSurfaceProfile& P)
    {
        return Enabled() && Allows(P);
    }
    inline bool IsInstance(UMaterialInterface* M)
    {
        if (APSShoreWaterMaterial::IsInstance(M)) return true;
        if (!IsValid(M) || !IsValid(M->GetMaterial()) || M->GetMaterial()->GetPathName() != MasterPath
            || M->GetBlendMode() != BLEND_Masked) return false;
        if (M->GetPathName() == TemplatePath) return true;
        const auto* MID = Cast<UMaterialInstanceDynamic>(M);
        return MID && IsValid(MID->Parent.Get()) && MID->Parent->GetPathName() == TemplatePath;
    }
    inline bool WriteFrame(UMaterialInstanceDynamic* M, USceneComponent* Frame, double Scale)
    {
        return IsInstance(M) && APSSharedTerrainMaterial::WritePhysicalFrame(M, Frame, Scale);
    }
    inline bool BindFrame(UMaterialInstanceDynamic* M, USceneComponent* Frame, double Scale)
    {
        check(IsInGameThread());
        if (!WriteFrame(M, Frame, Scale)) return false;
        if (APSShoreWaterMaterial::IsInstance(M) && !APSShoreWaterMaterial::RegisterLighting(M,Frame)) return false;
        if (Frame->TransformUpdated.IsBoundToObject(M)) return true;
        const TWeakObjectPtr<UMaterialInstanceDynamic> WeakM(M);
        const TWeakObjectPtr<USceneComponent> WeakFrame(Frame);
        Frame->TransformUpdated.AddWeakLambda(M, [WeakM, Scale](USceneComponent* F, EUpdateTransformFlags, ETeleportType)
        { WriteFrame(WeakM.Get(), F, Scale); });
        FCoreDelegates::PostWorldOriginOffset.AddWeakLambda(M, [WeakM, WeakFrame, Scale](UWorld* World, FIntVector, FIntVector)
        {
            auto* F = WeakFrame.Get();
            if (IsValid(F) && F->GetWorld() == World) WriteFrame(WeakM.Get(), F, Scale);
        });
        return true;
    }
}
