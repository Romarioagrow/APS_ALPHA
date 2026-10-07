#pragma once
#if WITH_EDITOR
#include "APSCanonicalCoverageUpdate.h"
#include "APSOrbitalReliefLightingUpdate.h"
#include "APSCanonicalPixelSlopeUpdate.h"
#include "APSContinuousOriginalColorUpdate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"

// Explicit offline causal experiment. Exactly two owned packages may be saved;
// every other caller receives the old slope through the optional input default.
namespace APSCanonicalPixelSlopePublisher
{
    constexpr const TCHAR* OriginalMaster = TEXT("9541C8FB506D3F95E277D97C2FB1E7D626A420B0");
    constexpr const TCHAR* OriginalSlope = TEXT("3FF327494FFB83C3521A5BA1318FA913C3303AC2");
    constexpr const TCHAR* SlopePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_SlopeBlock_f6be5407.MF_APS_MF_SlopeBlock_f6be5407");
    inline FString Hash(const FString& File)
    {
        TArray<uint8> Bytes;
        return FFileHelper::LoadFileToArray(Bytes,*File)
            ? FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num()).ToString().ToUpper() : FString();
    }
    inline FString Filename(const TCHAR* Object)
    {
        return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
            FPackageName::ObjectPathToPackageName(FString(Object)),FPackageName::GetAssetPackageExtension()));
    }
    inline bool Run(IAssetTools& Tools, bool bVerifyOnly)
    {
        const auto Refuse=[](const FString& Why)
        { UE_LOG(LogTemp,Error,TEXT("[APS.CanonicalSlope.Publish] REFUSED %s"),*Why); return false; };
        if(!IsRunningCommandlet() || !IsInGameThread() || !FApp::CanEverRender())
            return Refuse(TEXT("Requires isolated rendering-enabled offline commandlet"));
        FString ExpectedMaster=OriginalMaster,ExpectedSlope=OriginalSlope;
        if(bVerifyOnly && (!FParse::Value(FCommandLine::Get(),TEXT("APSSlopeExpectedMasterSHA1="),ExpectedMaster)
            || !FParse::Value(FCommandLine::Get(),TEXT("APSSlopeExpectedFunctionSHA1="),ExpectedSlope)
            || ExpectedMaster.Len()!=40 || ExpectedSlope.Len()!=40
            || ExpectedMaster.Equals(OriginalMaster,ESearchCase::IgnoreCase)
            || ExpectedSlope.Equals(OriginalSlope,ESearchCase::IgnoreCase)))
            return Refuse(TEXT("Cold verification needs both explicitly recorded candidate hashes"));
        const FString MasterFile=Filename(APSTerrainContinuityMaterial::MasterPath);
        const FString SlopeFile=Filename(SlopePath);
        const FString TemplateFile=Filename(APSTerrainContinuityMaterial::TemplatePath);
        const FString TemplateHash=Hash(TemplateFile);
        if(Hash(MasterFile)!=ExpectedMaster.ToUpper() || Hash(SlopeFile)!=ExpectedSlope.ToUpper() || TemplateHash.IsEmpty())
            return Refuse(TEXT("Unknown master/function/MIC state; exact hash guard failed"));
        for(const auto& File:{MasterFile,SlopeFile,TemplateFile})
            for(const TCHAR* Ext:{TEXT("uexp"),TEXT("ubulk")})
                if(IFileManager::Get().FileExists(*FPaths::ChangeExtension(File,Ext)))
                    return Refuse(TEXT("Unexpected sidecar requires full preservation: ")+File);
        auto* Template=LoadObject<UMaterialInstanceConstant>(nullptr,APSTerrainContinuityMaterial::TemplatePath);
        auto* Master=LoadObject<UMaterial>(nullptr,APSTerrainContinuityMaterial::MasterPath);
        auto* Slope=LoadObject<UMaterialFunction>(nullptr,SlopePath);
        if(!Master || !Template || !Slope || Template->Parent.Get()!=Master
            || Master->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty() || Slope->GetOutermost()->IsDirty())
            return Refuse(TEXT("Clean exact loaded master/function/MIC required"));
        FString Error,Backup;
        if(!bVerifyOnly)
        {
            if(!APSOrbitalReliefLightingUpdate::Patch(Master,Error,false))return Refuse(Error);
            Backup=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("CanonicalSlopeBackup_9541_3FF32749"));
            if(IFileManager::Get().DirectoryExists(*Backup) || !IFileManager::Get().MakeDirectory(*Backup,true))
                return Refuse(TEXT("Backup exists/cannot be created; no graph patch performed"));
            for(const auto& File:{MasterFile,SlopeFile,TemplateFile})
            {
                const FString Expected=Hash(File),Dest=Backup/FPaths::GetCleanFilename(File);
                if(Expected.IsEmpty() || IFileManager::Get().Copy(*Dest,*File,false,false)!=COPY_OK || Hash(Dest)!=Expected)
                    return Refuse(TEXT("Exact preimage failed before graph patch: ")+File);
            }
            if(!APSCanonicalCoverageUpdate::Patch(Master,Error))return Refuse(Error);
            UMaterialExpressionCustom* Sample=nullptr;
            UMaterialExpressionScalarParameter* Coverage=nullptr;
            TArray<UObject*> Children;GetObjectsWithOuter(Master,Children,false);
            for(auto* Child:Children)
            {
                if(auto* C=Cast<UMaterialExpressionCustom>(Child))
                    if(C->Description==TEXT("APS canonical stereo coverage sample v1"))
                    {if(Sample)return Refuse(TEXT("Ambiguous canonical sample"));Sample=C;}
                if(auto* P=Cast<UMaterialExpressionScalarParameter>(Child))
                {
                    if(P->ParameterName==TEXT("APS_CanonicalSlopeAvailable"))return Refuse(TEXT("Slope switch already exists"));
                    if(P->ParameterName==TEXT("APS_CanonicalCoverageAvailable"))
                    {if(Coverage)return Refuse(TEXT("Ambiguous canonical availability"));Coverage=P;}
                }
            }
            if(!Sample || !Coverage || Coverage->DefaultValue!=0)return Refuse(TEXT("Missing disabled canonical source"));
            const auto Register=[&](UMaterialExpression* E)
            {E->Material=Master;E->MaterialExpressionGuid=FGuid::NewGuid();Master->GetExpressionCollection().AddExpression(E);};
            auto* Available=NewObject<UMaterialExpressionScalarParameter>(Master);Register(Available);
            Available->ParameterName=TEXT("APS_CanonicalSlopeAvailable");Available->DefaultValue=0;
            Available->Group=TEXT("APS Canonical Coverage");Available->UpdateParameterGuid(true,true);
            auto* Pack=NewObject<UMaterialExpressionCustom>(Master);Register(Pack);
            Pack->Description=TEXT("APS canonical pixel slope optional payload v1");Pack->OutputType=CMOT_Float4;
            Pack->Code=TEXT("return float4(Canonical.xyz,saturate(SlopeAvailable)*saturate(CoverageAvailable)*saturate(Canonical.w));");
            Pack->Inputs.Reset();Pack->Outputs.Reset();Pack->Outputs.Add(FExpressionOutput());
            const auto Add=[&](const TCHAR* Name,UMaterialExpression* E)
            {FCustomInput P;P.InputName=Name;P.Input.Expression=E;Pack->Inputs.Add(P);};
            Add(TEXT("Canonical"),Sample);Add(TEXT("SlopeAvailable"),Available);Add(TEXT("CoverageAvailable"),Coverage);
            FExpressionInput Payload;Payload.Expression=Pack;
            if(!APSCanonicalPixelSlopeUpdate::Patch(Master,Slope,Payload,Error))return Refuse(Error);
            Slope->StateId=FGuid::NewGuid();
            APSSharedTerrainLodABBuilder::FBuilder Pins(Tools);
            if(!Pins.RestoreTransientFunctionPins(Master))return Refuse(Pins.B.Error);
        }
        // Cold path intentionally performs NO function-pointer repair. This must
        // compile the saved graph after ordinary engine load, not a warm repair.
        if(!APSCanonicalPixelSlopeUpdate::VerifyPatched(Master,Slope,Error))return Refuse(Error);
        UMaterialEditingLibrary::RecompileMaterial(Master);
        for(UMaterialInterface* M:{static_cast<UMaterialInterface*>(Master),static_cast<UMaterialInterface*>(Template)})
        {
            M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
            if(!APSContinuousOriginalColorUpdate::ShaderReady(M,Error))return Refuse(Error);
        }
        float SlopeDefault=-1,CoverageDefault=-1;
        if(!APSCanonicalPixelSlopeUpdate::VerifyPatched(Master,Slope,Error))return Refuse(Error);
        if(!Template->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_CanonicalSlopeAvailable")),SlopeDefault)
            || !Template->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_CanonicalCoverageAvailable")),CoverageDefault)
            || SlopeDefault!=0 || CoverageDefault!=0 || Template->Parent.Get()!=Master
            || Hash(MasterFile)!=ExpectedMaster.ToUpper() || Hash(SlopeFile)!=ExpectedSlope.ToUpper() || Hash(TemplateFile)!=TemplateHash)
            return Refuse(TEXT("Default/parent/disk preservation failed before save"));
        if(bVerifyOnly)
        {
            UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope.Publish] COLD_VERIFY masterSHA1=%s slopeSHA1=%s defaults=0/0 shaderReady=1 noRepair=1 noSave=1; not visual acceptance"),*ExpectedMaster,*ExpectedSlope);
            return true;
        }
        // Both were compiled before either save. A failed later save is NOT
        // success: caller restores both immutable preimages after process exit.
        FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
        for(UObject* Asset:{static_cast<UObject*>(Slope),static_cast<UObject*>(Master)})
        {
            UPackage* P=Asset->GetOutermost();P->MarkPackageDirty();
            const FString File=Asset==Master?MasterFile:SlopeFile;
            if(!UPackage::SavePackage(P,Asset,*File,Args))
                return Refuse(TEXT("Partial-save failure; restore BOTH preimages after process exit: ")+Backup);
            if(!APSCanonicalPixelSlopeUpdate::VerifyPatched(Master,Slope,Error))return Refuse(Error+TEXT("; restore BOTH preimages: ")+Backup);
        }
        const FString NewMaster=Hash(MasterFile),NewSlope=Hash(SlopeFile);
        if(NewMaster.IsEmpty() || NewSlope.IsEmpty() || NewMaster==OriginalMaster || NewSlope==OriginalSlope || Hash(TemplateFile)!=TemplateHash)
            return Refuse(TEXT("Post-save integrity failure; restore BOTH preimages: ")+Backup);
        UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope.Publish] SAVED masterSHA1=%s slopeSHA1=%s backup=%s defaults=0/0; only far pixel slope adapters optional, NativeVI/WAT/UV/palette/offsets unchanged; cold/rendered proof REQUIRED"),*NewMaster,*NewSlope,*Backup);
        return true;
    }
}
#endif
