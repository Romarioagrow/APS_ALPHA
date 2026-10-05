#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainLodABBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSTerrainContinuityMaterial.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "Misc/App.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"

// One owned master update: restore the original five colour-volume samples.
// No function, normal-hex graph, texture, MIC, diagnostic or lava asset is saved.
namespace APSContinuousOriginalColorUpdate
{
    using FBuild = APSSharedTerrainMaterialBuilder::FBuild;
    inline constexpr const TCHAR* OriginalMasterSHA1 = TEXT("0CDF09601BCCB751D2FE3A1CA3B28F980829F0C3");
    inline constexpr const TCHAR* Description = TEXT("APS orbital color field diagnostic v3; native Noise1 and near");

    inline FString HashPackage(const FString& Package)
    {
        TArray<uint8> Bytes;
        const FString File = FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension());
        return FFileHelper::LoadFileToArray(Bytes, *File)
            ? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().ToUpper() : FString();
    }

    inline bool Patch(FBuild& B, UMaterial* Master)
    {
        const auto Fail = [&](const FString& Why) { B.Error = Why; return false; };
        if (!IsRunningCommandlet() || !IsInGameThread() || !IsValid(Master)
            || Master->GetPathName() != APSTerrainContinuityMaterial::MasterPath || !B.Error.IsEmpty())
            return Fail(TEXT("Original colour patch requires the exact owned Continuous master in an offline commandlet"));
        APSSharedTerrainNormalContinuity::TTransform<FBuild> Reader(B);
        const auto Graph = Reader.Graph(Master);
        FString ExpectedCode = APSOrbitalColorFieldsAB::Shader;
        if (ExpectedCode.ReplaceInline(TEXT("if(w<=0.0) return Legacy;"),
            TEXT("w*=saturate(FieldMode);\nif(w<=0.0) return Legacy;"), ESearchCase::CaseSensitive) != 1)
            return Fail(TEXT("Original combined colour shader anchor changed"));
        TMap<UMaterialExpressionCustom*, UMaterialExpressionTextureSampleParameterVolume*> Replacements;
        int32 Noise2 = 0, Noise3 = 0, Noise4 = 0;
        for (UMaterialExpression* E : Graph)
        {
            auto* Filter = Cast<UMaterialExpressionCustom>(E);
            if (!Filter || Filter->Description != Description) continue;
            if (!Filter->IsIn(Master) || Filter->OutputType != CMOT_Float3
                || Filter->Code != ExpectedCode || Filter->Inputs.Num() != 8)
                return Fail(TEXT("Audited colour filter shape/Code changed"));
            const TCHAR* Names[] = {TEXT("Legacy"), TEXT("P"), TEXT("Mean"), TEXT("StdDev"),
                TEXT("Salt"), TEXT("CameraDelta"), TEXT("Scale"), TEXT("FieldMode")};
            for (int32 I = 0; I < 8; ++I)
                if (Filter->Inputs[I].InputName != Names[I] || !Filter->Inputs[I].Input.Expression)
                    return Fail(TEXT("Audited colour filter inputs changed"));
            const FExpressionInput& Legacy = Filter->Inputs[0].Input;
            auto* Sample = Cast<UMaterialExpressionTextureSampleParameterVolume>(Legacy.Expression);
            const auto* Mode = Cast<UMaterialExpressionScalarParameter>(Filter->Inputs[7].Input.Expression);
            if (!Sample || !Sample->IsIn(Master) || !Sample->Coordinates.Expression
                || Legacy.OutputIndex || Legacy.Mask || Legacy.MaskR || Legacy.MaskG || Legacy.MaskB || Legacy.MaskA
                || !Mode || Mode->ParameterName != TEXT("APS_NormalMacroWarpMode"))
                return Fail(TEXT("Audited original colour sample/normal-mode link changed"));
            if (Sample->ParameterName == TEXT("Noise2")) ++Noise2;
            else if (Sample->ParameterName == TEXT("Noise3")) ++Noise3;
            else if (Sample->ParameterName == TEXT("Noise4")) ++Noise4;
            else return Fail(TEXT("Unexpected original colour sample; Noise1/warp must stay untouched"));
            Replacements.Add(Filter, Sample);
        }
        struct FLink { FExpressionInput* Input; UMaterialExpression* Filter; UMaterialExpression* Original; };
        TArray<FLink> Plan;
        TSet<UMaterialExpressionCustom*> Consumed;
        for (UMaterialExpression* Consumer : Graph)
            for (FExpressionInput* Input : Consumer->GetInputsView())
            {
                if (!Input) continue;
                auto* Filter = Cast<UMaterialExpressionCustom>(Input->Expression);
                auto* const* Original = Replacements.Find(Filter);
                if (!Original) continue;
                if (!Consumer->IsIn(Master) || Input->OutputIndex != 0 || Input->MaskA)
                    return Fail(TEXT("Audited colour consumer output/mask/ownership changed"));
                Plan.Add({Input, Filter, *Original}); Consumed.Add(Filter);
            }
        if (Replacements.Num() != 5 || Consumed.Num() != 5 || Plan.Num() != 9
            || Noise2 != 3 || Noise3 != 1 || Noise4 != 1)
            return Fail(FString::Printf(TEXT("Expected exact5filters/9links and Noise2x3/Noise3/Noise4; got %d/%d consumed=%d counts=%d/%d/%d"),
                Replacements.Num(), Plan.Num(), Consumed.Num(), Noise2, Noise3, Noise4));
        // All guards precede the first persistent graph write. Keep each complete
        // input's output index and channel masks; replace only its expression.
        for (const FLink& Link : Plan) Link.Input->Expression = Link.Original;
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor] Restored5native colour samples/9consumer links; Noise1, warp, normal-hex, masks and parameters untouched"));
        return true;
    }

    inline bool ShaderReady(UMaterialInterface* Material, FString& Error)
    {
        FMaterialResource* R = Material ? Material->GetMaterialResource(GMaxRHIFeatureLevel) : nullptr;
        const FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        if (R && R->GetCompileErrors().Num())
            for (const FString& Message : R->GetCompileErrors()) Error += TEXT("\n") + Message;
        if (!R || R->GetCompileErrors().Num() || !R->IsGameThreadShaderMapComplete()
            || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
        { Error = TEXT("Shader incomplete: ") + GetPathNameSafe(Material) + Error; return false; }
        return true;
    }

    // Forensic readback of the failed saved master, not a repair or a bake.
    // Normal MIC PostLoad may start ordinary async compilation; this function
    // never submits, finishes, recompiles, hydrates or saves material resources.
    inline bool ColdCheck()
    {
        constexpr const TCHAR* FailedSHA1 = TEXT("9541C8FB506D3F95E277D97C2FB1E7D626A420B0");
        const auto Refuse = [](const FString& Why)
        { UE_LOG(LogTemp, Error, TEXT("[APS.ContinuousOriginalColor.Cold] Refused: %s"), *Why); return false; };
        if (!IsRunningCommandlet() || !IsInGameThread() || !FApp::CanEverRender())
            return Refuse(TEXT("Rendering-enabled offline commandlet required; no NullRHI"));
        const FString Package = FPackageName::ObjectPathToPackageName(FString(APSTerrainContinuityMaterial::MasterPath));
        const FString TemplatePackage = FPackageName::ObjectPathToPackageName(FString(APSTerrainContinuityMaterial::TemplatePath));
        const FString TemplateHash = HashPackage(TemplatePackage);
        if (HashPackage(Package) != FailedSHA1 || TemplateHash.IsEmpty())
            return Refuse(TEXT("Requires exact failed master SHA1 9541C8FB506D3F95E277D97C2FB1E7D626A420B0 and existing MIC"));
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor.Cold] BEGIN MIC-first load; preloadedMaster=%d preloadedMIC=%d feature=%d remainingJobs=%d; normal PostLoad only, no shader-job submission/finish/save"),
            FindObject<UMaterial>(nullptr, APSTerrainContinuityMaterial::MasterPath) != nullptr,
            FindObject<UMaterialInstanceConstant>(nullptr, APSTerrainContinuityMaterial::TemplatePath) != nullptr,
            int32(GMaxRHIFeatureLevel), GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : -1);
        auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, APSTerrainContinuityMaterial::TemplatePath);
        // Do not preload the master/functions: inspect the actual parent that
        // normal MIC-first loading resolved, including its usual PostLoad order.
        auto* Master = Template ? Cast<UMaterial>(Template->Parent.Get()) : nullptr;
        if (!Master || Master->GetPathName() != APSTerrainContinuityMaterial::MasterPath)
            return Refuse(TEXT("MIC did not resolve the exact Continuous master parent"));
        const bool MasterDirty = Master->GetOutermost()->IsDirty();
        const bool TemplateDirty = Template->GetOutermost()->IsDirty();
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor.Cold] parent=%s StateId=%s EOD=%s expressions=%d dirtyMaster=%d dirtyMIC=%d"),
            *Master->GetPathName(), *Master->StateId.ToString(), *GetPathNameSafe(Master->GetEditorOnlyData()),
            Master->GetExpressions().Num(), MasterDirty, TemplateDirty);
        bool AllReady = true;
        TArray<const FMaterialRenderProxy*> Proxies;
        TArray<FString> Paths;
        for (UMaterialInterface* Material : {static_cast<UMaterialInterface*>(Master), static_cast<UMaterialInterface*>(Template)})
        {
            const FString Path = Material->GetPathName();
            FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
            const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            const bool GTReady = Resource && Resource->IsGameThreadShaderMapComplete()
                && Resource->GetCompileErrors().IsEmpty() && Map
                && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor.Cold] GT material=%s resource=%p feature=%d quality=%d map=%p complete=%d LocalVF=%d errors=%d compilationFinished=%d remainingJobs=%d"),
                *Path, Resource, Resource ? int32(Resource->GetFeatureLevel()) : -1,
                Resource ? int32(Resource->GetQualityLevel()) : -1, Map,
                Resource && Resource->IsGameThreadShaderMapComplete(),
                Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType),
                Resource ? Resource->GetCompileErrors().Num() : -1,
                Resource && Resource->IsCompilationFinished(),
                GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : -1);
            if (Resource)
                for (const FString& Error : Resource->GetCompileErrors())
                    UE_LOG(LogTemp, Error, TEXT("[APS.ContinuousOriginalColor.Cold] CompileError material=%s %s"), *Path, *Error);
            Proxies.Add(Material->GetRenderProxy());
            Paths.Add(Path);
            AllReady &= GTReady;
        }
        const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel;
        bool AllRTReady = true;
        ENQUEUE_RENDER_COMMAND(APSContinuousOriginalColorColdReadback)(
            [Proxies, Paths, FeatureLevel, &AllRTReady](FRHICommandListImmediate&)
            {
                for (int32 Index = 0; Index < Proxies.Num(); ++Index)
                {
                    const FString& Path = Paths[Index];
                    bool RTReady = false;
                    // GetMaterialWithFallback would itself SubmitCompileJobs.
                    // Walk the same fallback selection without that side effect.
                    const FMaterialRenderProxy* Current = Proxies[Index];
                    TSet<const FMaterialRenderProxy*> Visited;
                    for (int32 Depth = 0; Current && Depth < 8 && !Visited.Contains(Current); ++Depth)
                    {
                        Visited.Add(Current);
                        const FMaterial* R = Current->GetMaterialNoFallback(FeatureLevel);
                        const FMaterialShaderMap* M = R ? R->GetRenderingThreadShaderMap() : nullptr;
                        const bool Complete = R && R->IsRenderingThreadShaderMapComplete();
                        const bool LocalVF = M && M->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
                        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor.Cold] RT material=%s depth=%d proxy=%p resource=%p resolved=%s feature=%d quality=%d map=%p complete=%d LocalVF=%d selected=%d fallback=%d"),
                            *Path, Depth, Current, R, R ? *R->GetFriendlyName() : TEXT("<null>"),
                            R ? int32(R->GetFeatureLevel()) : -1, R ? int32(R->GetQualityLevel()) : -1,
                            M, Complete, LocalVF, Complete, Depth > 0);
                        if (Complete)
                        {
                            RTReady = Depth == 0 && LocalVF;
                            break;
                        }
                        Current = Current->GetFallback(FeatureLevel);
                    }
                    AllRTReady &= RTReady;
                }
            });
        // Only drain render commands (including normal PostLoad resource
        // publication), never wait for or force shader compilation.
        FlushRenderingCommands();
        AllReady &= AllRTReady;
        if (HashPackage(Package) != FailedSHA1 || HashPackage(TemplatePackage) != TemplateHash
            || Master->GetOutermost()->IsDirty() != MasterDirty || Template->GetOutermost()->IsDirty() != TemplateDirty)
            return Refuse(TEXT("Master/MIC disk hash or post-load dirty state changed during readback"));
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor.Cold] END hashesUnchanged=1 dirtyStateUnchanged=1 resourcesReady=%d remainingJobs=%d; immediate cold snapshot only, not visual acceptance"),
            AllReady, GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : -1);
        return AllReady;
    }

    inline bool Update(IAssetTools& Tools)
    {
        const auto Refuse = [](const FString& Why)
        { UE_LOG(LogTemp, Error, TEXT("[APS.ContinuousOriginalColor] Refused: %s"), *Why); return false; };
        if (!IsRunningCommandlet() || !IsInGameThread()) return Refuse(TEXT("Offline commandlet only"));
        const FString Package = FPackageName::ObjectPathToPackageName(FString(APSTerrainContinuityMaterial::MasterPath));
        const FString TemplatePackage = FPackageName::ObjectPathToPackageName(FString(APSTerrainContinuityMaterial::TemplatePath));
        if (HashPackage(Package) != OriginalMasterSHA1) return Refuse(TEXT("Owned master differs from exact pre-update SHA1; repeated/unknown update refused"));
        const FString TemplateHash = HashPackage(TemplatePackage);
        auto* Master = LoadObject<UMaterial>(nullptr, APSTerrainContinuityMaterial::MasterPath);
        auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, APSTerrainContinuityMaterial::TemplatePath);
        if (!Master || !Template || TemplateHash.IsEmpty() || Template->Parent.Get() != Master
            || Master->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty())
            return Refuse(TEXT("Exact clean Continuous master/MIC required"));
        FBuild B(Tools, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1"));
        if (!Patch(B, Master)) return Refuse(B.Error);
        // Existing bounded recursive helper walks reachable (not only registered)
        // function calls and restores transient input/output pointers by GUID/name.
        // It does not alter collections, serialized edges or save any function.
        APSSharedTerrainLodABBuilder::FBuilder Pins(Tools);
        if (!Pins.RestoreTransientFunctionPins(Master)) return Refuse(Pins.B.Error);
        UMaterialEditingLibrary::RecompileMaterial(Master);
        for (UMaterialInterface* Material : {static_cast<UMaterialInterface*>(Master), static_cast<UMaterialInterface*>(Template)})
        {
            Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
            FString Error;
            if (!ShaderReady(Material, Error)) return Refuse(Error);
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor] shaderComplete=1 LocalVF=1 errors=0 material=%s"), *Material->GetPathName());
        }
        if (HashPackage(Package) != OriginalMasterSHA1 || HashPackage(TemplatePackage) != TemplateHash)
            return Refuse(TEXT("Master/MIC disk changed before the single owned save"));
        UPackage* Outer = Master->GetOutermost(); Outer->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        const FString File = FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Outer, Master, *File, Args)) return Refuse(TEXT("Owned master save failed"));
        const FString NewHash = HashPackage(Package);
        if (NewHash.IsEmpty() || NewHash == OriginalMasterSHA1 || HashPackage(TemplatePackage) != TemplateHash)
            return Refuse(TEXT("Post-save master/MIC hash verification failed"));
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalColor] Saved ONLY %s oldSHA1=%s newSHA1=%s; MIC/functions/diagnostics/lava not saved; cold-load/render acceptance remains separate"),
            *File, OriginalMasterSHA1, *NewHash);
        return true;
    }
}
#endif
