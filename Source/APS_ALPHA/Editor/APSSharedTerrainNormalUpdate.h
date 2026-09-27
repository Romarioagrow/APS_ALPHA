#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "HAL/FileManager.h"
#include "LocalVertexFactory.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

// Deliberate one-time update of THREE existing generated shared graph assets.
// No new parent/path or global runtime material selector. All seven baseline
// shared files are checked and backed up BEFORE the first in-memory mutation.
namespace APSSharedTerrainNormalUpdate
{
    struct FExpected { const TCHAR* Name; const TCHAR* SHA1; bool bWrite; };
    inline constexpr FExpected Expected[] = {
        {TEXT("M_APS_SharedWorldScapeTerrain"), TEXT("D2F4B93692BA2957C487162B54FE232E81A32F7B"), true},
        {TEXT("MF_APS_MF_MacroVariationBlock_50169517"), TEXT("3FD8F36F4DC8A117BFFD9A8E1978CC8126137BBC"), false},
        {TEXT("MF_APS_MF_PlanetMap_80d6e0bd"), TEXT("2957DE0FCC842614B088FCBBD30A7F3F887B2B3B"), false},
        {TEXT("MF_APS_MF_SlopeBlock_f6be5407"), TEXT("2BF9FDA11CECD5DE8C523CF2EEB01E517CD2978D"), true},
        {TEXT("MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("02E831AF6973EA181E9AD5AD55B342824650D30E"), true},
        {TEXT("MI_APS_SharedMagma"), TEXT("A198333D905DA52AA96FFFC3EBF988690062AAB5"), false},
        {TEXT("MI_APS_SharedTerra"), TEXT("D05DBA6349ED39245B7BE1A0937F8225DBC84598"), false}
    };
    inline FString Filename(const FExpected& E)
    {
        return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
            FString(APSSharedTerrainMaterialBuilder::OutputRoot) / E.Name, FPackageName::GetAssetPackageExtension()));
    }
    inline FString Hash(const FString& File)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *File)) return FString();
        return FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().ToUpper();
    }

    inline bool Run(IAssetTools& Tools)
    {
        APSSharedTerrainMaterialBuilder::FBuild B(Tools);
        const auto Refuse = [&](const FString& Error)
        { UE_LOG(LogTemp, Error, TEXT("[APS.TerrainNormalUpdate] Refused: %s"), *Error); return false; };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline protected commandlet only"));
        TArray<UObject*> Objects;
        for (const FExpected& E : Expected)
        {
            const FString File = Filename(E);
            if (Hash(File) != E.SHA1) return Refuse(TEXT("Exact baseline changed: ") + File);
            // These are uncooked editor packages. Unexpected companions need a
            // revised manifest, not a partial package backup.
            if (IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("uexp")))
                || IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("ubulk"))))
                return Refuse(TEXT("Unexpected package sidecar: ") + File);
            const FString Path = FString(APSSharedTerrainMaterialBuilder::OutputRoot) / E.Name + TEXT(".") + E.Name;
            UObject* O = LoadObject<UObject>(nullptr, *Path);
            if (!O || O->GetOutermost()->IsDirty()) return Refuse(TEXT("Missing or unsaved shared object: ") + Path);
            Objects.Add(O);
        }
        auto* Master = Cast<UMaterial>(Objects[0]);
        auto* Magma = Cast<UMaterialInstanceConstant>(Objects[5]);
        auto* Terra = Cast<UMaterialInstanceConstant>(Objects[6]);
        if (!Master || !Magma || !Terra || Magma->Parent != Master || Terra->Parent != Master
            || Master->bTangentSpaceNormal || Master->GetBlendMode() != BLEND_Masked)
            return Refuse(TEXT("Exact shared master/templates contract changed"));

        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedTerrainNormalContinuityBackup"));
        if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Backup exists; inspect previous attempt: ") + Backup);
        if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Could not create backup directory"));
        for (const FExpected& E : Expected)
        {
            const FString Source = Filename(E), Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
            if (Hash(Source) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Source, false, false) != COPY_OK
                || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup verification failed; no graph modified: ") + Source);
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.TerrainNormalUpdate] Seven exact original packages backed up and verified: %s"), *Backup);

        APSSharedTerrainNormalContinuity::TTransform<APSSharedTerrainMaterialBuilder::FBuild> Transform(B);
        if (!Transform.Apply(Master)) return Refuse(B.Error);
        for (UObject* O : Transform.Changed)
        {
            bool Allowed = false;
            for (int32 I = 0; I < UE_ARRAY_COUNT(Expected); ++I) Allowed |= Expected[I].bWrite && O == Objects[I];
            if (!Allowed) return Refuse(TEXT("Transform escaped exact three-object write set"));
        }
        Master->PostEditChange();
        UMaterialInterface* Materials[] = {Master, Magma, Terra};
        for (UMaterialInterface* M : Materials)
            M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (UMaterialInterface* M : Materials)
        {
            FMaterialResource* R = M->GetMaterialResource(GMaxRHIFeatureLevel);
            const FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
            if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return Refuse(TEXT("Shader/LocalVF incomplete; no packages saved: ") + M->GetPathName());
        }
        // A concurrent asset edit during compilation must never be overwritten.
        for (const FExpected& E : Expected)
            if (Hash(Filename(E)) != E.SHA1) return Refuse(TEXT("File changed while compiling: ") + Filename(E));
        if (Magma->Parent != Master || Terra->Parent != Master) return Refuse(TEXT("MIC parents changed"));

        for (int32 I = 0; I < UE_ARRAY_COUNT(Expected); ++I)
        {
            const FExpected& E = Expected[I];
            if (!E.bWrite) continue;
            UObject* O = Objects[I]; UPackage* P = O->GetOutermost(); P->MarkPackageDirty();
            FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(P, O, *Filename(E), Args))
                return Refuse(TEXT("Partial save failure; verified originals available in ") + Backup);
        }
        for (const FExpected& E : Expected)
            if (!E.bWrite && Hash(Filename(E)) != E.SHA1)
                return Refuse(TEXT("Protected untouched asset changed: ") + Filename(E));
        UE_LOG(LogTemp, Display, TEXT("[APS.TerrainNormalUpdate] Saved existing master + slope + WAT only. MICs/palette/static overrides and other functions byte-identical; parent paths unchanged. Backup=%s. Rendered near/far and family acceptance still required."), *Backup);
        return true;
    }
}
#endif
