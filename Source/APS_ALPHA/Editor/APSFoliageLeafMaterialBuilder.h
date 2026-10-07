#pragma once
#if WITH_EDITOR
#include "APSSharedAmmoniaMaterialBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageLeafMaterial.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"

namespace APSFoliageLeafMaterialBuilder
{
inline bool Build(IAssetTools& AssetTools)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using namespace APSSharedAmmoniaMaterialBuilder;
    FCore Core(AssetTools, APSFoliageLeafMaterial::Folder);
    auto Fail = [&Core](const TCHAR* Why)
    { UE_LOG(LogTemp, Error, TEXT("[APS.FoliageLeaf] %s %s"), Why, *Core.Error); return false; };
    auto* SourceLeaf = LoadObject<UMaterialInstanceConstant>(nullptr,
        TEXT("/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf.MI_Grass_Leaf"));
    auto* SourceParent = SourceLeaf ? Cast<UMaterialInstanceConstant>(SourceLeaf->Parent.Get()) : nullptr;
    auto* SourceMaster = SourceParent ? Cast<UMaterial>(SourceParent->Parent.Get()) : nullptr;
    const auto Unchanged = [&]()
    {
        return SavedSourceMatches(SourceMaster, TEXT("F0396BBB265843493AEBEE0A055C6D7F16B210F0"))
            && SavedSourceMatches(SourceParent, TEXT("80D0FC6D77D9271E7E467F703C60DBDF897B1549"))
            && SavedSourceMatches(SourceLeaf, TEXT("CB8B2746720835E6D5D7977F900DB6A157F0F51E"));
    };
    if (!Unchanged() || SourceMaster->GetBlendMode() != BLEND_Masked) return Fail(TEXT("Vendor source drift"));
    auto* Master = Cast<UMaterial>(Core.Duplicate(SourceMaster, TEXT("M_APS_PrototypeLeaf")));
    auto* Parent = Cast<UMaterialInstanceConstant>(Core.Duplicate(SourceParent, TEXT("MI_APS_PrototypeFoliageParent")));
    auto* Leaf = Cast<UMaterialInstanceConstant>(Core.Duplicate(SourceLeaf, TEXT("MI_APS_PrototypeLeaf")));
    if (!Master || !Parent || !Leaf) return Fail(TEXT("New owned outputs only"));
    UMaterialExpressionVertexInterpolator* OldFade = nullptr;
    for (auto* E : FCore::Expressions(Master))
        if (E->GetFName() == TEXT("MaterialExpressionVertexInterpolator_0")) OldFade = Cast<UMaterialExpressionVertexInterpolator>(E);
    auto* Cutout = Cast<UMaterialExpressionMaterialFunctionCall>(Master->GetExpressionInputForProperty(MP_OpacityMask)->Expression);
    if (!OldFade || !OldFade->Input.Expression
        || OldFade->Input.Expression->GetFName() != TEXT("MaterialExpressionMultiply_14")
        || !Cutout || !Cutout->MaterialFunction
        || Cutout->MaterialFunction->GetPathName() != TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA"))
        return Fail(TEXT("Opacity topology drift"));
    // Keep the existing leaf alpha/cutout and all colour/normal art. Remove only
    // its mesh-radius-dependent camera mask (which erased the whole crown).
    auto* FullAlphaScale = Core.Add<UMaterialExpressionConstant>(Master); FullAlphaScale->R = 10;
    OldFade->Input = FExpressionInput(); OldFade->Input.Expression = FullAlphaScale;
    auto* Position = Core.Add<UMaterialExpressionWorldPosition>(Master);
    Position->WorldPositionShaderOffset = WPT_CameraRelativeNoOffsets;
    auto* Fade = Core.Add<UMaterialExpressionCustom>(Master);
    Fade->OutputType = CMOT_Float1; Fade->Inputs.Empty();
    Fade->Description = TEXT("Prototype leaf opacity: camera-relative 80-95m, before native 100m HISM cull");
    FCustomInput P; P.InputName = TEXT("P"); P.Input.Expression = Position; Fade->Inputs.Add(P);
    Fade->Code = FString::Printf(TEXT("return 1.0 - smoothstep(%.1f, %.1f, length(P));"),
        APSFoliageLeafMaterial::FadeStartCm, APSFoliageLeafMaterial::FadeEndCm);
    auto* Dither = Core.Add<UMaterialExpressionMaterialFunctionCall>(Master);
    if (!Dither->SetMaterialFunction(Cutout->MaterialFunction) || Dither->FunctionInputs.Num() != 2)
        return Fail(TEXT("Distance dither function contract"));
    // Newly created function pins use INDEX_NONE until explicitly connected.
    Dither->FunctionInputs[0].Input.Expression = Fade;
    Dither->FunctionInputs[0].Input.OutputIndex = 0;
    auto* Mask = Core.Add<UMaterialExpressionMultiply>(Master);
    Mask->A = *Master->GetExpressionInputForProperty(MP_OpacityMask);
    Mask->B.Expression = Dither;
    Master->GetExpressionInputForProperty(MP_OpacityMask)->Expression = Mask;
    Parent->SetParentEditorOnly(Master, false); Leaf->SetParentEditorOnly(Parent, false);
    bool NeedsRecompile = false;
    Master->SetMaterialUsage(NeedsRecompile, MATUSAGE_InstancedStaticMeshes);
    Master->PostEditChange(); Parent->PostEditChange(); Leaf->PostEditChange();
    for (auto* Output : Core.Outputs)
        CastChecked<UMaterialInterface>(Output)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* Output : Core.Outputs)
    {
        auto* Resource = CastChecked<UMaterialInterface>(Output)->GetMaterialResource(GMaxRHIFeatureLevel);
        auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        if (!Resource || !Map || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Fail(TEXT("Incomplete instanced-capable material"));
    }
    if (!Unchanged() || Core.Outputs.Num() != 3) return Fail(TEXT("Input/output drift"));
    for (auto* Output : Core.Outputs)
    {
        auto* Package = Output->GetOutermost();
        if (!Package->GetName().StartsWith(FString(APSFoliageLeafMaterial::Folder) + TEXT("/"))) return Fail(TEXT("Output escaped folder"));
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, Output, *Filename, Args)) return Fail(TEXT("Save failed"));
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.FoliageLeaf] saved=3 bound=0 fade=80-95m geometry/palette/instances/vendor=unchanged; rendered/cost validation pending"));
    return true;
}
}
#endif
