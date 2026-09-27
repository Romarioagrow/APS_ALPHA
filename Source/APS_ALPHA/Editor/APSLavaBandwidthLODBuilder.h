#pragma once
#if WITH_EDITOR
#include "APSSharedLavaMaterialBuilder.h"
#include "APSLavaBandwidthLOD.h"

namespace APSLavaBandwidthLODBuilder
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaBandwidthLOD");
    inline bool Build(IAssetTools& AssetTools)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        FCore Core(AssetTools, Destination);
        const auto Fail = [&Core](const TCHAR* Why)
        { UE_LOG(LogTemp, Error, TEXT("[APS.LavaBandwidthLOD] Refused: %s %s"), Why, *Core.Error); return false; };
        auto* Source = LoadObject<UMaterial>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/M_APS_SharedLava.M_APS_SharedLava"));
        auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedLavaNative.MI_APS_SharedLavaNative"));
        auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedLava.MI_APS_SharedLava"));
        if (!Source || !Native || !APS || Native->Parent.Get() != Source || APS->Parent.Get() != Native
            || FCore::Expressions(Source).Num() != 115 || Source->GetBlendMode() != BLEND_Masked)
            return Fail(TEXT("Audited shared Lava source/parent chain changed"));
        auto* Master = Cast<UMaterial>(Core.Duplicate(Source, TEXT("M_APS_LavaBandwidthLOD")));
        if (!Master || !APSLavaBandwidthLOD::Apply(Core, Master)) return Fail(TEXT("20km-only bandwidth adaptation"));
        auto* NativeCopy = Cast<UMaterialInstanceConstant>(Core.Duplicate(Native, TEXT("MI_APS_LavaBandwidthLODNative")));
        auto* APSCopy = Cast<UMaterialInstanceConstant>(Core.Duplicate(APS, TEXT("MI_APS_LavaBandwidthLOD")));
        if (!NativeCopy || !APSCopy) return Fail(TEXT("Duplicate exact authored instance chain"));
        NativeCopy->SetParentEditorOnly(Master, false); APSCopy->SetParentEditorOnly(NativeCopy, false);
        Master->PostEditChange(); NativeCopy->PostEditChange(); APSCopy->PostEditChange();
        if (!APSSharedLavaMaterialBuilder::SameTypeState(Native, NativeCopy)
            || !APSSharedLavaMaterialBuilder::SameTypeState(APS, APSCopy)
            || APSCopy->GetMaterial() != Master || Core.Outputs.Num() != 4 || FCore::Expressions(Master).Num() != 115)
            return Fail(TEXT("Authored master/parameter state changed beyond private function"));
        for (UObject* Output : Core.Outputs)
            if (auto* Material = Cast<UMaterialInterface>(Output))
                Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        // Fail closed before saving any asset, including stage-illegal DDX/DY.
        for (UObject* Output : Core.Outputs)
        {
            auto* Material = Cast<UMaterialInterface>(Output);
            if (!Material) continue;
            FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
            FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            const bool Ready = Resource && Resource->IsGameThreadShaderMapComplete()
                && Resource->GetCompileErrors().Num() == 0 && Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
            UE_LOG(LogTemp, Display, TEXT("[APS.LavaBandwidthLOD] Ready=%d path=%s"), Ready, *Material->GetPathName());
            if (!Ready)
            {
                if (Resource) for (const FString& Error : Resource->GetCompileErrors()) UE_LOG(LogTemp, Error, TEXT("[APS.LavaBandwidthLOD] %s"), *Error);
                return Fail(TEXT("Incomplete shader map/compile errors/missing LocalVF"));
            }
        }
        for (UObject* Output : Core.Outputs)
        {
            UPackage* Package = Output->GetOutermost();
            if (!Package->GetName().StartsWith(FString(Destination) + TEXT("/"))) return Fail(TEXT("Output escaped candidate folder"));
            Package->MarkPackageDirty();
            const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, Output, *File, Args)) return Fail(TEXT("Save new owned candidate asset"));
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaBandwidthLOD] Saved4 new assets; masterExpressions=115 nativeSampler=unchanged noDebugGates=1 paletteBrightnessCoverage=unchanged productionSelected=0"));
        return true;
    }
}
#endif
