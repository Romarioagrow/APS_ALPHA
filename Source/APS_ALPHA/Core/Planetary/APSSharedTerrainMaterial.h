#pragma once

#include "APSNativeTerrainMaterial.h"
#include "HAL/IConsoleManager.h"

/** One native WorldScape shading stack for closed PLANET meshes and live terrain.
 * The graph consumes physical centimetres in a planet-fixed frame. Geometry may
 * be compressed for the menu, but texture size and distance LOD remain physical.
 * Only generated surfaces opt in; authored reference assets are never reparented.
 */
namespace APSSharedTerrainMaterial
{
    inline bool AllowsLegacyDiagnosticFallback()
    {
        const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(
            TEXT("aps.Surface.SharedTerrainLegacyDiagnosticFallback"));
        return Variable && Variable->GetInt() != 0;
    }

    inline bool IsGeneratedCatalogStack(UMaterialInterface* Material)
    {
        const UMaterial* Master = IsValid(Material) ? Material->GetMaterial() : nullptr;
        return IsValid(Master) && Master->GetPathName() ==
            TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeTerrain.M_APS_WorldScapeTerrain");
    }

    inline const TCHAR* MasterPath()
    {
        return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/M_APS_SharedWorldScapeTerrain.M_APS_SharedWorldScapeTerrain");
    }

    inline const TCHAR* TemplatePath(EAPSPlanetSurfaceArchetype Archetype)
    {
        return Archetype == EAPSPlanetSurfaceArchetype::Magmatic
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedMagma.MI_APS_SharedMagma")
            : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra");
    }

    inline bool IsSharedStack(UMaterialInterface* Material)
    {
        const UMaterial* Master = IsValid(Material) ? Material->GetMaterial() : nullptr;
        return IsValid(Master) && Master->GetPathName() == MasterPath();
    }

    // Called only through an exact stack-specific guard.
    inline bool WritePhysicalFrame(UMaterialInstanceDynamic* Material,
        const USceneComponent* PlanetRoot, double PresentationScale)
    {
        if (!IsValid(Material) || !IsValid(PlanetRoot)
            || !FMath::IsFinite(PresentationScale) || PresentationScale <= 0.0) return false;
        const FVector Center = PlanetRoot->GetComponentLocation();
        const FQuat Rotation = PlanetRoot->GetComponentQuat().GetNormalized();
        if (Center.ContainsNaN() || Rotation.ContainsNaN()) return false;
        const FVector ComponentScale = PlanetRoot->GetComponentScale();
        const double UniformScale = ComponentScale.GetAbsMax();
        if (ComponentScale.ContainsNaN() || ComponentScale.X <= 0.0
            || ComponentScale.Y <= 0.0 || ComponentScale.Z <= 0.0
            || UniformScale <= 0.0
            || UniformScale - ComponentScale.GetAbsMin() > UniformScale * 1.0e-5) return false;
        // The preview stores already compressed vertex positions, then applies
        // additional hierarchy focus/zoom scale to the component. Both matter.
        const double EffectiveScale = PresentationScale * UniformScale;
        if (!FMath::IsFinite(EffectiveScale) || EffectiveScale <= 0.0) return false;
        const double InverseScale = 1.0 / EffectiveScale;
        if (!FMath::IsFinite(InverseScale)
            || InverseScale > double(TNumericLimits<float>::Max())) return false;
        Material->SetDoubleVectorParameterValue(TEXT("APS_SharedPlanetCenter"),
            FVector4(Center.X, Center.Y, Center.Z, 0.0));
        // Float scale/axes cause centimetre-to-metre phase drift at planetary
        // radius even if the position math itself stays LWC.
        Material->SetDoubleVectorParameterValue(TEXT("APS_SharedInverseScale"),
            FVector4(1.0 / EffectiveScale, 0.0, 0.0, 0.0));
        const auto SetAxis = [Material](const TCHAR* Name, const FVector& Axis)
        {
            Material->SetDoubleVectorParameterValue(Name,
                FVector4(Axis.X, Axis.Y, Axis.Z, 0.0));
        };
        SetAxis(TEXT("APS_SharedAxisX"), Rotation.GetAxisX());
        SetAxis(TEXT("APS_SharedAxisY"), Rotation.GetAxisY());
        SetAxis(TEXT("APS_SharedAxisZ"), Rotation.GetAxisZ());

        // The near WAT path evaluates world-to-physical rows with DoubleFloat.
        // A tile/offset DoubleVector parameter cannot retain the residual of a
        // unit rotation coefficient (its tile is zero), so split each composed
        // rotation/scale row explicitly. The macro frame above stays unchanged.
        const auto SetDetailRow = [Material, InverseScale](const TCHAR* HighName,
            const TCHAR* LowName, const FVector& Axis)
        {
            const FVector Row = Axis * InverseScale;
            const FLinearColor High(float(Row.X), float(Row.Y), float(Row.Z), 0.0f);
            const FLinearColor Low(float(Row.X - double(High.R)),
                float(Row.Y - double(High.G)), float(Row.Z - double(High.B)), 0.0f);
            Material->SetVectorParameterValue(HighName, High);
            Material->SetVectorParameterValue(LowName, Low);
        };
        SetDetailRow(TEXT("APS_SharedDetailRowXHigh"), TEXT("APS_SharedDetailRowXLow"), Rotation.GetAxisX());
        SetDetailRow(TEXT("APS_SharedDetailRowYHigh"), TEXT("APS_SharedDetailRowYLow"), Rotation.GetAxisY());
        SetDetailRow(TEXT("APS_SharedDetailRowZHigh"), TEXT("APS_SharedDetailRowZLow"), Rotation.GetAxisZ());
        return true;
    }

