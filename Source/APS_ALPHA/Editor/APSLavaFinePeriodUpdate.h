#pragma once
#if WITH_EDITOR
#include "APSLavaCrustReflectanceUpdate.h"
#include "Misc/CommandLine.h"

// Offline parameterization for an opt-in physical-footprint comparison. The
// saved default remains 400 m, thermal coverage remains disabled. Not promotion.
namespace APSLavaFinePeriodUpdate
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
using APSLavaAntiGridUpdate::Filename;
using APSLavaAntiGridUpdate::Hash;
struct FExpected { const TCHAR* Name; const TCHAR* SHA1; };
inline constexpr FExpected Expected[] = {
    {TEXT("M_APS_SharedLava"), TEXT("1F57724D85B9BED0BF1B34C05410D6EA515AB6C3")},
    {TEXT("MI_APS_SharedLavaNative"), TEXT("1F3F7FE8F5562477149A54E4F6B42EF31D155E51")},
    {TEXT("MI_APS_SharedLava"), TEXT("9E5D5EF9183A800DA87D88F42B4CE47ECFDE6342")},
    {TEXT("MF_APS_LavaWAT20kmAntiGrid_v1"), TEXT("27640676C83642BF2A50C432F1D605C2025FA306")},
    {TEXT("MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("41C18297D0016483B5C7D1FC04F5C4B16EC060F1")},
    {TEXT("T_APS_LavaThermal400m_RGBA_v1"), TEXT("D1F19514948971DE6BAFDF2836EAC16C099FE2D8")}
};
inline bool Run(IAssetTools& Tools)
{
    FCore B(Tools, APSSharedLavaMaterialBuilder::Destination);
    const auto Refuse = [](const FString& Why) {
        UE_LOG(LogTemp, Error, TEXT("[APS.LavaFinePeriod] Refused: %s"), *Why); return false;
    };
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    for (const auto& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(FString(TEXT("Asset drift: ")) + E.Name);
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty()
        || Native->Parent.Get() != Master || APS->Parent.Get() != Native || FCore::Expressions(Master).Num() != 126)
        return Refuse(TEXT("Unexpected candidate chain/graph"));
    auto* Fine = FindObject<UMaterialExpressionMaterialFunctionCall>(Master, TEXT("MaterialExpressionMaterialFunctionCall_14"));
    auto* Pin = Fine ? Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P)
        {return P.Input.InputName == TEXT("TextureSize");}) : nullptr;
    auto* Multiply = Pin ? Cast<UMaterialExpressionMultiply>(Pin->Input.Expression) : nullptr;
    auto* A = Multiply ? Cast<UMaterialExpressionConstant>(Multiply->A.Expression) : nullptr;
    auto* C = Multiply ? Cast<UMaterialExpressionConstant>(Multiply->B.Expression) : nullptr;
    if (!Fine || !Fine->MaterialFunction || Fine->MaterialFunction->GetName() != TEXT("MF_APS_WorldAlignedTexture_a83aa78c")
        || !Multiply || Multiply->GetName() != TEXT("MaterialExpressionMultiply_18")
        || !A || !C || A->R != 400.0f || C->R != 100.0f || Pin->Input.OutputIndex != 0 || Pin->Input.Mask)
        return Refuse(TEXT("Audited 400m physical texture-size input changed"));
    const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedLavaFinePeriodBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup exists"));
    if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
    for (const auto& E : Expected)
    {
        const FString Dest = Backup / (FString(E.Name)+TEXT(".uasset"));
        if (Hash(Filename(E.Name)) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Filename(E.Name), false, false) != COPY_OK
            || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup failed"));
    }
    auto* Period = B.Add<UMaterialExpressionScalarParameter>(Master);
    Period->ParameterName = TEXT("APS_LavaFinePeriodCm"); Period->DefaultValue = 40000.0f;
    Period->SliderMin = 100.0f; Period->SliderMax = 40000.0f;
    Period->Group = TEXT("APS Lava Surface"); Period->UpdateParameterGuid(true, true);
    auto* Bound = B.Add<UMaterialExpressionClamp>(Master);
    Bound->Input.Connect(0, Period); Bound->MinDefault = 100.0f; Bound->MaxDefault = 40000.0f;
    Pin->Input.Connect(0, Bound);
    if (FCore::Expressions(Master).Num() != 128) return Refuse(TEXT("Unexpected graph size"));
    Master->PostEditChange();
    UMaterialInterface* Materials[] = {Master, Native, APS};
    for (auto* M : Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* M : Materials)
    {
        auto* R = M->GetMaterialResource(GMaxRHIFeatureLevel);
        const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
            || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Refuse(TEXT("Shader/LocalVF incomplete"));
    }
    float Coverage = -1, ActualPeriod = -1;
    if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaFinePeriodCm")), ActualPeriod)
        || ActualPeriod != 40000.0f || !APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaThermalCoverage")), Coverage)
        || Coverage != 0.0f) return Refuse(TEXT("Saved defaults must retain original appearance"));
    for (const auto& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent edit"));
    UPackage* Package = Master->GetOutermost(); Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Master, *Filename(Expected[0].Name), Args)) return Refuse(TEXT("Save failed"));
    for (int32 I=1; I<UE_ARRAY_COUNT(Expected); ++I)
        if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1) return Refuse(TEXT("Protected asset changed"));
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaFinePeriod] SAVED masterOnly=1 defaultPeriodCm=40000 defaultCoverage=0 newTextureReads=0 backup=%s renderedAcceptancePending=1"), *Backup);
    return true;
}
}
#endif
