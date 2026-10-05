#pragma once

#include "APSPlanetSurfaceProfile.h"
#include "APSLivingTerrainPalette.h"
#include "APSTerrestrialMaterialPalette.h"
#include "APSLivingBiomeTransfer.h"
#include "APSOrbitalMacroVariation.h"
#include "APSTerrainContinuityMaterial.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "HAL/IConsoleManager.h"
#include "Components/SceneComponent.h"
#include "Misc/CoreDelegates.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"

/** First material-only integration of the saved SinglePlay WorldScape stack.
 * No noise, mesh, texture assets, static switches or vendor code are modified.
 * The native world-aligned coordinates are retained: rotation/rebase stability
 * is NOT established by selecting this stack and must still be checked in game.
 */
namespace APSNativeTerrainMaterial
{
    inline const TCHAR* MasterPath()
    {
        return TEXT("/Game/Ressources/Materials/WorldScapeMaterials/MastersMaterials/MM_WorldscapeMaterial_V2.MM_WorldscapeMaterial_V2");
    }

    inline const TCHAR* TemplatePath(EAPSPlanetSurfaceArchetype Archetype)
    {
        return Archetype == EAPSPlanetSurfaceArchetype::Magmatic
            ? TEXT("/Game/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Magma.MI_Magma")
            : TEXT("/Game/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Terra.MI_Terra");
    }

    inline bool IsNativeStack(UMaterialInterface* Material)
    {
        const UMaterial* Master = IsValid(Material) ? Material->GetMaterial() : nullptr;
        return IsValid(Master) && Master->GetPathName() == MasterPath();
    }

