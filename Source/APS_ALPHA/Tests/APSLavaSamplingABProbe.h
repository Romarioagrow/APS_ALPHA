#pragma once
#include "APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Test-only binding; no changes to the production family selector.
namespace APSLavaSamplingABProbe
{
    inline bool UsesExplicitDerivatives() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeLavaDerivativeFix")); }
    inline bool UsesScaleIsolation() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeLavaScaleAB")); }
    inline bool UsesBandwidthLOD() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeLavaBandwidthLOD")); }
    inline const TCHAR* TemplatePath()
    {
        return UsesBandwidthLOD()
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaBandwidthLOD/MI_APS_LavaBandwidthLOD.MI_APS_LavaBandwidthLOD") : UsesScaleIsolation()
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaScaleAB/MI_APS_LavaSamplingAB.MI_APS_LavaSamplingAB") : UsesExplicitDerivatives()
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaDerivativeFix/MI_APS_LavaSamplingAB.MI_APS_LavaSamplingAB")
            : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaSamplingAB/MI_APS_LavaSamplingAB.MI_APS_LavaSamplingAB");
    }
    inline bool IsStack(UMaterialInterface* M)
    {
        const TCHAR* Master = UsesBandwidthLOD()
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaBandwidthLOD/M_APS_LavaBandwidthLOD.M_APS_LavaBandwidthLOD") : UsesScaleIsolation()
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaScaleAB/M_APS_LavaSamplingAB.M_APS_LavaSamplingAB") : UsesExplicitDerivatives()
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaDerivativeFix/M_APS_LavaSamplingAB.M_APS_LavaSamplingAB")
            : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaSamplingAB/M_APS_LavaSamplingAB.M_APS_LavaSamplingAB");
        return IsValid(M) && IsValid(M->GetMaterial()) && M->GetMaterial()->GetPathName() == Master;
    }
    inline UMaterialInstanceDynamic* Create(USceneComponent* Frame, double PresentationScale)
    {
        if (int32(UsesScaleIsolation()) + int32(UsesExplicitDerivatives()) + int32(UsesBandwidthLOD()) > 1) return nullptr;
        auto* Template = LoadObject<UMaterialInterface>(nullptr, TemplatePath());
        if (!IsStack(Template) || Template->GetBlendMode() != BLEND_Masked) return nullptr;
        auto* Material = UMaterialInstanceDynamic::Create(Template, Frame);
        if (!Material || !APSSharedTerrainMaterial::WritePhysicalFrame(Material, Frame, PresentationScale)) return nullptr;
        Material->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), 1.0f);
        // The clean LOD candidate preserves the saved authored brightness.
        if (!UsesBandwidthLOD()) Material->SetScalarParameterValue(TEXT("Brightness"), 0.0f);
        const TWeakObjectPtr<UMaterialInstanceDynamic> WeakMaterial(Material);
        const TWeakObjectPtr<USceneComponent> WeakFrame(Frame);
        Frame->TransformUpdated.AddWeakLambda(Material, [WeakMaterial, PresentationScale](USceneComponent* Updated, EUpdateTransformFlags, ETeleportType)
        { APSSharedTerrainMaterial::WritePhysicalFrame(WeakMaterial.Get(), Updated, PresentationScale); });
        FCoreDelegates::PostWorldOriginOffset.AddWeakLambda(Material, [WeakMaterial, WeakFrame, PresentationScale](UWorld* World, FIntVector, FIntVector)
        {
            USceneComponent* Current = WeakFrame.Get();
            if (IsValid(Current) && Current->GetWorld() == World)
                APSSharedTerrainMaterial::WritePhysicalFrame(WeakMaterial.Get(), Current, PresentationScale);
        });
        return Material;
    }
}
