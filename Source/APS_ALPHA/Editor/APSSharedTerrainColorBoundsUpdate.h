#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainLodABBuilder.h"
#include "APSSharedTerrainDetailPrecisionUpdate.h"
#include "APSSharedTerrainColorBounds.h"

// One-time protected saved-asset update. Only the master is saved. Both family
// MICs, the precise coordinate function and all other shared functions stay byte-identical.
namespace APSSharedTerrainColorBoundsUpdate
{
    using FExpected = APSSharedTerrainDetailPrecisionUpdate::FExpected;
    inline constexpr FExpected Expected[] = {
        {TEXT("M_APS_SharedWorldScapeTerrain"), TEXT("CB0E7DF1BB20A94964EF2AFCA560E0870AC28646")},
        {TEXT("MF_APS_MF_MacroVariationBlock_50169517"), TEXT("3FD8F36F4DC8A117BFFD9A8E1978CC8126137BBC")},
        {TEXT("MF_APS_MF_PlanetMap_80d6e0bd"), TEXT("2957DE0FCC842614B088FCBBD30A7F3F887B2B3B")},
        {TEXT("MF_APS_MF_SlopeBlock_f6be5407"), TEXT("3FF327494FFB83C3521A5BA1318FA913C3303AC2")},
        {TEXT("MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("2DF40DDCAFA3BEB9C1AB3CE3DFFB489F29A878CF")},
        {TEXT("MI_APS_SharedMagma"), TEXT("A198333D905DA52AA96FFFC3EBF988690062AAB5")},
        {TEXT("MI_APS_SharedTerra"), TEXT("D05DBA6349ED39245B7BE1A0937F8225DBC84598")}
    };

    inline bool Run(IAssetTools& Tools)
    {
        using APSSharedTerrainDetailPrecisionUpdate::Filename;
        using APSSharedTerrainDetailPrecisionUpdate::Hash;
        const auto Refuse = [](const FString& Why)
        { UE_LOG(LogTemp, Error, TEXT("[APS.TerrainColorBounds] Refused: %s"), *Why); return false; };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline commandlet only"));
        for (const auto& E : Expected)
            if (Hash(Filename(E)) != E.SHA1) return Refuse(TEXT("Accepted source changed: ") + Filename(E));
        const FString Root(APSSharedTerrainMaterialBuilder::OutputRoot);
        auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedWorldScapeTerrain.M_APS_SharedWorldScapeTerrain")));
        auto* Terra = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedTerra.MI_APS_SharedTerra")));
        auto* Magma = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedMagma.MI_APS_SharedMagma")));
        if (!Master || !Terra || !Magma || Master->GetOutermost()->IsDirty()
            || Terra->Parent != Master || Magma->Parent != Master || Master->bTangentSpaceNormal)
            return Refuse(TEXT("Shared source/parent/unsaved contract changed"));
        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()
            / TEXT("SharedTerrainColorBoundsBackup_20260927"));
        if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup already exists"));
        if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
        for (const auto& E : Expected)
        {
            const FString Source = Filename(E), Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
            if (Hash(Source) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Source, false, false) != COPY_OK
                || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup verification failed before edit"));
        }
        APSSharedTerrainMaterialBuilder::FBuild Build(Tools);
        bool Changed = false;
        if (!APSSharedTerrainColorBounds::Apply(Build, Master, Changed) || !Changed)
            return Refuse(Build.Error.IsEmpty() ? TEXT("Expected one new bound") : Build.Error);
        // Assert idempotence and exact preserved-edge contracts before compilation.
        bool ChangedAgain = false;
        if (!APSSharedTerrainColorBounds::Apply(Build, Master, ChangedAgain) || ChangedAgain)
            return Refuse(TEXT("Color bound is not idempotent"));
        Master->PostEditChange();
        APSSharedTerrainLodABBuilder::FBuilder Reader(Tools);
        if (!Reader.RestoreTransientFunctionPins(Master)) return Refuse(Reader.B.Error);
        UMaterialInterface* Materials[] = {Master, Terra, Magma};
        for (auto* M : Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (auto* M : Materials)
        {
            FMaterialResource* Resource = M->GetMaterialResource(GMaxRHIFeatureLevel);
            const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            {
                if (Resource) for (const auto& Error : Resource->GetCompileErrors()) UE_LOG(LogTemp, Error, TEXT("%s"), *Error);
                return Refuse(TEXT("Shared shader/LocalVF not complete; no asset saved"));
            }
        }
        for (const auto& E : Expected)
            if (Hash(Filename(E)) != E.SHA1) return Refuse(TEXT("Concurrent source change"));
        UPackage* Package = Master->GetOutermost(); Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Master, *Filename(Expected[0]), Args))
            return Refuse(TEXT("Master save failed; verified backup retained"));
        for (int32 I = 1; I < UE_ARRAY_COUNT(Expected); ++I)
            if (Hash(Filename(Expected[I])) != Expected[I].SHA1) return Refuse(TEXT("Protected MIC/function changed"));
        UE_LOG(LogTemp, Display, TEXT("[APS.TerrainColorBounds] SAVED masterOnly=1 topAlpha=Clamp01 nodesAdded=1 newTextureSamples=0 paletteCoverageGeometryUnchanged=1 backup=%s renderedAcceptancePending=1"), *Backup);
        return true;
    }
}
#endif