    inline bool UsesNativePlanetCenter(UMaterialInterface* Material)
    {
        const UMaterial* Master = IsValid(Material) ? Material->GetMaterial() : nullptr;
        // The installed UE5.4 lava master uses the same DoubleVector input as
        // native terrain. Do not change the palette or parent of either stack.
        return IsNativeStack(Material) || (IsValid(Master) && Master->GetPathName() ==
            TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Lava_WorldScape.M_Lava_WorldScape"));
    }

    inline bool ShouldReplace(UMaterialInstance* Current, double PresentationScale,
        bool bManualPlanet, bool bEnabled)
    {
        // Preserve bespoke catalog materials, manual reference planets and the
        // compressed PLANET renderer. Metre-scale ground UVs do not belong on a
        // compressed globe; its separate orbital material is not fixed here.
        if (!bEnabled || bManualPlanet || !FMath::IsFinite(PresentationScale)
            || PresentationScale < 0.999 || !IsValid(Current)) return false;
        const UMaterial* Master = Current->GetMaterial();
        return IsValid(Master) && Master->GetPathName() ==
            TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeTerrain.M_APS_WorldScapeTerrain");
    }

    inline void WritePlanetCenter(UMaterialInstanceDynamic* Material, const FVector& Center)
    {
        if (!IsValid(Material) || Center.ContainsNaN()) return;
        // Exact DoubleVector parameter name in the saved native master (including
        // its spelling). WorldScape's built-in PlanetLocation float parameter is
        // a different input and does not drive this graph's macro/slope branches.
        Material->SetDoubleVectorParameterValue(TEXT("PlanetPosistion"),
            FVector4(Center.X, Center.Y, Center.Z, 0.0));
    }

    /** Called only for a newly created per-root MID, before WorldScape sees it. */
    inline void BindNewInstanceCenter(UMaterialInstanceDynamic* Material,
        USceneComponent* PlanetRoot)
    {
        check(IsInGameThread());
        if (!IsValid(PlanetRoot) || !UsesNativePlanetCenter(Material)) return;
        const FVector Center = PlanetRoot->GetComponentLocation();
        WritePlanetCenter(Material, Center);
        if (PlanetRoot->TransformUpdated.IsBoundToObject(Material)) return;
        const TWeakObjectPtr<UMaterialInstanceDynamic> WeakMaterial(Material);
        // No pawn/camera query and no permanent generator tick. The weak delegate
        // cannot keep an old MID/root alive after profile replacement or teardown.
        PlanetRoot->TransformUpdated.AddWeakLambda(Material,
            [WeakMaterial, CachedCenter = Center](USceneComponent* Updated,
                EUpdateTransformFlags, ETeleportType) mutable
            {
                UMaterialInstanceDynamic* LiveMaterial = WeakMaterial.Get();
                if (!IsValid(LiveMaterial) || !IsValid(Updated)) return;
                const FVector NewCenter = Updated->GetComponentLocation();
                if (NewCenter.ContainsNaN() || NewCenter == CachedCenter) return;
                WritePlanetCenter(LiveMaterial, NewCenter);
                CachedCenter = NewCenter;
            });
        // UE 5.4 SceneComponent::ApplyWorldOffset does NOT broadcast
        // TransformUpdated. Read the final root location after an origin shift,
        // not the offset itself (the hierarchy has already applied it once).
        const TWeakObjectPtr<USceneComponent> WeakRoot(PlanetRoot);
        APSWorldShiftEvents::BindPostShift(Material,
            [WeakMaterial, WeakRoot](UWorld* World)
            {
                USceneComponent* LiveRoot = WeakRoot.Get();
                if (IsValid(LiveRoot) && LiveRoot->GetWorld() == World)
                {
                    WritePlanetCenter(WeakMaterial.Get(), LiveRoot->GetComponentLocation());
                }
            });
    }

    inline void ApplyPalette(UMaterialInstanceDynamic* Material,
        const FAPSResolvedPlanetSurfaceProfile& Profile)
    {
        if (!IsValid(Material)) return;
        const FAPSPlanetSurfacePalette& P = Profile.Palette;
        const bool bRefineTerra = APSTerrestrialMaterialPalette::Enabled()
            && APSTerrestrialMaterialPalette::Allows(Profile.PlanetType, GetPathNameSafe(Material->Parent.Get()));
        auto Set = [Material, bRefineTerra](const TCHAR* Name, const FLinearColor& Color)
        {
            Material->SetVectorParameterValue(Name, bRefineTerra ? APSTerrestrialMaterialPalette::Refine(Name, Color) : Color);
        };
        // Native humid and dry branches use different parameter names. Updating
        // only Color1..5 leaves the dry branch in the reference Earth's palette.
        Set(TEXT("BottomColor"), P.Coast);
        Set(TEXT("Sedimentcolor"), P.Coast);
        Set(TEXT("Color1"), P.Lowland);
        Set(TEXT("Color2"), P.MidLowland);
        Set(TEXT("Color3"), P.Highland);
        Set(TEXT("Color4"), P.Dryland);
        Set(TEXT("Color5"), P.Dryland); // Native Color5 is savanna, not the snow cap.
        // Scope the refinement to generated SharedTerra instances. In particular,
        // never alter a bespoke catalog parent or the authored native reference.
        const bool bGeneratedTerra = IsValid(Material->Parent.Get()) &&
            (Material->Parent->GetPathName() == TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra")
                || Material->Parent->GetPathName() == APSTerrainContinuityMaterial::TemplatePath);
        if (bGeneratedTerra)
        {
            const IConsoleVariable* Detail = IConsoleManager::Get().FindConsoleVariable(
                TEXT("aps.Surface.LivingPaletteDetail"));
            Set(TEXT("Color4"), APSLivingTerrainPalette::CoolDryland(Profile, Detail ? Detail->GetFloat() : 0.0f));
            if (APSLivingTerrainPalette::Allows(Profile))
            {
                // Calibrate the native shoreline to our red-channel sea datum .08.
                // Do not confuse these with similarly named simplified-graph knobs.
                Material->SetScalarParameterValue(TEXT("BottomLayerShift"), APSLivingBiomeTransfer::CoastShift);
                Material->SetScalarParameterValue(TEXT("BottomLayerSharpness"), APSLivingBiomeTransfer::CoastContrast);
                Material->SetScalarParameterValue(TEXT("ShiftSavhana"), APSLivingBiomeTransfer::HumidLandShift);
            }
        }
        Set(TEXT("2_Color1"), P.Dryland);
        Set(TEXT("2_Color2"), P.Coast);
        Set(TEXT("2_Color3"), P.Highland);
        Set(TEXT("2_Color4"), P.Dryland);
        Set(TEXT("Color1_3"), P.Peak);
        Set(TEXT("Color2_3"), P.Highland);
        if (bGeneratedTerra)
        {
            const FLinearColor Rock = APSLivingTerrainPalette::ExposedRock(Profile);
            Set(TEXT("2_Color3"), Rock);
            Set(TEXT("Color2_3"), Rock);
        }
        Set(TEXT("SlopeColor"), P.Slope);
        Set(TEXT("ColotTint"), FLinearColor::White);
        Set(TEXT("EmissiveColor"), P.Emissive);
        if (bRefineTerra)
            UE_LOG(LogTemp, Display, TEXT("[APS.TerrestrialPalette] material=%s vegetationChroma=.52 coastChroma=.65 dryChroma=.62 luminancePreserved=1 exposureUnchanged=1 geometryUnchanged=1 extraTextureSamples=0"), *Material->GetPathName());
		// Generated shared materials already contain a continuous physical-distance
		// normal filter. Its former 200..700 km range left mesh-dependent slope/UV
		// weights fully active across coarse orbital rings, exposing a square LOD0.
		// Finish that filter before orbit, identically in menu and gameplay. Ground
		// shading below 2 km, displaced geometry and collision are unchanged.
		const auto Graph = APSPlanetSurfaceMaterialPolicy::TerrainGraph(Material);
		const bool bContinuous = Graph == APSPlanetSurfaceMaterialPolicy::ETerrainGraph::Continuous;
		if (bContinuous || Graph == APSPlanetSurfaceMaterialPolicy::ETerrainGraph::Shared)
		{
			APSPlanetSurfaceMaterialPolicy::ApplyFarNormalPolicy(Material);
			// One authored colour field from ground to orbit. Mode1 substitutes a
			// different procedural albedo pattern between 5 and 50 km; distance may
			// filter detail, but must not change the surface's identity.
			Material->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), 0.0f);
			if (bContinuous) Material->SetScalarParameterValue(TEXT("APS_NormalMacroWarpMode"), 1.0f);
		}
        // Retain template texture sizes and all other layer/normal transfers.
        // The APS simplified graph's HeightContrast/WarpedScale controls are not
        // interchangeable with identically named controls in this native stack.
    }
}
