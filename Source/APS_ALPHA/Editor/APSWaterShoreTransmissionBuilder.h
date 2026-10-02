#pragma once
#if WITH_EDITOR
#include "APSSharedWaterMaterialBuilder.h"
#include "APSWaterSurfacePassBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSCoastalWaterMaterial.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionMultiply.h"

// Opt-in candidate: transmit the actual seabed near the zero-depth boundary.
// No geometry/dither/sea-level edits, no planet-inappropriate world-Z extinction.
namespace APSWaterShoreTransmissionBuilder
{
inline bool Build(IAssetTools& Tools)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using namespace APSSharedAmmoniaMaterialBuilder;
    const TCHAR* Folder=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterShoreTransmission20261001");
    FCore Core(Tools, Folder);
    const auto Fail=[&](const TCHAR* Why)
    { UE_LOG(LogTemp,Error,TEXT("[APS.WaterShoreTransmission] %s %s"),Why,*Core.Error); return false; };
    if (!IsRunningCommandlet()) return Fail(TEXT("Offline commandlet only"));
    auto* Source=LoadObject<UMaterial>(nullptr,APSCoastalWaterMaterial::MasterPath);
    auto* SourceMI=LoadObject<UMaterialInstanceConstant>(nullptr,APSCoastalWaterMaterial::TemplatePath);
    const auto Unchanged=[&]()
    { return SavedSourceMatches(Source,TEXT("F9AA87B3A0201AB8F73E1216FEB4B7F6EC03BC2F"))
        && SavedSourceMatches(SourceMI,TEXT("AFA128921FBFB6C482CD18A430B6DC502A98CF87")); };
    if (!Unchanged() || !SourceMI || SourceMI->Parent.Get()!=Source) return Fail(TEXT("Published source changed"));
    auto* M=Cast<UMaterial>(Core.Duplicate(Source,TEXT("M_APS_WaterShoreTransmission")));
    auto* MI=Cast<UMaterialInstanceConstant>(Core.Duplicate(SourceMI,TEXT("MI_APS_WaterShoreTransmission")));
    if (!M || !MI || !APSWaterSurfacePassBuilder::Add(Core,M)) return Fail(TEXT("New water pass setup failed"));
    auto* Depth=Core.Add<UMaterialExpressionTextureCoordinate>(M);
    auto* Ramp=Core.Add<UMaterialExpressionCustom>(M);
    auto* Extent=Core.Add<UMaterialExpressionScalarParameter>(M);
    auto* Refraction=Core.Add<UMaterialExpressionConstant>(M);
    auto* Emissive=Core.Add<UMaterialExpressionMultiply>(M);
    if (!Depth || !Ramp || !Extent || !Refraction || !Emissive) return Fail(TEXT("Expression allocation failed"));
    Depth->CoordinateIndex=1;
    Extent->ParameterName=TEXT("APS_ShoreOpaqueDepthM"); Extent->DefaultValue=3.0f;
    Extent->UpdateParameterGuid(true,true);
    Ramp->OutputType=CMOT_Float1;
    Ramp->Description=TEXT("Radial native depth: seabed at shore, saved opaque water beyond 3m; no stochastic mask");
    Ramp->Code=TEXT("if (!all(isfinite(Depth)) || abs(Depth.y-1.0)>0.001) return 1.0;\n")
        TEXT("float x=saturate(max(Depth.x,0.0)*1000.0/max(Extent,0.01));\nreturn x*x*(3.0-2.0*x);");
    for (const auto& P : {TPair<FName,UMaterialExpression*>(TEXT("Depth"),Depth),{TEXT("Extent"),Extent}})
    { FCustomInput I; I.InputName=P.Key; I.Input.Expression=P.Value; Ramp->Inputs.Add(I); }
    M->GetExpressionInputForProperty(MP_Opacity)->Connect(0,Ramp);
    Refraction->R=1.0f;
    M->GetExpressionInputForProperty(MP_Refraction)->Connect(0,Refraction);
    Emissive->A=*M->GetExpressionInputForProperty(MP_EmissiveColor);
    Emissive->B.Expression=Ramp;
    M->GetExpressionInputForProperty(MP_EmissiveColor)->Connect(0,Emissive);
    MI->SetParentEditorOnly(M,false);
    M->PostEditChange(); MI->PostEditChange();
    for (auto* O:Core.Outputs) CastChecked<UMaterialInterface>(O)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* O:Core.Outputs)
    {
        auto* R=CastChecked<UMaterialInterface>(O)->GetMaterialResource(GMaxRHIFeatureLevel);
        auto* Map=R ? R->GetGameThreadShaderMap() : nullptr;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Fail(TEXT("Shader/LocalVF incomplete"));
    }
    if (!Unchanged() || Core.Outputs.Num()!=2) return Fail(TEXT("Protected source/output contract changed"));
    for (auto* O:Core.Outputs)
    {
        auto* P=O->GetOutermost();
        if (!P->GetName().StartsWith(FString(Folder)+TEXT("/"))) return Fail(TEXT("Output path escaped"));
        P->MarkPackageDirty(); FSavePackageArgs A; A.TopLevelFlags=RF_Public|RF_Standalone; A.SaveFlags=SAVE_NoError;
        const auto File=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(P,O,*File,A)) return Fail(TEXT("Save failed"));
    }
    UE_LOG(LogTemp,Display,TEXT("[APS.WaterShoreTransmission] saved=2 sourceUnchanged=1 opaqueDepthM=3 extinction=0 default=OFF"));
    return true;
}
}
#endif
