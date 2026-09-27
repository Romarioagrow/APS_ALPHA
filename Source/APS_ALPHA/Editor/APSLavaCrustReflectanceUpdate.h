#pragma once
#if WITH_EDITOR
#include "APSLavaAntiGridUpdate.h"
#include "Materials/MaterialExpressionClamp.h"

// Offline, hash-guarded correction of reflected crust light only. No runtime
// mode-dependent tint, emission compensation, coordinate or texture changes.
namespace APSLavaCrustReflectanceUpdate
{
    struct FExpected { const TCHAR* Name; const TCHAR* SHA1; };
    inline constexpr FExpected Expected[] = {
        {TEXT("M_APS_SharedLava"), TEXT("42EB0E9B4318B27A6F6C62640C226EF8E761DB2B")},
        {TEXT("MI_APS_SharedLavaNative"), TEXT("1F3F7FE8F5562477149A54E4F6B42EF31D155E51")},
        {TEXT("MI_APS_SharedLava"), TEXT("A995F1FD467F9BFE44E370C34AF1E4B4680E9DE0")},
        {TEXT("MF_APS_LavaWAT20kmAntiGrid_v1"), TEXT("27640676C83642BF2A50C432F1D605C2025FA306")}
    };

    inline bool SameInput(const FExpressionInput& A, const FExpressionInput& B)
    {
        return A.Expression == B.Expression && A.OutputIndex == B.OutputIndex
            && A.Mask == B.Mask && A.MaskR == B.MaskR && A.MaskG == B.MaskG
            && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
    }

    inline bool Run(IAssetTools& Tools)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        using APSLavaAntiGridUpdate::Filename;
        using APSLavaAntiGridUpdate::Hash;
        FCore Core(Tools, APSSharedLavaMaterialBuilder::Destination);
        const auto Refuse = [](const FString& Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.LavaCrustReflectance] Refused: %s"), *Why);
            return false;
        };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline commandlet only"));
        for (const FExpected& E : Expected)
            if (Hash(Filename(E.Name)) != E.SHA1)
                return Refuse(TEXT("Accepted asset changed: ") + Filename(E.Name));
        const FString Root(APSSharedLavaMaterialBuilder::Destination);
        auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
        auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
        auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
        if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty()
            || Native->Parent.Get() != Master || APS->Parent.Get() != Native
            || Master->GetBlendMode() != BLEND_Masked || FCore::Expressions(Master).Num() != 115)
            return Refuse(TEXT("Unexpected graph/parent/blend/unsaved state"));
        FExpressionInput* Base = Master->GetExpressionInputForProperty(MP_BaseColor);
        if (!Base || !Base->Expression || Base->Expression->GetName() != TEXT("MaterialExpressionMultiply_1")
            || Base->OutputIndex != 0 || Base->Mask)
            return Refuse(TEXT("Native reflected-light root changed"));
        const FExpressionInput OriginalBase = *Base;
        TMap<int32, FExpressionInput> OriginalProperties;
        for (int32 P = 0; P < MP_MAX; ++P)
            if (const FExpressionInput* Input = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P)))
                OriginalProperties.Add(P, *Input);
        float Brightness = -1.0f;
        FLinearColor Emission;
        if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), Brightness) || Brightness != 2.0f
            || !APS->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveColor")), Emission)
            || !Emission.Equals(FLinearColor(0.42f, 0.018f, 0.001f, 1.0f), 1.e-6f))
            return Refuse(TEXT("Saved emission authority changed"));
        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()
            / TEXT("SharedLavaCrustReflectanceBackup_20260926"));
        if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup already exists"));
        if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
        for (const FExpected& E : Expected)
        {
            const FString Source = Filename(E.Name), Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
            if (Hash(Source) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Source, false, false) != COPY_OK
                || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup mismatch before edit"));
        }

        // The legacy texture product is a pattern, not the albedo of cooled
        // basalt. Keep its spatial pattern and bound its reflected contribution.
        // 0.08 is a tunable initial linear reflectance ceiling, not an emissive
        // value or a claim of measured composition for every generated planet.
        auto* Pattern = Core.Add<UMaterialExpressionClamp>(Master);
        Pattern->Input = OriginalBase;
        Pattern->MinDefault = 0.0f; Pattern->MaxDefault = 1.0f;
        auto* Reflectance = Core.Add<UMaterialExpressionScalarParameter>(Master);
        Reflectance->ParameterName = TEXT("APS_LavaCrustReflectance");
        Reflectance->DefaultValue = 0.08f;
        Reflectance->SliderMin = 0.0f; Reflectance->SliderMax = 1.0f;
        Reflectance->Group = TEXT("APS Lava Surface");
        Reflectance->UpdateParameterGuid(true, true);
        auto* Bound = Core.Add<UMaterialExpressionClamp>(Master);
        Bound->Input.Connect(0, Reflectance);
        Bound->MinDefault = 0.0f; Bound->MaxDefault = 1.0f;
        auto* Diffuse = Core.Add<UMaterialExpressionMultiply>(Master);
        Diffuse->A.Connect(0, Pattern); Diffuse->B.Connect(0, Bound);
        Base->Connect(0, Diffuse);
        if (FCore::Expressions(Master).Num() != 119 || !SameInput(Pattern->Input, OriginalBase))
            return Refuse(TEXT("Unexpected graph rewrite"));
        for (const auto& Pair : OriginalProperties)
        {
            if (Pair.Key == MP_BaseColor) continue;
            const FExpressionInput* Now = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Pair.Key));
            if (!Now || !SameInput(*Now, Pair.Value))
                return Refuse(TEXT("A non-base-color material output changed"));
        }
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
                return Refuse(TEXT("Shader/LocalVF not ready; no asset saved"));
        }
        float ActualReflectance = -1.0f, ActualBrightness = -1.0f;
        FLinearColor ActualEmission;
        if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaCrustReflectance")), ActualReflectance)
            || ActualReflectance != 0.08f
            || !APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), ActualBrightness)
            || ActualBrightness != Brightness
            || !APS->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveColor")), ActualEmission)
            || ActualEmission != Emission) return Refuse(TEXT("Inherited parameter contract changed"));
        for (const FExpected& E : Expected)
            if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent asset edit"));
        UPackage* Package = Master->GetOutermost();
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Master, *Filename(Expected[0].Name), Args))
            return Refuse(TEXT("Master save failed; retain verified backup"));
        for (int32 I = 1; I < UE_ARRAY_COUNT(Expected); ++I)
            if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1)
                return Refuse(TEXT("Protected MIC/function changed"));
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaCrustReflectance] SAVED masterOnly=1 reflectance=0.08 emissionUnchanged=1 coordinatesTexturesMaskUnchanged=1 newTextureSamples=0 backup=%s renderedAcceptancePending=1"), *Backup);
        return true;
    }
}
#endif
