#pragma once
#if WITH_EDITOR
#include "APSSharedWaterMaterialBuilder.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionCustom.h"

// New diagnostic packages only. No runtime selection or shared-parent edits.
namespace APSWaterDepthMaterialBuilder
{
inline bool Build(IAssetTools& AssetTools)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using namespace APSSharedAmmoniaMaterialBuilder;
    constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepth20260927");
    FCore Core(AssetTools, Folder);
    auto Fail = [&Core](const TCHAR* Why)
    {
        UE_LOG(LogTemp, Error, TEXT("[APS.WaterDepth] %s %s"), Why, *Core.Error);
        return false;
    };
    auto* Source = LoadObject<UMaterial>(nullptr, APSSharedWaterMaterialBuilder::ParentPath);
    auto* Water = LoadObject<UMaterialInstanceConstant>(nullptr,
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedWater.MI_APS_SharedWater"));
    if (!SavedSourceMatches(Source, TEXT("91EA9BB6BAD5DE82BD29594B259DE4D94D52CCFC"))
        || !SavedSourceMatches(Water, TEXT("30ADFAAAF2EB11B5E4D30299FA886A67861E8CC8"))
        || Water->Parent.Get() != Source || Source->GetBlendMode() != BLEND_Masked)
        return Fail(TEXT("Accepted saved Water dependencies changed"));
    auto* Master = Cast<UMaterial>(Core.Duplicate(Source, TEXT("M_APS_WaterDepth")));
    if (!Master) return Fail(TEXT("New master only"));

    UMaterialExpressionLinearInterpolate* ColorMix = nullptr;
    for (UMaterialExpression* E : FCore::Expressions(Master))
    {
        auto* L = Cast<UMaterialExpressionLinearInterpolate>(E);
        auto* A = L ? Cast<UMaterialExpressionVectorParameter>(L->A.Expression) : nullptr;
        auto* B = L ? Cast<UMaterialExpressionVectorParameter>(L->B.Expression) : nullptr;
        if (A && B && A->ParameterName == TEXT("LiquidDeepColor") && B->ParameterName == TEXT("LiquidShallowColor"))
        {
            if (ColorMix) return Fail(TEXT("Ambiguous palette edge"));
            ColorMix = L;
        }
    }
    if (!ColorMix || !ColorMix->Alpha.Expression) return Fail(TEXT("Saved palette edge absent"));
    // Retain every existing edge/output except this one palette interpolation.
    TArray<TPair<FExpressionInput*, FExpressionInput>> Preserved;
    for (UMaterialExpression* E : FCore::Expressions(Master))
        for (FExpressionInput* Input : E->GetInputsView())
            if (Input && Input != &ColorMix->Alpha) Preserved.Emplace(Input, *Input);
    auto* UV = Core.Add<UMaterialExpressionTextureCoordinate>(Master);
    auto* Strength = Core.Add<UMaterialExpressionScalarParameter>(Master);
    auto* HalfDepth = Core.Add<UMaterialExpressionScalarParameter>(Master);
    auto* Alpha = Core.Add<UMaterialExpressionCustom>(Master);
    if (!UV || !Strength || !HalfDepth || !Alpha) return Fail(TEXT("Depth expressions could not be allocated"));
    UV->CoordinateIndex = 1;
    Strength->ParameterName = TEXT("APS_WaterDepthStrength");
    Strength->DefaultValue = 0.65f;
    Strength->SliderMin = 0.0f; Strength->SliderMax = 1.0f;
    Strength->UpdateParameterGuid(true, true);
    HalfDepth->ParameterName = TEXT("APS_WaterHalfDepthM");
    HalfDepth->DefaultValue = 80.0f;
    HalfDepth->SliderMin = 1.0f; HalfDepth->SliderMax = 500.0f;
    HalfDepth->UpdateParameterGuid(true, true);
    Alpha->OutputType = CMOT_Float1;
    Alpha->Description = TEXT("Signed physical depth km + validity; exponential palette attenuation, not scene depth");
    Alpha->Code = TEXT("if (!all(isfinite(DepthUV)) || DepthUV.y < 0.999 || DepthUV.y > 1.001) return Legacy;\n")
        TEXT("float depthM = max(DepthUV.x, 0.0) * 1000.0;\n")
        TEXT("float shallow = exp2(-depthM / max(HalfDepthM, 1.0));\n")
        TEXT("return lerp(Legacy, shallow, saturate(Strength));");
    const TPair<FName, UMaterialExpression*> Inputs[] = {
        {TEXT("DepthUV"), UV}, {TEXT("Legacy"), ColorMix->Alpha.Expression},
        {TEXT("Strength"), Strength}, {TEXT("HalfDepthM"), HalfDepth}};
    Alpha->Inputs.Reset();
    for (const auto& Pair : Inputs)
    {
        FCustomInput Input;
        Input.InputName = Pair.Key;
        Input.Input.Expression = Pair.Value;
        if (Pair.Key == TEXT("Legacy")) Input.Input = ColorMix->Alpha;
        Alpha->Inputs.Add(Input);
    }
    ColorMix->Alpha = FExpressionInput();
    ColorMix->Alpha.Expression = Alpha;
    for (const auto& Pair : Preserved)
        if (!SameInput(*Pair.Key, Pair.Value, false)) return Fail(TEXT("Unrelated graph edge changed"));
    for (EMaterialProperty Property : {MP_BaseColor, MP_Normal, MP_Roughness, MP_Metallic,
        MP_Specular, MP_EmissiveColor, MP_OpacityMask, MP_WorldPositionOffset, MP_PixelDepthOffset})
        if (!SameInput(*Source->GetExpressionInputForProperty(Property), *Master->GetExpressionInputForProperty(Property), true))
            return Fail(TEXT("Material output contract changed"));

    auto* Candidate = Cast<UMaterialInstanceConstant>(Core.Duplicate(Water, TEXT("MI_APS_WaterDepth")));
    if (!Candidate) return Fail(TEXT("New Water MIC only"));
    Candidate->SetParentEditorOnly(Master, false);
    Master->PostEditChange(); Candidate->PostEditChange();
    if (!APSSharedLavaMaterialBuilder::SameTypeState(Water, Candidate)
        || Master->TwoSided != Source->TwoSided || Master->bTangentSpaceNormal != Source->bTangentSpaceNormal
        || Master->OpacityMaskClipValue != Source->OpacityMaskClipValue
        || Master->GetBlendMode() != Source->GetBlendMode() || Core.Outputs.Num() != 2)
        return Fail(TEXT("Saved Water appearance/coverage parameters changed"));
    for (UObject* Output : Core.Outputs)
        CastChecked<UMaterialInterface>(Output)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (UObject* Output : Core.Outputs)
    {
        auto* Resource = CastChecked<UMaterialInterface>(Output)->GetMaterialResource(GMaxRHIFeatureLevel);
        auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        if (!Resource || !Map || !Resource->IsGameThreadShaderMapComplete()
            || Resource->GetCompileErrors().Num() || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
        {
            if (Resource) for (const FString& Error : Resource->GetCompileErrors())
                UE_LOG(LogTemp, Error, TEXT("[APS.WaterDepth] %s"), *Error);
            return Fail(TEXT("Candidate shader/LocalVF incomplete"));
        }
    }
    if (!SavedSourceMatches(Source, TEXT("91EA9BB6BAD5DE82BD29594B259DE4D94D52CCFC"))
        || !SavedSourceMatches(Water, TEXT("30ADFAAAF2EB11B5E4D30299FA886A67861E8CC8")))
        return Fail(TEXT("Read-only sources changed during candidate creation"));
    for (UObject* Output : Core.Outputs)
    {
        UPackage* Package = Output->GetOutermost();
        if (!Package->GetName().StartsWith(FString(Folder) + TEXT("/"))) return Fail(TEXT("Unexpected package path"));
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, Output, *Filename, Args)) return Fail(TEXT("Candidate save failed"));
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepth] Saved=2 bound=0 changedPaletteEdges=1 halfDepthM=80 strength=.65 acceptedParametersPreserved=1"));
    return true;
}
}
#endif
