#pragma once
#if WITH_EDITOR
#include "APSLavaThermalCoverageUpdate.h"

// One-time revision of the DISABLED thermal trial after its six-frame A/B.
// v1 (.35) produced mostly-hot peppering. v2 tests larger cooled regions.
namespace APSLavaThermalCoverageRetune
{
inline bool Run(IAssetTools& Tools)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using APSLavaAntiGridUpdate::Filename;
    using APSLavaAntiGridUpdate::Hash;
    const auto Refuse = [](const FString& Why)
    {
        UE_LOG(LogTemp, Error, TEXT("[APS.LavaThermalRetune] Refused: %s"), *Why);
        return false;
    };
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    TArray<APSLavaThermalCoverageUpdate::FExpected> Expected;
    for (const auto& E : APSLavaThermalCoverageUpdate::Expected) Expected.Add(E);
    Expected[0].SHA1 = TEXT("1C72A8DE5B6DCFF3100F708B04B1FCC5E611BE18");
    for (const auto& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Saved baseline drift"));
    if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty()
        || Native->Parent.Get() != Master || APS->Parent.Get() != Native
        || FCore::Expressions(Master).Num() != 129)
        return Refuse(TEXT("Unexpected graph/chain"));
    float Strength = -1, Brightness = -1;
    FLinearColor Colour;
    if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaThermalCoverage")), Strength) || Strength != 0
        || !APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), Brightness) || Brightness != 1.5f
        || !APS->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveColor")), Colour)
        || !Colour.Equals(FLinearColor(.42f, .018f, .001f, 1), 1.e-6f))
        return Refuse(TEXT("Disabled trial/palette contract changed"));
    UMaterialExpressionCustom* Coverage = nullptr;
    for (UMaterialExpression* E : FCore::Expressions(Master))
        if (auto* C = Cast<UMaterialExpressionCustom>(E))
            if (C->Description == TEXT("APS filtered native lava thermal coverage v1"))
            {
                if (Coverage) return Refuse(TEXT("Ambiguous coverage node"));
                Coverage = C;
            }
    if (!Coverage || Coverage->OutputType != CMOT_Float1 || Coverage->Inputs.Num() != 3
        || !Coverage->Code.Contains(TEXT("max(0.07,")) || !Coverage->Code.Contains(TEXT("0.35 - width, 0.35 + width"))
        || !Coverage->Code.Contains(TEXT("lerp(resolved, 0.35, unresolved)")))
        return Refuse(TEXT("Expected v1 thermal arithmetic missing"));
    TMap<int32, FExpressionInput> Properties;
    for (int32 P = 0; P < MP_MAX; ++P)
        if (const auto* In = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P)))
            Properties.Add(P, *In);
    const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedLavaThermalRetuneBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup exists"));
    if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
    for (const auto& E : Expected)
    {
        const FString Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
        if (Hash(Filename(E.Name)) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Filename(E.Name), false, false) != COPY_OK
            || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup mismatch"));
    }
    Coverage->Code = Coverage->Code.Replace(TEXT("max(0.07,"), TEXT("max(0.10,"))
        .Replace(TEXT("0.35 - width, 0.35 + width"), TEXT("0.65 - width, 0.65 + width"))
        .Replace(TEXT("lerp(resolved, 0.35, unresolved)"), TEXT("lerp(resolved, 0.20, unresolved)"));
    Coverage->Description = TEXT("APS filtered native lava thermal coverage v2");
    for (const auto& Pair : Properties)
    {
        const auto* Now = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Pair.Key));
        if (!Now || !APSLavaThermalCoverageUpdate::SameInput(*Now, Pair.Value))
            return Refuse(TEXT("Material output changed"));
    }
    Master->PostEditChange();
    UMaterialInterface* Materials[] = {Master, Native, APS};
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
            return Refuse(TEXT("Shader incomplete; no save"));
    }
    for (const auto& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent saved edit"));
    UPackage* Package = Master->GetOutermost(); Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Master, *Filename(Expected[0].Name), Args)) return Refuse(TEXT("Save failed"));
    for (int32 I = 1; I < Expected.Num(); ++I)
        if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1) return Refuse(TEXT("Protected instance/function changed"));
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaThermalRetune] SAVED masterOnly=1 disabledByDefault=1 threshold=0.65 width=0.10 provisionalUnresolvedCoverage=0.20 texturesPaletteMaskGeometryUnchanged=1 renderedAcceptancePending=1"));
    return true;
}
}
#endif
