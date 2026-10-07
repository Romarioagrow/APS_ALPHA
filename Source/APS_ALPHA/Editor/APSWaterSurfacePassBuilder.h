#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "Materials/MaterialExpressionSingleLayerWaterMaterialOutput.h"

// Water-only candidate. Keep the same opaque, depth-writing sphere and native
// bathymetry; route its lighting through UE's water pass, not opaque diffuse AO.
// No transmission: UE5.4's world-Z bottom approximation is not planet-radial.
namespace APSWaterSurfacePassBuilder
{
inline bool Add(APSSharedTerrainMaterialBuilder::FBuild& Core, UMaterial* Master)
{
    if (!Master || Master->bTangentSpaceNormal || Master->GetBlendMode() != BLEND_Masked
        || !Master->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit)
        || Master->GetExpressionInputForProperty(MP_Opacity)->Expression)
        return false;
    for (UMaterialExpression* E : Core.Expressions(Master))
        if (Cast<UMaterialExpressionSingleLayerWaterMaterialOutput>(E)) return false;

    // The forward water pass omits our optional second, diffuse-only directional
    // fill. Transport its ACTUAL irradiance; never multiply the palette to fake it.
    // A missing/unbound fill is black. Runtime must validate the Lambert model.
    auto* Fill = Core.Add<UMaterialExpressionCustom>(Master);
    Fill->OutputType = CMOT_Float3;
    Fill->Description = TEXT("Water secondary scene fill: actual Lambert irradiance, no albedo gain");
    for (const auto& P : {TPair<FName, EMaterialProperty>(TEXT("Base"), MP_BaseColor),
        {TEXT("N"), MP_Normal}, {TEXT("Metal"), MP_Metallic}, {TEXT("Existing"), MP_EmissiveColor}})
    {
        const FExpressionInput* Source = Master->GetExpressionInputForProperty(P.Value);
        if (!Source || !Source->Expression) return false;
        FCustomInput I; I.InputName = P.Key; I.Input = *Source; Fill->Inputs.Add(I);
    }
    for (const auto& P : {TPair<const TCHAR*, const TCHAR*>(TEXT("APS_WaterFillDirection"), TEXT("ToLight")),
        {TEXT("APS_WaterFillIrradiance"), TEXT("Irradiance")}})
    {
        auto* V = Core.Add<UMaterialExpressionVectorParameter>(Master);
        V->ParameterName = P.Key; V->DefaultValue = FLinearColor::Black;
        V->Group = TEXT("APS Scene-derived Surface Fill"); V->UpdateParameterGuid(true, true);
        FCustomInput I; I.InputName = P.Value; I.Input.Expression = V;
        I.Input.Mask = I.Input.MaskR = I.Input.MaskG = I.Input.MaskB = 1;
        Fill->Inputs.Add(I);
    }
    Fill->Code = TEXT("if (!all(isfinite(Irradiance)) || !any(Irradiance > 0.0) || dot(ToLight,ToLight) < 0.5) return Existing;\n")
        TEXT("float NoL = saturate(dot(normalize(N), normalize(ToLight)));\n")
        TEXT("return Existing + saturate(Base) * (1.0-saturate(Metal)) * max(Irradiance,0.0) * (NoL * 0.318309886184);\n");
    auto* Volume = Core.Add<UMaterialExpressionSingleLayerWaterMaterialOutput>(Master);
    auto* Zero = Core.Add<UMaterialExpressionConstant3Vector>(Master);
    auto* One = Core.Add<UMaterialExpressionConstant>(Master);
    Zero->Constant = FLinearColor::Black; One->R = 1.0f;
    Volume->ScatteringCoefficients.Expression = Zero;
    Volume->AbsorptionCoefficients.Expression = Zero;
    Volume->ColorScaleBehindWater.Expression = One;
    Master->GetExpressionInputForProperty(MP_Opacity)->Connect(0, One);
    Master->GetExpressionInputForProperty(MP_EmissiveColor)->Connect(0, Fill);
    Master->SetShadingModel(MSM_SingleLayerWater);
    return true;
}
}
#endif
