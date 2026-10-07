#pragma once
#if WITH_EDITOR
#include "APSSharedWaterMaterialBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSCoastalWaterMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedWaterMaterial.h"

// Save the rendered graph with the exact tested style. Never overwrite Shared,
// diagnostic source assets, the catalog or another liquid family's packages.
namespace APSCoastalWaterPublisher
{
inline bool Build(IAssetTools& Tools)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using namespace APSSharedAmmoniaMaterialBuilder;
    FCore Core(Tools, APSCoastalWaterMaterial::Folder);
    const auto Fail = [&Core](const TCHAR* Why)
    { UE_LOG(LogTemp, Error, TEXT("[APS.CoastalWaterPublish] %s %s"), Why, *Core.Error); return false; };
    if (!IsRunningCommandlet()) return Fail(TEXT("Offline commandlet only"));
    auto* Source = LoadObject<UMaterial>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930/M_APS_WaterAnalytic.M_APS_WaterAnalytic"));
    auto* SourceMI = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930/MI_APS_WaterAnalytic.MI_APS_WaterAnalytic"));
    auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, APSSharedWaterMaterial::TemplatePath());
    const auto Unchanged = [&]()
    { return SavedSourceMatches(Source, TEXT("21C652A544125339495E87E3A8767C00FDC2E8F3"))
        && SavedSourceMatches(SourceMI, TEXT("133A0EF6927CA02B3EFA84ACD117A0BC141D9611"))
        && SavedSourceMatches(Native, TEXT("30ADFAAAF2EB11B5E4D30299FA886A67861E8CC8")); };
    if (!Unchanged() || SourceMI->Parent.Get() != Source || Source->bTangentSpaceNormal
        || Source->GetBlendMode() != BLEND_Masked || !Source->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit))
        return Fail(TEXT("Rendered source contract drift"));
    if (FPackageName::DoesPackageExist(FString(APSCoastalWaterMaterial::Folder) / TEXT("M_APS_CoastalWater"))
        || FPackageName::DoesPackageExist(FString(APSCoastalWaterMaterial::Folder) / TEXT("MI_APS_CoastalWater")))
        return Fail(TEXT("Release exists; never overwrite"));
    auto* M = Cast<UMaterial>(Core.Duplicate(Source, TEXT("M_APS_CoastalWater")));
    auto* MI = Cast<UMaterialInstanceConstant>(Core.Duplicate(SourceMI, TEXT("MI_APS_CoastalWater")));
    if (!M || !MI) return Fail(TEXT("New outputs only"));
    MI->SetParentEditorOnly(M, false);
    // Same starting uniforms as the live-route probe, followed by only its
    // tested style overrides. The frame/context are bound by the runtime owner.
    MI->CopyMaterialUniformParametersEditorOnly(Native, true);
    MI->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("LiquidDeepColor")), FLinearColor(.0025f,.009f,.018f,1));
    MI->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("LiquidShallowColor")), FLinearColor(.025f,.060f,.055f,1));
    const TPair<FName,float> Scalars[] = {
        {TEXT("APS_WaterDepthStrength"),1}, {TEXT("APS_WaterHalfDepthM"),20},
        {TEXT("WaveScaleCm"),120}, {TEXT("PhysicalWaveDetailScaleCm"),41},
        {TEXT("WaveColorStrength"),0}, {TEXT("PhysicalWaveRoughnessStrength"),0}};
    for (const auto& S : Scalars) MI->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(S.Key), S.Value);
    MI->PostEditChange(); M->PostEditChange();
    for (auto* Output : Core.Outputs)
        CastChecked<UMaterialInterface>(Output)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* Output : Core.Outputs)
    {
        auto* R = CastChecked<UMaterialInterface>(Output)->GetMaterialResource(GMaxRHIFeatureLevel);
        const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
            || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Fail(TEXT("Shader/LocalVF incomplete"));
    }
    if (!Unchanged() || Core.Outputs.Num() != 2) return Fail(TEXT("Protected inputs changed"));
    for (auto* Output : Core.Outputs)
    {
        auto* Package = Output->GetOutermost();
        if (!Package->GetName().StartsWith(FString(APSCoastalWaterMaterial::Folder)+TEXT("/"))) return Fail(TEXT("Escaped output"));
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        const FString File=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package,Output,*File,Args)) return Fail(TEXT("Save failed"));
    }
    UE_LOG(LogTemp,Display,TEXT("[APS.CoastalWaterPublish] saved=2 sourceUnchanged=1 runtimeDefault=0; release-path render validation required"));
    return true;
}
}
#endif
