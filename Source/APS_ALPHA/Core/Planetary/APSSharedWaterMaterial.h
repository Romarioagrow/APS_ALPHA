#pragma once

#include "APSSharedAmmoniaMaterial.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"

// Optional test-only factory; no caller or production selector is installed by
// this candidate. It deliberately reuses the existing physical-liquid frame
// adapter but never the ammonia family's template or its chemistry overrides.
namespace APSSharedWaterMaterial
{
    inline const TCHAR* TemplatePath()
    {
        return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedWater.MI_APS_SharedWater");
    }
    inline bool AllowsProfile(const FAPSResolvedPlanetSurfaceProfile& Profile,
        bool bManualPlanet, bool bResolvedOceanEnabled)
    {
        return !bManualPlanet && bResolvedOceanEnabled
            && Profile.LiquidType == EAPSPlanetLiquidType::Water
            && FMath::IsFinite(Profile.LandCoverage)
            && Profile.LandCoverage >= 0.0f && Profile.LandCoverage < 0.995f;
    }
    inline UMaterialInstance* LoadTemplate()
    {
        UMaterialInstance* Template = LoadObject<UMaterialInstance>(nullptr, TemplatePath());
        return IsValid(Template) && IsValid(Template->Parent.Get())
            && Template->Parent->GetPathName() == APSSharedAmmoniaMaterial::MasterPath()
            && APSSharedAmmoniaMaterial::IsSharedStack(Template)
            && Template->GetBlendMode() == BLEND_Masked ? Template : nullptr;
    }
    inline bool IsReadyToPublish(UMaterialInstanceDynamic* Candidate,
        ERHIFeatureLevel::Type FeatureLevel)
    {
        if (!IsValid(Candidate) || !IsValid(Candidate->Parent.Get())
            || Candidate->Parent->GetPathName() != TemplatePath()
            || !APSSharedAmmoniaMaterial::IsSharedStack(Candidate)) return false;
        FMaterialResource* Resource = Candidate->GetMaterialResource(FeatureLevel);
        FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        return Resource && Resource->IsGameThreadShaderMapComplete()
#if WITH_EDITOR
            && Resource->GetCompileErrors().Num() == 0
#endif
            && Map
            && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
    }
    // Creation is not publication. The probe must request asynchronous shader/
    // LocalVF PSO warmup once, wait with a bounded timeout and publish only after
    // IsReadyToPublish. Loading alone can leave an uncooked partial shader map.
    inline UMaterialInstanceDynamic* CreateUnboundCandidate(UObject* Outer, USceneComponent* Frame,
        double PresentationScale, bool bPlanetPresentation,
        const FAPSResolvedPlanetSurfaceProfile& Profile,
        bool bManualPlanet, bool bResolvedOceanEnabled)
    {
        if (!AllowsProfile(Profile, bManualPlanet, bResolvedOceanEnabled)) return nullptr;
        UMaterialInstance* Template = LoadTemplate();
        if (!Template) return nullptr;
        UMaterialInstanceDynamic* Result = UMaterialInstanceDynamic::Create(Template, Outer);
        if (!IsValid(Result)) return nullptr;
        Result->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), bPlanetPresentation ? 1.0f : 0.0f);
        // Do not copy the legacy unlit-Water or ammonia runtime parameters here.
        // The saved Water MIC is the same numeric authority in both contexts.
        return APSSharedAmmoniaMaterial::BindFrame(Result, Frame, PresentationScale) ? Result : nullptr;
    }
}