    inline bool WriteFrame(UMaterialInstanceDynamic* Material,
        const USceneComponent* PlanetRoot, double PresentationScale)
    {
        return IsSharedStack(Material) && WritePhysicalFrame(Material, PlanetRoot, PresentationScale);
    }

    inline bool BindNewInstanceFrame(UMaterialInstanceDynamic* Material,
        USceneComponent* PlanetRoot, double PresentationScale)
    {
        check(IsInGameThread());
        if (!WriteFrame(Material, PlanetRoot, PresentationScale)) return false;
        if (PlanetRoot->TransformUpdated.IsBoundToObject(Material)) return true;
        const TWeakObjectPtr<UMaterialInstanceDynamic> WeakMaterial(Material);
        const TWeakObjectPtr<USceneComponent> WeakRoot(PlanetRoot);
        // PresentationScale is the immutable local-vertex profile scale, not a
        // patch LOD scale. Component scale is reread on every transform update.
        // Rebuild/rebind the per-body MID when the profile itself changes.
        PlanetRoot->TransformUpdated.AddWeakLambda(Material,
            [WeakMaterial, PresentationScale](USceneComponent* Updated,
                EUpdateTransformFlags, ETeleportType)
            {
                WriteFrame(WeakMaterial.Get(), Updated, PresentationScale);
            });
        FCoreDelegates::PostWorldOriginOffset.AddWeakLambda(Material,
            [WeakMaterial, WeakRoot, PresentationScale](UWorld* World, FIntVector, FIntVector)
            {
                USceneComponent* Root = WeakRoot.Get();
                if (IsValid(Root) && Root->GetWorld() == World)
                    WriteFrame(WeakMaterial.Get(), Root, PresentationScale);
            });
        return true;
    }

    inline UMaterialInstanceDynamic* Create(UObject* Outer,
        const FAPSResolvedPlanetSurfaceProfile& Profile,
        USceneComponent* PlanetRoot, double PresentationScale)
    {
        UMaterialInstance* Template = LoadObject<UMaterialInstance>(nullptr,
            TemplatePath(Profile.Archetype));
        if (!IsSharedStack(Template)) return nullptr;
        UMaterialInstanceDynamic* Result = UMaterialInstanceDynamic::Create(Template, Outer);
        if (!IsValid(Result)) return nullptr;
        APSNativeTerrainMaterial::ApplyPalette(Result, Profile);
        // No copy of simplified/orbital parameters: equal names do not guarantee
        // equal units or transfer curves in the native graph.
        return BindNewInstanceFrame(Result, PlanetRoot, PresentationScale) ? Result : nullptr;
    }
}
