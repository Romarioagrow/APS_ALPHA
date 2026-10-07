#pragma once
#include "CoreMinimal.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"

// Scene-read-only binding for the water-only pass candidate. Not automatically
// selected by gameplay. Reject unsupported light/BRDF configurations explicitly.
namespace APSWaterSurfaceLighting
{
inline bool RenderContract()
{
    for (const TCHAR* Name : {TEXT("r.Material.RoughDiffuse"), TEXT("r.Material.EnergyConservation"), TEXT("r.Substrate")})
    {
        const auto* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
        if (!CVar || CVar->GetInt() != 0)
        { UE_LOG(LogTemp,Warning,TEXT("[APS.WaterPass.Contract] %s actual=%d expected=0"),Name,CVar?CVar->GetInt():-999); return false; }
    }
    for (const TCHAR* Name : {TEXT("r.Water.SingleLayer.ShadersSupportVSMFiltering"),
        TEXT("r.Water.SingleLayer.VSMFiltering"), TEXT("r.Water.SingleLayer.ShadersSupportDistanceFieldShadow"),
        TEXT("r.Shadow.Virtual.Enable"), TEXT("r.Water.SingleLayer.DepthPrepass"),
        TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO")})
    {
        const auto* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
        if (!CVar || CVar->GetInt() != 1)
        { UE_LOG(LogTemp,Warning,TEXT("[APS.WaterPass.Contract] %s actual=%d expected=1"),Name,CVar?CVar->GetInt():-999); return false; }
    }
    return true;
}
inline bool Resolve(UWorld* World, FLinearColor& Direction, FLinearColor& Irradiance)
{
    Direction = FLinearColor::Black; Irradiance = FLinearColor::Black;
    if (!World) return false;
    UDirectionalLightComponent* Fill = nullptr;
    UDirectionalLightComponent* Main = nullptr;
    int32 VisibleLights = 0;
    for (TActorIterator<ADirectionalLight> It(World); It; ++It)
    {
        auto* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
        if (!Light || !Light->bAffectsWorld || !Light->IsVisible() || Light->bHiddenInGame
            || It->IsHidden() || !Light->LightingChannels.bChannel0) continue;
        if (!FMath::IsFinite(Light->Intensity) || Light->Intensity < 0) return false;
        if (Light->Intensity == 0) continue;
        ++VisibleLights;
        if (!Main || Light->ForwardShadingPriority > Main->ForwardShadingPriority) Main = Light;
        if (It->ActorHasTag(TEXT("APSGameplaySurfaceFillLight")) || It->ActorHasTag(TEXT("APSPreviewFillLight")))
        {
            if (Fill || !It->HasAnyFlags(RF_Transient) || Light->CastShadows
                || Light->SpecularScale != 0 || Light->LightingChannels.bChannel1
                || Light->LightingChannels.bChannel2) return false;
            Fill = Light;
        }
    }
    // No optional fill above its range: zero contribution, not a material swap.
    if (VisibleLights == 1 && !Fill && Main && Main->ForwardShadingPriority > 0) return true;
    if (VisibleLights != 2 || !Fill || !Main || Main == Fill
        || Main->ForwardShadingPriority <= Fill->ForwardShadingPriority)
    { UE_LOG(LogTemp,Warning,TEXT("[APS.WaterPass.Contract] lights=%d main=%s fill=%s"),VisibleLights,*GetNameSafe(Main),*GetNameSafe(Fill)); return false; }
    const FVector ToLight = -Fill->GetForwardVector().GetSafeNormal();
    Irradiance = Fill->GetColoredLightBrightness();
    if (ToLight.ContainsNaN() || ToLight.IsNearlyZero() || !FMath::IsFinite(Irradiance.R)
        || !FMath::IsFinite(Irradiance.G) || !FMath::IsFinite(Irradiance.B)
        || Irradiance.R < 0 || Irradiance.G < 0 || Irradiance.B < 0) return false;
    Direction = FLinearColor(ToLight.X, ToLight.Y, ToLight.Z, 0);
    return true;
}
inline bool Write(UMaterialInstanceDynamic* Material, const FLinearColor& Direction, const FLinearColor& Irradiance)
{
    if (!Material) return false;
    FLinearColor OldDirection, OldIrradiance;
    if (!Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("APS_WaterFillDirection")), OldDirection)
        || OldDirection != Direction)
        Material->SetVectorParameterValue(TEXT("APS_WaterFillDirection"), Direction);
    if (!Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("APS_WaterFillIrradiance")), OldIrradiance)
        || OldIrradiance != Irradiance)
        Material->SetVectorParameterValue(TEXT("APS_WaterFillIrradiance"), Irradiance);
    return true;
}
inline bool Bind(UMaterialInstanceDynamic* Material, UWorld* World)
{
    FLinearColor Direction,Irradiance;
    return RenderContract() && Resolve(World,Direction,Irradiance) && Write(Material,Direction,Irradiance);
}
inline bool Matches(UMaterialInstanceDynamic* Material, UWorld* World)
{
    FLinearColor Direction, Irradiance, ActualDirection, ActualIrradiance;
    return Material && RenderContract() && Resolve(World, Direction, Irradiance)
        && Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("APS_WaterFillDirection")), ActualDirection)
        && Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("APS_WaterFillIrradiance")), ActualIrradiance)
        && ActualDirection.Equals(Direction, 1.e-6f) && ActualIrradiance.Equals(Irradiance, 1.e-6f);
}
}
