#pragma once
#if WITH_EDITOR
#include "APSSharedLavaMaterialBuilder.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

// Explicit one-time offline rebake of the accepted SharedLiquid lava master.
// The old WAT and both MICs stay read-only. A new private WAT is saved first,
// then only Call17 is published in the existing master at its existing path.
namespace APSLavaAntiGridUpdate
{
    struct FExpected { const TCHAR* Name; const TCHAR* SHA1; };
    inline constexpr FExpected Expected[] = {
        {TEXT("M_APS_SharedLava"), TEXT("B090E9DC2372A03472C02872F331A9D6B88F949B")},
        {TEXT("MI_APS_SharedLavaNative"), TEXT("1F3F7FE8F5562477149A54E4F6B42EF31D155E51")},
        {TEXT("MI_APS_SharedLava"), TEXT("0DD970B6F29A029CCAE5130BC44E96B399C66AB8")},
        {TEXT("MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("92D0F30659A78568DC2510E4D3CC25FD17150C83")}
    };

    inline FString Filename(const TCHAR* Name)
    {
        return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
            FString(APSSharedLavaMaterialBuilder::Destination) / Name, FPackageName::GetAssetPackageExtension()));
    }

    inline FString Hash(const FString& File)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *File)) return FString();
        return FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().ToUpper();
    }

    inline bool Run(IAssetTools& Tools)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        FCore Core(Tools, APSSharedLavaMaterialBuilder::Destination);
        const auto Refuse = [&](const FString& Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.LavaAntiGridUpdate] Refused: %s %s"), *Why, *Core.Error);
            return false;
        };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline protected commandlet only"));
        const FString PrivatePackage = FString(APSSharedLavaMaterialBuilder::Destination) / APSLavaAntiGrid::PrivateName;
        const FString PrivatePath = PrivatePackage + TEXT(".") + APSLavaAntiGrid::PrivateName;
        if (FPackageName::DoesPackageExist(PrivatePackage) || FindObject<UObject>(nullptr, *PrivatePath))
            return Refuse(TEXT("Private output already exists; inspect prior rebake rather than overwrite it"));

        TArray<UObject*> Originals;
        for (const FExpected& E : Expected)
        {
            const FString File = Filename(E.Name);
            if (Hash(File) != E.SHA1) return Refuse(TEXT("Exact accepted baseline changed: ") + File);
            if (IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("uexp")))
                || IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("ubulk"))))
                return Refuse(TEXT("Unexpected package sidecar: ") + File);
            const FString Path = FString(APSSharedLavaMaterialBuilder::Destination) / E.Name + TEXT(".") + E.Name;
            UObject* Object = LoadObject<UObject>(nullptr, *Path);
            if (!Object || Object->GetOutermost()->IsDirty())
                return Refuse(TEXT("Missing or unsaved shared object: ") + Path);
            Originals.Add(Object);
        }
        auto* Master = Cast<UMaterial>(Originals[0]);
        auto* Native = Cast<UMaterialInstanceConstant>(Originals[1]);
        auto* APS = Cast<UMaterialInstanceConstant>(Originals[2]);
        auto* OriginalWAT = Cast<UMaterialFunction>(Originals[3]);
        if (!Master || !Native || !APS || !OriginalWAT
            || Native->Parent.Get() != Master || APS->Parent.Get() != Native
            || Master->GetBlendMode() != BLEND_Masked || FCore::Expressions(Master).Num() != 115)
            return Refuse(TEXT("Exact shared lava graph/instance chain changed"));

        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedLavaAntiGridBackup_v1"));
        if (IFileManager::Get().DirectoryExists(*Backup))
            return Refuse(TEXT("Backup exists; inspect previous attempt: ") + Backup);
        if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
        for (const FExpected& E : Expected)
        {
            const FString Source = Filename(E.Name), Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
            if (Hash(Source) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Source, false, false) != COPY_OK
                || Hash(Dest) != E.SHA1)
                return Refuse(TEXT("Backup failed before graph changes: ") + Source);
        }

        if (!APSLavaAntiGrid::Apply(Core, Master)) return Refuse(TEXT("Private macro anti-grid adaptation"));
        if (Core.Outputs.Num() != 1 || Core.Outputs[0]->GetPathName() != PrivatePath
            || !Cast<UMaterialFunction>(Core.Outputs[0]) || Core.Outputs[0] == OriginalWAT
            || FCore::Expressions(Master).Num() != 115)
            return Refuse(TEXT("Transform escaped the new private-function write set"));
        Master->PostEditChange();
        UMaterialInterface* Materials[] = {Master, Native, APS};
        for (UMaterialInterface* Material : Materials)
            Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (UMaterialInterface* Material : Materials)
        {
            FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
            const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            {
                if (Resource) for (const FString& Error : Resource->GetCompileErrors())
                    UE_LOG(LogTemp, Error, TEXT("[APS.LavaAntiGridUpdate] %s"), *Error);
                return Refuse(TEXT("Shader/LocalVF incomplete; no asset saved: ") + Material->GetPathName());
            }
        }
        for (const FExpected& E : Expected)
            if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent file edit: ") + Filename(E.Name));
        if (Native->Parent.Get() != Master || APS->Parent.Get() != Native)
            return Refuse(TEXT("Instance parents changed"));
        // Another writer must not turn our new private package into an overwrite.
        if (FPackageName::DoesPackageExist(PrivatePackage))
            return Refuse(TEXT("Private package appeared during compilation"));

        const auto Save = [](UObject* Object, const FString& File)
        {
            UPackage* Package = Object->GetOutermost();
            Package->MarkPackageDirty();
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            return UPackage::SavePackage(Package, Object, *File, Args);
        };
        if (!Save(Core.Outputs[0], Filename(APSLavaAntiGrid::PrivateName)))
            return Refuse(TEXT("Private save failed; existing master not saved. Backup: ") + Backup);
        if (Hash(Filename(Expected[0].Name)) != Expected[0].SHA1)
            return Refuse(TEXT("Master changed before publish; new private output is unbound"));
        if (!Save(Master, Filename(Expected[0].Name)))
            return Refuse(TEXT("Master save failed; verified original available in ") + Backup);
        for (int32 I = 1; I < UE_ARRAY_COUNT(Expected); ++I)
            if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1)
                return Refuse(TEXT("Protected old WAT/MIC changed: ") + Filename(Expected[I].Name));
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaAntiGridUpdate] Saved new private WAT and existing master only; MICs/oldWAT unchanged; same orbit/gameplay material. Backup=%s. Rendered acceptance still required."), *Backup);
        return true;
    }
}
#endif
