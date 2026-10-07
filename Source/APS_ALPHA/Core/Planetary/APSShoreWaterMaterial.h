#pragma once
#include "APSSharedTerrainMaterial.h"
#include "APSWaterSurfaceLighting.h"
#include "APSWaterLightingSubsystem.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

// Factory trial, default OFF. One saved template in gameplay and PLANET;
// radial UV1 depth, physical frame and scene-derived fill remain independent.
namespace APSShoreWaterMaterial
{
inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterShoreTransmission20261001/M_APS_WaterShoreTransmission.M_APS_WaterShoreTransmission");
inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterShoreTransmission20261001/MI_APS_WaterShoreTransmission.MI_APS_WaterShoreTransmission");
inline bool Enabled()
{
    const auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.ShoreWater"));
    return V && V->GetInt()==1;
}
inline bool IsInstance(UMaterialInterface* M)
{
    if (!IsValid(M) || !IsValid(M->GetMaterial()) || M->GetMaterial()->GetPathName()!=MasterPath
        || M->GetBlendMode()!=BLEND_Masked || !M->GetShadingModels().HasOnlyShadingModel(MSM_SingleLayerWater)) return false;
    if (M->GetPathName()==TemplatePath) return true;
    const auto* MID=Cast<UMaterialInstanceDynamic>(M);
    return MID && IsValid(MID->Parent.Get()) && MID->Parent->GetPathName()==TemplatePath;
}
inline bool IsVisiblePreviewOcean(const AActor* PreviewOwner, const UMeshComponent* Ocean)
{
    // Query the committed drawable, not the body's separate gameplay resolver
    // or a retained/staging material that may still be alive but hidden.
    return IsValid(PreviewOwner) && !PreviewOwner->IsActorBeingDestroyed() && !PreviewOwner->IsHidden()
        && PreviewOwner->ActorHasTag(TEXT("WorldGenerationPreview"))
        && IsValid(Ocean) && Ocean->GetOwner()==PreviewOwner && Ocean->IsRegistered()
        && Ocean->IsVisible() && !Ocean->bHiddenInGame && IsInstance(Ocean->GetMaterial(0));
}
inline bool RegisterLighting(UMaterialInstanceDynamic* M, USceneComponent* Frame)
{
    if (!IsInstance(M) || !IsValid(Frame)) return false;
    auto* World=Frame->GetWorld();
    // Transient offline contract fixtures need no scene registration.
    if (!World || !World->IsGameWorld()) return true;
    if (!APSWaterSurfaceLighting::RenderContract()) return false;
    auto* Binding=World->GetSubsystem<UAPSWaterLightingSubsystem>();
    return Binding && Binding->RegisterMaterial(M);
}
}
