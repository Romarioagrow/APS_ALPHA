#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainLodABBuilder.h"
#include "APSNormalHexSampling.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"

// Diagnostic candidates only. Preserve source normal maps, projection axes,
// RNM/layer blending and geometric normals. Warp uses the native fetch count;
// hex intentionally spends three samples per projection to break repetition.
namespace APSNormalMacroWarpABBuilder
{
    inline constexpr const TCHAR* Destination=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/NormalMacroWarp20260929V1");
    struct FBuilder
    {
        using FBuild=APSSharedTerrainMaterialBuilder::FBuild;
        FBuild B;
        APSSharedTerrainNormalContinuity::TTransform<FBuild> Reader;
        const bool bHex;
        const bool bCombined;
        explicit FBuilder(IAssetTools& Tools,bool Hex=false,bool Combined=false):B(Tools,
            Combined?TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuityCombined20260929V1"):
            Hex?TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/NormalHex20260929V1"):Destination),Reader(B),bHex(Hex),bCombined(Combined){}
        bool Fail(const FString& Error){B.Error=Error;return false;}
        bool Patch(UMaterialFunction* Function)
        {
            if(bHex)return APSNormalHexSampling::Patch(B,Reader,Function);
            int32 Count=0, Samples=0;
            for (UMaterialExpression* E:Reader.Graph(Function))
            {
                Reader.Register(Function,E);
                if (auto* Sample=Cast<UMaterialExpressionTextureSample>(E))
                {
                    if (Sample->MipValueMode!=TMVM_Derivative || Sample->SamplerSource!=SSM_Wrap_WorldGroupSettings)
                        return Fail(TEXT("Unexpected normal sampling contract"));
                    ++Samples;
                }
                auto* UV=Cast<UMaterialExpressionCustom>(E);
                if (!UV || UV->Description!=TEXT("APS compensated planet-fixed detail UV, bounded detiling and unwrapped gradients")) continue;
                auto* Mode=B.Add<UMaterialExpressionScalarParameter>(Function);
                Mode->ParameterName=TEXT("APS_NormalMacroWarpMode"); Mode->DefaultValue=1;
                Mode->Group=TEXT("APS Normal Macro Diagnostic"); Mode->UpdateParameterGuid(true,true);
                FCustomInput Input; Input.InputName=TEXT("MacroWarpMode"); Input.Input.Expression=Mode; UV->Inputs.Add(Input);
                // Existing compensated physical -> signed-size transform remains.
                // Fine periods <=50m are exact legacy. Above200m, each row of the
                // added Jacobian has abs-sum <.75: no fold, no sudden tile seams.
                // Derivatives below see the final unwrapped warped coordinates.
                const TCHAR* Insert=TEXT(R"HLSL(FDFVector3 Unwrapped = DFDivide(Physical, Size);
float3 Period=abs(Size); // Some inherited calls specialize Size as a scalar.
float MacroStrength=saturate(MacroWarpMode)*smoothstep(5000.0,20000.0,min(Period.x,min(Period.y,Period.z)));
if(MacroStrength>0.0)
{
    float3 A=DFFracDemote(DFDivide(Unwrapped,float3(7.13,8.57,9.73)));
    float3 B=DFFracDemote(DFDivide(Unwrapped,float3(13.17,15.71,18.13)));
    float3 Offset=.67*sin(6.28318530718*(A.yzx+float3(.173,.419,.731)))
                 +.31*sin(6.28318530718*(B.zxy+float3(.613,.287,.947)));
    Unwrapped=DFAdd(Unwrapped,MacroStrength*Offset);
}
)HLSL");
                if (UV->Code.ReplaceInline(TEXT("FDFVector3 Unwrapped = DFDivide(Physical, Size);\n"),Insert,ESearchCase::CaseSensitive)!=1)
                    return Fail(TEXT("Audited compensated normal domain anchor changed"));
                UV->Description=TEXT("APS macro-normal bounded planet-fixed domain shear; native <=50m periods");
                ++Count;
            }
            return Count==1 && Samples==3 && B.Error.IsEmpty() ? true : Fail(TEXT("Expected one normal domain and three unchanged samples"));
        }
        bool Run()
        {
            if(bCombined && !bHex)return Fail(TEXT("Combined candidate requires hex normal sampling"));
            auto* Source=LoadObject<UMaterial>(nullptr,TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/M_APS_SharedWorldScapeTerrain.M_APS_SharedWorldScapeTerrain"));
            auto* Template=LoadObject<UMaterialInstanceConstant>(nullptr,TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra"));
            if(!Source || !Template || Template->Parent!=Source || Source->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty())
                return Fail(TEXT("Saved current native master/template required"));
            auto* Master=Cast<UMaterial>(B.Duplicate(Source,TEXT("M_APS_NormalWarpTerrain")));
            if(!Master)return false;
            UMaterialFunction* Copy=nullptr;
            int32 Calls=0;
            for(UMaterialExpression* E:Reader.Graph(Master))
            {
                Reader.Register(Master,E);
                auto* Call=Cast<UMaterialExpressionMaterialFunctionCall>(E);
                auto* Function=Call?Cast<UMaterialFunction>(Call->MaterialFunction):nullptr;
                if(!Function || Function->GetPathName()!=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_WorldAlignedTexture_a83aa78c.MF_APS_WorldAlignedTexture_a83aa78c"))continue;
                UMaterialExpressionTextureObjectParameter* Texture=nullptr;
                for(const auto& Input:Call->FunctionInputs)
                    if(Input.Input.InputName==TEXT("TextureObject"))Texture=Cast<UMaterialExpressionTextureObjectParameter>(Input.Input.Expression);
                if(!Texture || !(Texture->ParameterName==TEXT("SlopeNormal") || Texture->ParameterName==TEXT("SlopeNormal3") || Texture->ParameterName==TEXT("MacroSlopeNormal")))continue;
                if(!Copy)
                {
                    Copy=Cast<UMaterialFunction>(B.Duplicate(Function,TEXT("MF_APS_NormalMacroCoordinates")));
                    if(!Copy || !Patch(Copy))return false;
                    UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
                }
                if(!B.ReconnectFunctionById(Call,Function,Copy))return false;
                ++Calls;
            }
            if(Calls!=5)return Fail(FString::Printf(TEXT("Expected five audited macro-normal calls; found%d"),Calls));
            if(bCombined)
            {
                auto* Mode=Reader.Add<UMaterialExpressionScalarParameter>(Master);
                Mode->ParameterName=TEXT("APS_NormalMacroWarpMode");Mode->DefaultValue=1;
                Mode->Group=TEXT("APS Normal Macro Diagnostic");Mode->UpdateParameterGuid(true,true);
                if(!APSOrbitalColorFieldsAB::Patch(B,Master,Template,Mode))return false;
            }
            auto* Instance=Cast<UMaterialInstanceConstant>(B.Duplicate(Template,TEXT("MI_APS_NormalWarpTerra")));
            if(!Instance)return false;
            Instance->SetParentEditorOnly(Master,false); Instance->CopyMaterialUniformParametersEditorOnly(Template,true);
            Instance->PostEditChange(); Master->PostEditChange();
            APSSharedTerrainLodABBuilder::FBuilder PinRepair(B.AssetTools);
            if(!PinRepair.RestoreTransientFunctionPins(Master))return Fail(PinRepair.B.Error);
            for(UObject* O:B.Outputs)if(auto* M=Cast<UMaterialInterface>(O))M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
            for(UObject* O:B.Outputs)if(auto* M=Cast<UMaterialInterface>(O))
            {
                FMaterialResource* R=M->GetMaterialResource(GMaxRHIFeatureLevel);
                const FMaterialShaderMap* Map=R?R->GetGameThreadShaderMap():nullptr;
                if(!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                {
                    FString Error=TEXT("Normal candidate shader incomplete: ")+M->GetPathName();
                    if(R)for(const FString& Line:R->GetCompileErrors())Error+=TEXT("\n")+Line;
                    return Fail(Error);
                }
            }
            for(UObject* O:B.Outputs)
            {
                UPackage* P=O->GetOutermost(); P->MarkPackageDirty();
                FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
                if(!UPackage::SavePackage(P,O,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()),Args))return Fail(TEXT("Normal candidate save failed"));
            }
            UE_LOG(LogTemp,Display,TEXT("[APS.NormalMacroWarp] Saved isolated candidate hex=%d combinedFields=%d:5 normal calls,3 projections per call; native <=50m periods; no production publication"),bHex,bCombined);
            return true;
        }
    };
    inline bool Build(IAssetTools& Tools,bool Hex=false,bool Combined=false)
    {
        FBuilder Builder(Tools,Hex,Combined); const bool Result=Builder.Run();
        if(!Result)UE_LOG(LogTemp,Error,TEXT("[APS.NormalMacroWarp] Refused: %s"),*Builder.B.Error);
        return Result;
    }
}
#endif
