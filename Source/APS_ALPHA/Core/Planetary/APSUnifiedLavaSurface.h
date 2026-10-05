#pragma once

#include "APSSharedTerrainMaterial.h"
#include "APSUnifiedLavaAssets.h"
#include "MaterialShared.h"
#include "LocalVertexFactory.h"

namespace APSUnifiedLavaSurface
{
    enum class ECreateFailure : uint8
    {
        None, InvalidFrame, InvalidLiquid, InvalidRadius, MissingTemplate,
        UnexpectedMaster, NonOpaque, MissingResource, ShaderCompileErrors,
        MissingShaderMap, IncompleteShaderMap, MissingLocalVertexFactory,
        InstanceCreationFailed, InvalidPhysicalFrame
    };

    inline const TCHAR* FailureName(ECreateFailure Failure)
    {
        switch (Failure)
        {
#define APS_LAVA_FAILURE(Name) case ECreateFailure::Name: return TEXT(#Name);
            APS_LAVA_FAILURE(None)
            APS_LAVA_FAILURE(InvalidFrame)
            APS_LAVA_FAILURE(InvalidLiquid)
            APS_LAVA_FAILURE(InvalidRadius)
            APS_LAVA_FAILURE(MissingTemplate)
            APS_LAVA_FAILURE(UnexpectedMaster)
            APS_LAVA_FAILURE(NonOpaque)
            APS_LAVA_FAILURE(MissingResource)
            APS_LAVA_FAILURE(ShaderCompileErrors)
            APS_LAVA_FAILURE(MissingShaderMap)
            APS_LAVA_FAILURE(IncompleteShaderMap)
            APS_LAVA_FAILURE(MissingLocalVertexFactory)
            APS_LAVA_FAILURE(InstanceCreationFailed)
            APS_LAVA_FAILURE(InvalidPhysicalFrame)
#undef APS_LAVA_FAILURE
        }
        return TEXT("Unknown");
    }

    inline UMaterialInstanceDynamic* Create(UObject* Outer, USceneComponent* Frame,
        const FAPSResolvedPlanetSurfaceProfile& Profile, double RadiusCm, ERHIFeatureLevel::Type FeatureLevel,
        ECreateFailure* OutFailure = nullptr)
    {
        check(IsInGameThread());
        if (OutFailure) *OutFailure = ECreateFailure::None;
        UMaterialInstance* Template = nullptr;
        const UMaterial* Master = nullptr;
        FMaterialResource* Resource = nullptr;
        FMaterialShaderMap* Map = nullptr;
        const auto Reject = [&](ECreateFailure Failure) -> UMaterialInstanceDynamic*
        {
            if (OutFailure) *OutFailure = Failure;
            int32 CompileErrors = 0;
#if WITH_EDITOR
            CompileErrors = Resource ? Resource->GetCompileErrors().Num() : 0;
#endif
            // A failed candidate keeps the existing terrain/ocean pair. Report
            // the actual MIC permutation, never substitute the master resource:
            // static switches can make those two shader maps different.
            UE_LOG(LogTemp, Warning,
                TEXT("[APS.UnifiedLava] rejected=%s outer=%s template=%s master=%s feature=%d quality=%d staticPermutation=%d resource=%d complete=%d errors=%d map=%d localVF=%d radiusCm=%.3f frame=%s scale=%s"),
                FailureName(Failure), *GetNameSafe(Outer), *GetPathNameSafe(Template), *GetPathNameSafe(Master),
                int32(FeatureLevel), Resource ? int32(Resource->GetQualityLevel()) : -1,
                IsValid(Template) && Template->bHasStaticPermutationResource ? 1 : 0,
                Resource ? 1 : 0, Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
                CompileErrors, Map ? 1 : 0, Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType) ? 1 : 0,
                RadiusCm, *GetNameSafe(Frame), IsValid(Frame) ? *Frame->GetComponentScale().ToString() : TEXT("none"));
            return nullptr;
        };
        if (!IsValid(Frame)) return Reject(ECreateFailure::InvalidFrame);
        if (Profile.LiquidType != EAPSPlanetLiquidType::Lava) return Reject(ECreateFailure::InvalidLiquid);
        if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0) return Reject(ECreateFailure::InvalidRadius);
        const TCHAR* Path = APSUnifiedLavaAssets::TemplatePath();
        Template = LoadObject<UMaterialInstance>(nullptr, Path);
        if (!IsValid(Template)) return Reject(ECreateFailure::MissingTemplate);
        Master = Template->GetMaterial();
        if (!IsValid(Master) || Master->GetPathName() !=
                APSUnifiedLavaAssets::MasterPath())
            return Reject(ECreateFailure::UnexpectedMaster);
        if (Template->GetBlendMode() != BLEND_Opaque) return Reject(ECreateFailure::NonOpaque);
        Resource = Template->GetMaterialResource(FeatureLevel);
        if (!Resource) return Reject(ECreateFailure::MissingResource);
        Map = Resource->GetGameThreadShaderMap();
#if WITH_EDITOR
        if (Resource->GetCompileErrors().Num()) return Reject(ECreateFailure::ShaderCompileErrors);
#endif
        if (!Map) return Reject(ECreateFailure::MissingShaderMap);
        if (!Resource->IsGameThreadShaderMapComplete()) return Reject(ECreateFailure::IncompleteShaderMap);
        if (!Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Reject(ECreateFailure::MissingLocalVertexFactory);
        auto* Result = UMaterialInstanceDynamic::Create(Template, Outer);
        if (!Result) return Reject(ECreateFailure::InstanceCreationFailed);
        UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(Result, Profile);
        if (APSUnifiedLavaAssets::DetailCandidate())
            Result->SetScalarParameterValue(TEXT("APS_UnifiedDetailStrength"), APSUnifiedLavaAssets::DetailStrength());
        // Same accepted rock-side filter as shared terrain, without introducing
        // its orbital/warp overrides or changing lava, palette or coast weights.
        APSPlanetSurfaceMaterialPolicy::ApplyFarNormalPolicy(Result);
        Result->SetScalarParameterValue(TEXT("APS_UnifiedSeaRadiusCm"),
            static_cast<float>(RadiusCm + static_cast<double>(Profile.OceanLevel) * Profile.NoiseIntensity));
        // Covers radial float quantization; metre-scale coast feather, not a
        // distance/LOD-dependent mask. The base geography and RGB stay untouched.
        Result->SetScalarParameterValue(TEXT("APS_UnifiedRadiusToleranceCm"),
            static_cast<float>(FMath::Max(200.0, RadiusCm * 1.0e-6)));
        if (!APSSharedTerrainMaterial::BindNewInstanceFrame(Result, Frame, 1.0))
            return Reject(ECreateFailure::InvalidPhysicalFrame);
        return Result;
    }
}
