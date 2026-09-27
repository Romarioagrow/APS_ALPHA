#pragma once

#include "APSSharedTerrainMaterial.h"
#include "MaterialShared.h"
#include "LocalVertexFactory.h"

namespace APSUnifiedLavaSurface
{
    inline UMaterialInstanceDynamic* Create(UObject* Outer, USceneComponent* Frame,
        const FAPSResolvedPlanetSurfaceProfile& Profile, double RadiusCm, ERHIFeatureLevel::Type FeatureLevel)
    {
        if (!Frame || Profile.LiquidType != EAPSPlanetLiquidType::Lava
            || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0) return nullptr;
        constexpr const TCHAR* Path = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface");
        UMaterialInstance* Template = LoadObject<UMaterialInstance>(nullptr, Path);
        const UMaterial* Master = IsValid(Template) ? Template->GetMaterial() : nullptr;
        if (!IsValid(Master) || Master->GetPathName() !=
                TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface")
            || Template->GetBlendMode() != BLEND_Opaque) return nullptr;
        FMaterialResource* Resource = Template->GetMaterialResource(FeatureLevel);
        FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        if (!Resource || !Resource->IsGameThreadShaderMapComplete()
            || Resource->GetCompileErrors().Num() || !Map
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return nullptr;
        auto* Result = UMaterialInstanceDynamic::Create(Template, Outer);
        if (!Result) return nullptr;
        UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(Result, Profile);
        Result->SetScalarParameterValue(TEXT("APS_UnifiedSeaRadiusCm"),
            static_cast<float>(RadiusCm + static_cast<double>(Profile.OceanLevel) * Profile.NoiseIntensity));
        // Covers radial float quantization; metre-scale coast feather, not a
        // distance/LOD-dependent mask. The base geography and RGB stay untouched.
        Result->SetScalarParameterValue(TEXT("APS_UnifiedRadiusToleranceCm"),
            static_cast<float>(FMath::Max(200.0, RadiusCm * 1.0e-6)));
        return APSSharedTerrainMaterial::BindNewInstanceFrame(Result, Frame, 1.0) ? Result : nullptr;
    }
}
