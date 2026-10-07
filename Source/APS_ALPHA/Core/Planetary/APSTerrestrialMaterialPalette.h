#pragma once
#include "APSPlanetSurfaceProfile.h"
#include "APSTerrainContinuityMaterial.h"
#include "HAL/IConsoleManager.h"

// Material-uniform refinement, not exposure/postprocess or a new biome field.
// Enabled after matched ground/approach/orbit comparison; CVar 0 is rollback.
namespace APSTerrestrialMaterialPalette
{
inline bool Enabled()
{
    const auto* V = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.TerrestrialPalette"));
    return V && V->GetInt() != 0;
}
inline bool Allows(EPlanetType Type, const FString& ParentPath)
{
    return Type == EPlanetType::Terrestrial
        && (ParentPath == APSTerrainContinuityMaterial::TemplatePath
            || ParentPath == TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra"));
}
inline float Luminance(const FLinearColor& C)
{
    return C.R * 0.2126f + C.G * 0.7152f + C.B * 0.0722f;
}
inline float ChromaScale(FName Name)
{
    if (Name == TEXT("Color1") || Name == TEXT("Color2")) return 0.52f;
    if (Name == TEXT("Color3")) return 0.65f;
    if (Name == TEXT("BottomColor") || Name == TEXT("Sedimentcolor") || Name == TEXT("2_Color2")) return 0.65f;
    if (Name == TEXT("Color4") || Name == TEXT("Color5") || Name == TEXT("2_Color1") || Name == TEXT("2_Color4")) return 0.62f;
    if (Name == TEXT("Color2_3") || Name == TEXT("2_Color3")) return 0.80f;
    return 1.0f; // Snow, slope, emissive, global tint and frame uniforms are untouched.
}
inline FLinearColor Refine(FName Name, const FLinearColor& Color)
{
    const float Scale = ChromaScale(Name);
    if (Scale == 1.0f || !FMath::IsFinite(Color.R) || !FMath::IsFinite(Color.G)
        || !FMath::IsFinite(Color.B) || !FMath::IsFinite(Color.A)) return Color;
    const float Y = Luminance(Color);
    // Convex mix in linear RGB preserves luminance and authored hue ordering.
    // Reduce yellow/green chroma without a blanket brightness reduction.
    return FLinearColor(FMath::Lerp(Y, Color.R, Scale), FMath::Lerp(Y, Color.G, Scale),
        FMath::Lerp(Y, Color.B, Scale), Color.A);
}
}
