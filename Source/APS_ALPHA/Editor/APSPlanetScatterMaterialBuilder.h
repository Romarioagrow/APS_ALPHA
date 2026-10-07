#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetScatterMaterial.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

namespace APSPlanetScatterMaterialBuilder
{
inline bool Build(IAssetTools& Tools)
{
    using namespace APSPlanetScatterMaterial;
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    if (!IsRunningCommandlet()) return false;
    const FString MasterName(TEXT("M_APS_ScatterRock"));
    TArray<FString> Packages{FString(Folder) / MasterName};
    for (int32 I=0; I<5; ++I) Packages.Add(FSoftObjectPath(Path(I)).GetLongPackageName());
    for (const auto& P : Packages)
        if (FPackageName::DoesPackageExist(P) || FindPackage(nullptr,*P)) return false;
    const TCHAR* NormalPaths[] = {
        TEXT("Small_Rock/T_Small_Rock_normal.T_Small_Rock_normal"),
        TEXT("Rock_1/T_Rock_1_normal.T_Rock_1_normal"), TEXT("Rock_2/T_Rock_2_normal.T_Rock_2_normal"),
        TEXT("Cliff_1/T_Cliff_1_normal.T_Cliff_1_normal"), TEXT("Cliff_2/T_Cliff_2_normal.T_Cliff_2_normal")};
    auto* Albedo = LoadObject<UTexture>(nullptr,TEXT("/WorldScape/Ressources/Textures/Rock/T_Rock3.T_Rock3"));
    TArray<UTexture*> Normals;
    for (const auto* P : NormalPaths)
        Normals.Add(LoadObject<UTexture>(nullptr,*(FString(TEXT("/WorldScape/Ressources/Mesh/Rock/"))+P)));
    if (!Albedo || Normals.Contains(nullptr)) return false;
    FCore B(Tools,Folder);
    auto* M = Cast<UMaterial>(Tools.CreateAsset(MasterName,Folder,UMaterial::StaticClass(),NewObject<UMaterialFactoryNew>()));
    if (!M) return false;
    M->SetShadingModel(MSM_DefaultLit); M->BlendMode=BLEND_Opaque;
    auto* Color = B.Add<UMaterialExpressionTextureSample>(M);
    Color->Texture=Albedo; Color->SamplerType=SAMPLERTYPE_Color;
    auto* Normal = B.Add<UMaterialExpressionTextureSampleParameter2D>(M);
    Normal->ParameterName=TEXT("RockNormal"); Normal->Texture=Normals[0]; Normal->SamplerType=SAMPLERTYPE_Normal;
    Normal->UpdateParameterGuid(true,true);
    auto* TintNode = B.Add<UMaterialExpressionVectorParameter>(M);
    TintNode->ParameterName=TEXT("RockTint"); TintNode->DefaultValue=Tint(EPlanetType::Terrestrial);
    TintNode->UpdateParameterGuid(true,true);
    // Installed engine does not export this expression's StaticClass.
    auto* RandomClass=FindObject<UClass>(nullptr,TEXT("/Script/Engine.MaterialExpressionPerInstanceRandom"));
    if (!RandomClass) return false;
    auto* Random=UMaterialEditingLibrary::CreateMaterialExpression(M,RandomClass);
    auto* Shade = B.Add<UMaterialExpressionCustom>(M); Shade->Inputs.Empty(); Shade->OutputType=CMOT_Float3;
    Shade->Description=TEXT("Two-sample local-UV mineral; no planet climate masks, WPO or emissive");
    Shade->Code=TEXT("float grain=saturate(dot(C,float3(.2126,.7152,.0722))); return Tint*(.48+1.1*grain)*lerp(.86,1.10,saturate(R));");
    const auto Input=[&](const TCHAR* Name,UMaterialExpression* Node)
    { FCustomInput I; I.InputName=Name; I.Input.Connect(0,Node); Shade->Inputs.Add(I); };
    Input(TEXT("C"),Color); Input(TEXT("Tint"),TintNode); Input(TEXT("R"),Random);
    auto* Rough=B.Add<UMaterialExpressionConstant>(M); Rough->R=.86f;
    auto* Spec=B.Add<UMaterialExpressionConstant>(M); Spec->R=.25f;
    M->GetExpressionInputForProperty(MP_BaseColor)->Connect(0,Shade);
    M->GetExpressionInputForProperty(MP_Normal)->Connect(0,Normal);
    M->GetExpressionInputForProperty(MP_Roughness)->Connect(0,Rough);
    M->GetExpressionInputForProperty(MP_Specular)->Connect(0,Spec);
    // Usage changes compile immediately: register new texture references first.
    M->UpdateCachedExpressionData();
    bool Recompile=false; M->SetMaterialUsage(Recompile,MATUSAGE_InstancedStaticMeshes);
    M->PostEditChange();
    TArray<UMaterialInterface*> Outputs{M};
    for (int32 I=0; I<5; ++I)
    {
        const FSoftObjectPath ObjectPath(Path(I));
        auto* MI=Cast<UMaterialInstanceConstant>(Tools.CreateAsset(ObjectPath.GetAssetName(),Folder,
            UMaterialInstanceConstant::StaticClass(),NewObject<UMaterialInstanceConstantFactoryNew>()));
        if (!MI) return false;
        MI->SetParentEditorOnly(M,false);
        MI->SetTextureParameterValueEditorOnly(TEXT("RockNormal"),Normals[I]);
        MI->PostEditChange(); Outputs.Add(MI);
    }
    for (auto* O : Outputs) O->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* O : Outputs)
    {
        auto* R=O->GetMaterialResource(GMaxRHIFeatureLevel);
        auto* Map=R?R->GetGameThreadShaderMap():nullptr;
        if (!R || !Map || R->GetCompileErrors().Num() || !R->IsGameThreadShaderMapComplete()
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return false;
    }
    for (auto* O : Outputs)
    {
        auto* P=O->GetOutermost(); FSavePackageArgs Args;
        Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        if (!UPackage::SavePackage(P,O,*FPackageName::LongPackageNameToFilename(P->GetName(),
            FPackageName::GetAssetPackageExtension()),Args)) return false;
    }
    UE_LOG(LogTemp,Display,TEXT("[APS.ScatterMaterial] saved=6 normalMaps=5 textureSamples=2 noClimateMask=1; rendered acceptance pending"));
    return true;
}
}
#endif
