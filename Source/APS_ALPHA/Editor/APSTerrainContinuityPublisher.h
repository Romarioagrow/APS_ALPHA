#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainLodABBuilder.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"

// Copy the exact rendered candidate into an immutable release location. This
// command never changes a selector, existing Shared package or material graph.
namespace APSTerrainContinuityPublisher
{
    inline bool Build(IAssetTools& Tools)
    {
        const auto Refuse=[](const FString& Error)
        { UE_LOG(LogTemp,Error,TEXT("[APS.ContinuityPublish] Refused: %s"),*Error); return false; };
        if(!IsRunningCommandlet())return Refuse(TEXT("Offline commandlet only"));
        const FString SourceRoot=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuityCombined20260929V1/");
        const TCHAR* Destination=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1");
        struct FAsset{const TCHAR* Source;const TCHAR* Output;const TCHAR* SHA1;};
        const FAsset Assets[]={
            {TEXT("MF_APS_NormalMacroCoordinates"),TEXT("MF_APS_ContinuousNormalCoordinates"),TEXT("E5A466D571C3FBB52B6871E0352ED6FB1A685285")},
            {TEXT("M_APS_NormalWarpTerrain"),TEXT("M_APS_ContinuousTerrain"),TEXT("537341BE0160BF155C3DFDF898A743429E0E11ED")},
            {TEXT("MI_APS_NormalWarpTerra"),TEXT("MI_APS_ContinuousTerra"),TEXT("48D9E89E9C4FD85657B6329B7E86A011A0FBA887")}};
        const auto Hash=[](const FString& Package)
        {
            TArray<uint8> Bytes;
            const FString File=FPackageName::LongPackageNameToFilename(Package,FPackageName::GetAssetPackageExtension());
            return FFileHelper::LoadFileToArray(Bytes,*File)?FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num()).ToString().ToUpper():FString();
        };
        TArray<UObject*> Sources;
        for(const auto& Asset:Assets)
        {
            if(Hash(SourceRoot+Asset.Source)!=Asset.SHA1)return Refuse(TEXT("Rendered source hash differs: ")+SourceRoot+Asset.Source);
            if(FPackageName::DoesPackageExist(FString(Destination)/Asset.Output))return Refuse(TEXT("Release exists; refusing overwrite"));
            auto* Source=LoadObject<UObject>(nullptr,*(SourceRoot+Asset.Source+TEXT(".")+Asset.Source));
            if(!Source || Source->GetOutermost()->IsDirty())return Refuse(TEXT("Missing/dirty rendered source"));
            Sources.Add(Source);
        }
        auto* SourceFunction=Cast<UMaterialFunction>(Sources[0]);
        auto* SourceMaster=Cast<UMaterial>(Sources[1]);
        auto* SourceInstance=Cast<UMaterialInstanceConstant>(Sources[2]);
        if(!SourceFunction || !SourceMaster || !SourceInstance || SourceInstance->Parent!=SourceMaster)
            return Refuse(TEXT("Rendered hierarchy changed"));
        APSSharedTerrainMaterialBuilder::FBuild B(Tools,Destination);
        APSSharedTerrainNormalContinuity::TTransform<decltype(B)> Reader(B);
        auto* Function=Cast<UMaterialFunction>(B.Duplicate(SourceFunction,Assets[0].Output));
        auto* Master=Cast<UMaterial>(B.Duplicate(SourceMaster,Assets[1].Output));
        auto* Instance=Cast<UMaterialInstanceConstant>(B.Duplicate(SourceInstance,Assets[2].Output));
        if(!Function || !Master || !Instance)return Refuse(B.Error);
        UMaterialEditingLibrary::UpdateMaterialFunction(Function);
        int32 Calls=0;
        for(auto* E:Reader.Graph(Master))
        {
            Reader.Register(Master,E);
            if(auto* Call=Cast<UMaterialExpressionMaterialFunctionCall>(E);Call && Call->MaterialFunction==SourceFunction)
            {
                if(!B.ReconnectFunctionById(Call,SourceFunction,Function))return Refuse(B.Error);
                ++Calls;
            }
        }
        if(Calls!=5)return Refuse(TEXT("Expected five rendered normal calls"));
        for(UObject* Owner:{static_cast<UObject*>(Master),static_cast<UObject*>(Function)})
            for(auto* E:Reader.Graph(Owner))
                if(auto* Call=Cast<UMaterialExpressionMaterialFunctionCall>(E);Call && Call->MaterialFunction
                    && Call->MaterialFunction->GetPathName().Contains(TEXT("/Diagnostics/")))
                    return Refuse(TEXT("Release still references a diagnostic function"));
        Instance->SetParentEditorOnly(Master,false);
        Instance->CopyMaterialUniformParametersEditorOnly(SourceInstance,true);
        Instance->PostEditChange();Master->PostEditChange();
        APSSharedTerrainLodABBuilder::FBuilder PinRepair(Tools);
        if(!PinRepair.RestoreTransientFunctionPins(Master))return Refuse(PinRepair.B.Error);
        for(UMaterialInterface* M:{static_cast<UMaterialInterface*>(Master),static_cast<UMaterialInterface*>(Instance)})
            M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        for(UMaterialInterface* M:{static_cast<UMaterialInterface*>(Master),static_cast<UMaterialInterface*>(Instance)})
        {
            auto* R=M->GetMaterialResource(GMaxRHIFeatureLevel);const auto* Map=R?R->GetGameThreadShaderMap():nullptr;
            if(!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return Refuse(TEXT("Release shader incomplete: ")+M->GetPathName());
        }
        for(const auto& Asset:Assets)if(Hash(SourceRoot+Asset.Source)!=Asset.SHA1)return Refuse(TEXT("Source changed during compile"));
        for(UObject* Output:B.Outputs)
        {
            UPackage* Package=Output->GetOutermost();Package->MarkPackageDirty();
            FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
            const FString File=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
            if(!UPackage::SavePackage(Package,Output,*File,Args))return Refuse(TEXT("Release save failed: ")+File);
            UE_LOG(LogTemp,Display,TEXT("[APS.ContinuityPublish] Saved %s SHA1=%s"),*File,*Hash(Package->GetName()));
        }
        UE_LOG(LogTemp,Display,TEXT("[APS.ContinuityPublish] Three versioned files saved. Runtime selection unchanged; loading/render verification still required."));
        return true;
    }
}
#endif
