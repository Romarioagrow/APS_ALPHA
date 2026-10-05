#pragma once

#if WITH_EDITOR
#include "APSOrbitalReliefLightingUpdate.h"
#include "APSCanonicalReliefChartUpdate.h"
#include "APSCanonicalCoverageUpdate.h"
#include "APSCanonicalPixelSlopePublisher.h"
#include "APSOriginalDistanceColorUpdate.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "APSContinuousOriginalColorUpdate.h"
#include "HAL/FileManager.h"

// Explicit offline operation only. It updates the existing Continuous master,
// never chooses a replacement material, changes runtime parameters or touches
// UnifiedLava/Theon. Each explicit branch guards its own narrow graph transform.
namespace APSOrbitalReliefLightingPublisher
{
    inline constexpr const TCHAR* ExpectedSHA1 = TEXT("9541C8FB506D3F95E277D97C2FB1E7D626A420B0");

    inline FString FileHash(const FString& File)
    {
        TArray<uint8> Bytes;
        return FFileHelper::LoadFileToArray(Bytes, *File)
            ? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().ToUpper() : FString();
    }

    inline bool Run(IAssetTools& Tools, bool bInspectOnly = false)
    {
        const auto Refuse = [](const FString& Why)
        { UE_LOG(LogTemp, Error, TEXT("[APS.OrbitalReliefLighting] Refused: %s"), *Why); return false; };
        if (!IsRunningCommandlet() || !IsInGameThread() || !FApp::CanEverRender())
            return Refuse(TEXT("Rendering-enabled offline commandlet only"));
        int32 DistanceColorFlags=0; FString ModeToken; const TCHAR* ModeCursor=FCommandLine::Get();
        while(FParse::Token(ModeCursor,ModeToken,false))
        {
            if(ModeToken.Equals(TEXT("-APSOriginalDistanceColor"),ESearchCase::IgnoreCase)) ++DistanceColorFlags;
            else if(ModeToken.StartsWith(TEXT("-APSOriginalDistanceColor="),ESearchCase::IgnoreCase))
                return Refuse(TEXT("OriginalDistanceColor is a bare flag; value-bearing form refused before any default patch"));
        }
        if(DistanceColorFlags>1) return Refuse(TEXT("Duplicate OriginalDistanceColor flag"));
        const bool bDistanceColor=DistanceColorFlags==1;
        const bool bSlope=FParse::Param(FCommandLine::Get(),TEXT("APSCanonicalSlope"));
        const bool bSlopeVerify=FParse::Param(FCommandLine::Get(),TEXT("APSCanonicalSlopeVerify"));
        if(bSlope || bSlopeVerify)
        {
            if((bSlope && bSlopeVerify) || bInspectOnly || bDistanceColor
                || FParse::Param(FCommandLine::Get(),TEXT("APSCanonicalReliefChart"))
                || FParse::Param(FCommandLine::Get(),TEXT("APSCanonicalCoverage")))
                return Refuse(TEXT("Pixel slope bake/cold verify must be a separate exclusive operation"));
            return APSCanonicalPixelSlopePublisher::Run(Tools,bSlopeVerify);
        }
        const FString Package = FPackageName::ObjectPathToPackageName(FString(APSTerrainContinuityMaterial::MasterPath));
        const FString TemplatePackage = FPackageName::ObjectPathToPackageName(FString(APSTerrainContinuityMaterial::TemplatePath));
        const FString File = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
            Package, FPackageName::GetAssetPackageExtension()));
        const FString TemplateFile = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
            TemplatePackage, FPackageName::GetAssetPackageExtension()));
        const FString TemplateHash = FileHash(TemplateFile);
        if (FileHash(File) != ExpectedSHA1 || TemplateHash.IsEmpty())
            return Refuse(TEXT("Exact original-colour master 9541 and existing MIC required; repeated/unknown patch refused"));
        for (const FString& Source : {File, TemplateFile})
            for (const TCHAR* Extension : {TEXT("uexp"), TEXT("ubulk")})
                if (IFileManager::Get().FileExists(*FPaths::ChangeExtension(Source, Extension)))
                    return Refuse(TEXT("Unexpected asset sidecar requires an explicit complete backup: ") + Source);
        auto* Master = LoadObject<UMaterial>(nullptr, APSTerrainContinuityMaterial::MasterPath);
        auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, APSTerrainContinuityMaterial::TemplatePath);
        if (!Master || !Template || Template->Parent.Get() != Master
            || Master->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty())
            return Refuse(TEXT("Exact clean master and unchanged saved MIC parent required"));

        const bool bCanonicalChart = FParse::Param(FCommandLine::Get(), TEXT("APSCanonicalReliefChart"));
        const bool bCanonicalCoverage = FParse::Param(FCommandLine::Get(), TEXT("APSCanonicalCoverage"));
        if ((bCanonicalChart && bCanonicalCoverage) || (bDistanceColor && (bCanonicalChart || bCanonicalCoverage)))
            return Refuse(TEXT("Choose one exclusive material experiment only"));
        FString Error;
        // Validate the live loaded graph even for a real patch, before creating
        // a backup directory or mutating any persistent expression link.
        if (!(bDistanceColor ? APSOriginalDistanceColorUpdate::Patch(Master,Error,false)
            : APSOrbitalReliefLightingUpdate::Patch(Master, Error, false))) return Refuse(Error);
        if (bInspectOnly)
        {
            if (FileHash(File) != ExpectedSHA1 || FileHash(TemplateFile) != TemplateHash
                || Master->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty())
                return Refuse(TEXT("Read-only inspection changed master/MIC state"));
            UE_LOG(LogTemp, Display, TEXT("[APS.OrbitalReliefLighting] inspectOnly=1 exactGraphEligible=1 noBackup=1 noPatch=1 noExplicitCompile=1 noSave=1; NOT visual acceptance"));
            return true;
        }

        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()
            / (bDistanceColor ? TEXT("OriginalDistanceColorBackup_9541") : bCanonicalCoverage ? TEXT("CanonicalCoverageBackup_9541") : bCanonicalChart ? TEXT("CanonicalReliefChartBackup_9541") : TEXT("OrbitalReliefLightingBackup_9541")));
        if (IFileManager::Get().DirectoryExists(*Backup))
            return Refuse(TEXT("Backup already exists; inspect prior attempt before retry: ") + Backup);
        if (!IFileManager::Get().MakeDirectory(*Backup, true))
            return Refuse(TEXT("Cannot create immutable pre-change backup"));
        for (const FString& Source : {File, TemplateFile})
        {
            const FString Dest = Backup / FPaths::GetCleanFilename(Source);
            const FString Expected = Source == File ? FString(ExpectedSHA1) : TemplateHash;
            if (FileHash(Source) != Expected
                || IFileManager::Get().Copy(*Dest, *Source, false, false) != COPY_OK
                || FileHash(Dest) != Expected)
                return Refuse(TEXT("Backup integrity failed before graph edit: ") + Source);
        }
        if (!(bDistanceColor ? APSOriginalDistanceColorUpdate::Patch(Master,Error) : bCanonicalCoverage ? APSCanonicalCoverageUpdate::Patch(Master, Error) : bCanonicalChart ? APSCanonicalReliefChartUpdate::Patch(Master, Error)
            : APSOrbitalReliefLightingUpdate::Patch(Master, Error))) return Refuse(Error);
        // Repair only transient GUID/pin bindings dropped by loaded function
        // graphs. No serialized function links or function packages are saved.
        APSSharedTerrainLodABBuilder::FBuilder Pins(Tools);
        if (!Pins.RestoreTransientFunctionPins(Master)) return Refuse(Pins.B.Error);
        UMaterialEditingLibrary::RecompileMaterial(Master);
        for (UMaterialInterface* Material : {static_cast<UMaterialInterface*>(Master),
            static_cast<UMaterialInterface*>(Template)})
        {
            Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
            if (!APSContinuousOriginalColorUpdate::ShaderReady(Material, Error)) return Refuse(Error);
        }
        if (Template->Parent.Get() != Master || FileHash(File) != ExpectedSHA1
            || FileHash(TemplateFile) != TemplateHash)
            return Refuse(TEXT("Master/MIC changed while compiling; refuse overwrite"));
        UPackage* Outer = Master->GetOutermost(); Outer->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Outer, Master, *File, Args))
            return Refuse(TEXT("Master save failed; verified recovery copy: ") + Backup);
        const FString NewHash = FileHash(File);
        if (NewHash.IsEmpty() || NewHash == ExpectedSHA1 || FileHash(TemplateFile) != TemplateHash)
            return Refuse(TEXT("Post-save integrity failed; verified recovery copy: ") + Backup);
        UE_LOG(LogTemp, Display, TEXT("[APS.OrbitalReliefLighting] existingMaster=%s oldSHA1=%s newSHA1=%s backup=%s; explicit isolated graph change; no authored palette/UV/function/liquid/geometry writes. Source->asset only, NOT rendered acceptance."),
            *File, ExpectedSHA1, *NewHash, *Backup);
        UE_LOG(LogTemp, Display, TEXT("[APS.CanonicalChart] offlinePublish=%d disabledDefault=1"), bCanonicalChart ? 1 : 0);
        UE_LOG(LogTemp, Display, TEXT("[APS.OriginalDistanceColor] offlinePublish=%d; one outer BaseColor distance-selector only, inner distance paths unchanged; near not bit-identical, rendered comparison required"), bDistanceColor ? 1 : 0);
        return true;
    }
}
#endif
