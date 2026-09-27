#pragma once

#include "APSSharedWaterMaterial.h"
#include "APSSharedLavaMaterial.h"
#include "Engine/World.h"

// Generated APS source MICs only. All liquids inherit saved shared family
// parameters, never the incompatible legacy Water graph or another chemistry.
namespace APSSharedGeneratedLiquidMaterial
{
    inline const TCHAR* SourcePath(EAPSPlanetLiquidType Type)
    {
        switch (Type)
        {
        case EAPSPlanetLiquidType::Water: return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Water.MI_APS_WS_Water");
        case EAPSPlanetLiquidType::Ammonia: return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Ammonia.MI_APS_WS_Ammonia");
        case EAPSPlanetLiquidType::Lava: return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Lava.MI_APS_WS_Lava");
        default: return nullptr;
        }
    }
    inline const TCHAR* TemplatePath(EAPSPlanetLiquidType Type)
    {
        switch (Type)
        {
        case EAPSPlanetLiquidType::Water: return APSSharedWaterMaterial::TemplatePath();
        case EAPSPlanetLiquidType::Ammonia: return APSSharedAmmoniaMaterial::TemplatePath();
        case EAPSPlanetLiquidType::Lava: return APSSharedLavaMaterial::TemplatePath();
        default: return nullptr;
        }
    }
    inline bool AllowsProfile(const FAPSResolvedPlanetSurfaceProfile& Profile, bool bManualPlanet)
    {
        return !bManualPlanet
            && TemplatePath(Profile.LiquidType) != nullptr
            && FMath::IsFinite(Profile.LandCoverage)
            && Profile.LandCoverage >= 0.0f && Profile.LandCoverage < 0.995f;
    }
    inline bool ShouldMigrate(UMaterialInterface* Source,
        const FAPSResolvedPlanetSurfaceProfile& Profile, bool bManualPlanet)
    {
        return AllowsProfile(Profile, bManualPlanet) && IsValid(Source)
            && Source->GetPathName() == SourcePath(Profile.LiquidType);
    }
    inline UMaterialInstance* ResolveSource(const FAPSResolvedPlanetSurfaceProfile& Profile,
        const UAPSPlanetSurfaceCatalog* Catalog)
    {
        const TCHAR* Path = SourcePath(Profile.LiquidType);
        if (!Path) return nullptr;
        if (IsValid(Catalog))
            if (const auto* Definition = Catalog->Archetypes.Find(Profile.Archetype))
                if (Definition->LiquidType == Profile.LiquidType)
                    if (UMaterialInstance* Authored = Definition->OceanMaterial.LoadSynchronous()) return Authored;
        return LoadObject<UMaterialInstance>(nullptr, Path);
    }
    inline bool IsFamilyInstance(UMaterialInterface* Material, EAPSPlanetLiquidType Type)
    {
        const TCHAR* Path = TemplatePath(Type);
        if (!Path || !IsValid(Material)) return false;
        const bool bMatchingMaster = Type == EAPSPlanetLiquidType::Lava
            ? APSSharedLavaMaterial::IsSharedStack(Material) : APSSharedAmmoniaMaterial::IsSharedStack(Material);
        if (!bMatchingMaster || Material->GetBlendMode() != BLEND_Masked) return false;
        if (Material->GetPathName() == Path) return true;
        const auto* MID = Cast<UMaterialInstanceDynamic>(Material);
        return MID && IsValid(MID->Parent.Get()) && MID->Parent->GetPathName() == Path;
    }
    inline bool HasSavedParameterAuthority(UMaterialInterface* Material, EAPSPlanetLiquidType Type)
    {
        if (!IsFamilyInstance(Material, Type)) return false;
        if (Type == EAPSPlanetLiquidType::Lava)
        {
            float Brightness = -1.0f;
            FLinearColor Emission;
            // Saved MIC authority, including deliberate future cooling to zero.
            // Never inject a different brightness/palette in either draw path.
            return Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), Brightness)
                && FMath::IsFinite(Brightness) && Brightness >= 0.0f
                && Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveColor")), Emission)
                && FMath::IsFinite(Emission.R) && FMath::IsFinite(Emission.G) && FMath::IsFinite(Emission.B)
                && Emission.R >= 0.0f && Emission.G >= 0.0f && Emission.B >= 0.0f;
        }
        if (Type != EAPSPlanetLiquidType::Water) return true;
        // The saved MIC owns Water styling. Validate its scalar, not one chosen
        // palette value: future valid saved edits must not require a code change.
        float Opacity = -1.0f;
        return Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("WaterSurfaceOpacity")), Opacity)
            && FMath::IsFinite(Opacity) && Opacity >= 0.0f && Opacity <= 1.0f;
    }
    inline bool IsRenderReady(UMaterialInterface* Material, EAPSPlanetLiquidType Type, const UWorld* World)
    {
        if (!HasSavedParameterAuthority(Material, Type) || !IsValid(World)) return false;
        const auto* Resource = Material->GetMaterialResource(World->GetFeatureLevel());
        const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        return Resource && Resource->IsGameThreadShaderMapComplete() && Resource->GetCompileErrors().Num() == 0
            && Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
    }
    inline bool BindFrame(UMaterialInstanceDynamic* Material, EAPSPlanetLiquidType Type,
        USceneComponent* Frame, double PresentationScale)
    {
        if (!IsFamilyInstance(Material, Type)) return false;
        return Type == EAPSPlanetLiquidType::Lava
            ? APSSharedLavaMaterial::BindFrame(Material, Frame, PresentationScale)
            : APSSharedAmmoniaMaterial::BindFrame(Material, Frame, PresentationScale);
    }
    inline UMaterialInstanceDynamic* Create(UObject* Outer, USceneComponent* Frame,
        double PresentationScale, bool bPlanetPresentation, bool bManualPlanet,
        const FAPSResolvedPlanetSurfaceProfile& Profile, UMaterialInstance* Source)
    {
        const EAPSPlanetLiquidType Type = Profile.LiquidType;
        if (!AllowsProfile(Profile, bManualPlanet)
            || (!ShouldMigrate(Source, Profile, bManualPlanet) && !IsFamilyInstance(Source, Type))) return nullptr;
        UMaterialInstance* Template = LoadObject<UMaterialInstance>(nullptr, TemplatePath(Type));
        if (!HasSavedParameterAuthority(Template, Type)) return nullptr;
        UMaterialInstanceDynamic* Result = UMaterialInstanceDynamic::Create(Template, Outer);
        if (!IsValid(Result)) return nullptr;
        // No Copy*Parameters here: Water, Ammonia and Lava share their own saved
        // MIC between modes. Only mask semantics and physical frame differ.
        Result->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), bPlanetPresentation ? 1.0f : 0.0f);
        return BindFrame(Result, Type, Frame, PresentationScale) ? Result : nullptr;
    }
}
