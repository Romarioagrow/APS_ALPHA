#pragma once

#if WITH_EDITOR

// Reuse the existing LWC adapter and exact evaluated MIC-state comparison.
// This header adds no global adapter change and performs no runtime selection.
#include "APSSharedLavaMaterialBuilder.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionVectorNoise.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"

namespace APSSharedAmmoniaMaterialBuilder
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid");
    inline constexpr const TCHAR* SourcePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeLiquid.M_APS_WorldScapeLiquid");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Ammonia.MI_APS_WS_Ammonia");

    inline bool SavedSourceMatches(UObject* Source, const TCHAR* ExpectedSHA1)
    {
        if (!Source || Source->GetOutermost()->IsDirty()) return false;
        TArray<uint8> Bytes;
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Source->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
        return FFileHelper::LoadFileToArray(Bytes, *Filename)
            && FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().Equals(ExpectedSHA1, ESearchCase::IgnoreCase);
    }

    inline bool SameInput(const FExpressionInput& A, const FExpressionInput& B, bool bByName)
    {
        const bool SameExpression = bByName
            ? ((!A.Expression && !B.Expression) || (A.Expression && B.Expression
                && A.Expression->GetFName() == B.Expression->GetFName()
                && A.Expression->GetClass() == B.Expression->GetClass()))
            : A.Expression == B.Expression;
        return SameExpression && A.OutputIndex == B.OutputIndex && A.Mask == B.Mask
            && A.MaskR == B.MaskR && A.MaskG == B.MaskG && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
    }

    inline bool IsReplacedSource(UMaterialExpression* E)
    {
        if (!E) return false;
        const FString Name = E->GetClass()->GetName();
        return Name == TEXT("MaterialExpressionWorldPosition")
            || Name == TEXT("MaterialExpressionActorPositionWS")
            || Name == TEXT("MaterialExpressionPixelDepth");
    }

    inline bool AuditSource(UMaterial* Source)
    {
        if (!Source || Source->GetBlendMode() != BLEND_Opaque
            || !Source->GetShadingModels().HasShadingModel(MSM_DefaultLit)
            || Source->bTangentSpaceNormal || !Source->TwoSided) return false;
        for (EMaterialProperty P : {MP_WorldPositionOffset, MP_PixelDepthOffset, MP_MaterialAttributes, MP_OpacityMask, MP_Opacity})
            if (const FExpressionInput* I = Source->GetExpressionInputForProperty(P); I && I->Expression) return false;
        const TMap<FName, int32> Expected = {
            {TEXT("MaterialExpressionActorPositionWS"), 1}, {TEXT("MaterialExpressionAdd"), 7},
            {TEXT("MaterialExpressionClamp"), 7}, {TEXT("MaterialExpressionComponentMask"), 4},
            {TEXT("MaterialExpressionDivide"), 2}, {TEXT("MaterialExpressionDotProduct"), 1},
            {TEXT("MaterialExpressionFresnel"), 1}, {TEXT("MaterialExpressionLinearInterpolate"), 3},
            {TEXT("MaterialExpressionMultiply"), 19}, {TEXT("MaterialExpressionNormalize"), 2},
            {TEXT("MaterialExpressionOneMinus"), 3}, {TEXT("MaterialExpressionPixelDepth"), 1},
            {TEXT("MaterialExpressionScalarParameter"), 11}, {TEXT("MaterialExpressionSmoothStep"), 1},
            {TEXT("MaterialExpressionSubtract"), 3}, {TEXT("MaterialExpressionVectorNoise"), 2},
            {TEXT("MaterialExpressionVectorParameter"), 7}, {TEXT("MaterialExpressionWorldPosition"), 1}
        };
        TMap<FName, int32> Actual;
        const auto Expressions = FCore::Expressions(Source);
        if (Expressions.Num() != 76) return false;
        for (UMaterialExpression* E : Expressions)
        {
            if (!E) return false;
            ++Actual.FindOrAdd(E->GetClass()->GetFName());
            if (auto* F = Cast<UMaterialExpressionFresnel>(E))
            {
                if (!F->Normal.Expression || F->Normal.Expression->GetFName() != TEXT("MaterialExpressionNormalize_0")
                    || F->Exponent != 4.0f || F->BaseReflectFraction != 0.04f
                    || F->ExponentIn.Expression || F->BaseReflectFractionIn.Expression) return false;
            }
            if (auto* N = Cast<UMaterialExpressionVectorNoise>(E))
                if (N->NoiseFunction != VNF_GradientALU || N->Quality != 1 || N->bTiling
                    || N->WorldPositionOriginType != EPositionOrigin::Absolute || !N->Position.Expression) return false;
            if (auto* S = Cast<UMaterialExpressionSmoothStep>(E))
                if (S->ConstMin != 1000.0f || S->ConstMax != 3000000.0f
                    || S->Min.Expression || S->Max.Expression || !S->Value.Expression
                    || S->Value.Expression->GetClass()->GetFName() != TEXT("MaterialExpressionPixelDepth")) return false;
        }
        if (Actual.Num() != Expected.Num()) return false;
        for (const auto& Entry : Expected)
            if (Actual.FindRef(Entry.Key) != Entry.Value) return false;
        const TPair<EMaterialProperty, const TCHAR*> Outputs[] = {
            {MP_BaseColor, TEXT("MaterialExpressionLinearInterpolate_2")},
            {MP_Metallic, TEXT("MaterialExpressionScalarParameter_3")},
            {MP_Specular, TEXT("MaterialExpressionScalarParameter_4")},
            {MP_Roughness, TEXT("MaterialExpressionClamp_5")},
            {MP_Normal, TEXT("MaterialExpressionNormalize_1")},
            {MP_EmissiveColor, TEXT("MaterialExpressionClamp_6")}
        };
        for (const auto& Output : Outputs)
        {
            const FExpressionInput* Input = Source->GetExpressionInputForProperty(Output.Key);
            if (!Input || !Input->Expression || Input->Expression->GetFName() != Output.Value) return false;
        }
        return true;
    }

    inline bool Build(IAssetTools& AssetTools)
    {
        FCore Core(AssetTools, Destination);
        const auto Fail = [&Core](const FString& Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.SharedAmmonia] Refused before binding: %s %s"), *Why, *Core.Error);
            return false;
        };
        UMaterial* Source = LoadObject<UMaterial>(nullptr, SourcePath);
        auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, TemplatePath);
        if (!Source || !Template || Template->Parent.Get() != Source
            || !SavedSourceMatches(Source, TEXT("6F941E5BEB7193391E8487989F636343CADC2904"))
            || !SavedSourceMatches(Template, TEXT("08159F80F54EE7A1E142931E375F38475CE6A077"))
            || !AuditSource(Source))
            return Fail(TEXT("Saved exact source graph/MIC changed or unsupported graph; refresh exported audit"));

        // New names only: Core.Duplicate refuses an existing package/object.
        UMaterial* Master = Cast<UMaterial>(Core.Duplicate(Source, TEXT("M_APS_SharedAmmonia")));
        if (!Master || !AuditSource(Master)) return Fail(TEXT("Duplicate source graph"));
        const auto Original = FCore::Expressions(Master);
        const auto SourceExpressions = FCore::Expressions(Source);
        TMap<FName, UMaterialExpression*> SourceByName;
        for (UMaterialExpression* E : SourceExpressions) SourceByName.Add(E->GetFName(), E);
        struct FInputSnapshot { FExpressionInput* Input; FExpressionInput Before; };
        TArray<FInputSnapshot> PreservedInputs;
        UMaterialExpressionFresnel* Fresnel = nullptr;
        for (UMaterialExpression* E : Original)
        {
            UMaterialExpression* const* S = SourceByName.Find(E->GetFName());
            if (!S || (*S)->GetClass() != E->GetClass()) return Fail(TEXT("Cloned expression identity changed"));
            const auto SourceInputs = (*S)->GetInputsView();
            const auto CloneInputs = E->GetInputsView();
            if (SourceInputs.Num() != CloneInputs.Num()) return Fail(TEXT("Cloned expression input count changed"));
            for (int32 Index = 0; Index < CloneInputs.Num(); ++Index)
            {
                if (!SourceInputs[Index] || !CloneInputs[Index]
                    || !SameInput(*SourceInputs[Index], *CloneInputs[Index], true))
                    return Fail(TEXT("Duplicate changed a source graph edge/output mask"));
                PreservedInputs.Add({CloneInputs[Index], *CloneInputs[Index]});
            }
            if (auto* F = Cast<UMaterialExpressionFresnel>(E)) Fresnel = F;
        }
        if (!Fresnel || !Core.Patch(Master) || Core.SpatialInputs != 3 || Core.Copies.Num() != 0)
            return Fail(TEXT("Expected three direct spatial inputs, no functions, and one connected Fresnel"));
        for (const FInputSnapshot& Snapshot : PreservedInputs)
        {
            if (IsReplacedSource(Snapshot.Before.Expression))
            {
                if (!Snapshot.Input->Expression || Snapshot.Input->Expression == Snapshot.Before.Expression)
                    return Fail(TEXT("Expected spatial input was not redirected"));
            }
            else if (!SameInput(*Snapshot.Input, Snapshot.Before, false))
                return Fail(TEXT("Non-spatial style/chemistry graph edge changed during adaptation"));
        }
        for (EMaterialProperty P : {MP_BaseColor, MP_Metallic, MP_Specular, MP_Roughness, MP_EmissiveColor})
            if (!SameInput(*Source->GetExpressionInputForProperty(P), *Master->GetExpressionInputForProperty(P), true))
                return Fail(TEXT("Non-normal material output changed"));

        // Candidate-local fix only: the built-in Fresnel still reads a world
        // camera vector. Its explicitly connected radial normal is now physical
        // planet-local, so restore world space only at this consumer. MP_Normal
        // already receives its own single final rotation from Core.Patch.
        const FExpressionInput PlanetNormal = Fresnel->Normal;
        FCore::FFrame FresnelFrame(Core, Master);
        UMaterialExpression* WorldFresnelNormal = FresnelFrame.Rotate(PlanetNormal, true);
        if (!WorldFresnelNormal || WorldFresnelNormal == PlanetNormal.Expression)
            return Fail(TEXT("Connected Fresnel normal was not adapted"));
        Fresnel->Normal = FExpressionInput();
        Fresnel->Normal.Expression = WorldFresnelNormal;

        auto* Context = Core.Add<UMaterialExpressionScalarParameter>(Master);
        Context->ParameterName = TEXT("APS_UsePresentationWaterMask");
        Context->DefaultValue = 0.0f;
        Context->UpdateParameterGuid(true, true);
        auto* Color = Core.Add<UMaterialExpressionVertexColor>(Master);
        auto* Shore = Core.Add<UMaterialExpressionSmoothStep>(Master);
        Shore->ConstMin = 0.08f;
        Shore->ConstMax = 0.72f;
        Shore->Value.Connect(4, Color);
        auto* Coverage = Core.Add<UMaterialExpressionLinearInterpolate>(Master);
        Coverage->ConstA = 1.0f;
        Coverage->B.Expression = Shore;
        Coverage->Alpha.Expression = Context;
        Master->BlendMode = BLEND_Masked;
        Master->OpacityMaskClipValue = 0.3333f;
        Master->GetExpressionInputForProperty(MP_OpacityMask)->Expression = Coverage;

        auto* Copy = Cast<UMaterialInstanceConstant>(Core.Duplicate(Template, TEXT("MI_APS_SharedAmmonia")));
        if (!Copy) return Fail(TEXT("Exact MIC duplicate"));
        Copy->SetParentEditorOnly(Master, false);
        Master->PostEditChange();
        Copy->PostEditChange();
        if (!APSSharedLavaMaterialBuilder::SameTypeState(Template, Copy)
            || Copy->GetMaterial() != Master || Copy->GetBlendMode() != BLEND_Masked
            || !Master->TwoSided || Master->bTangentSpaceNormal
            || !Master->GetShadingModels().HasShadingModel(MSM_DefaultLit)
            || Core.Outputs.Num() != 2)
            return Fail(TEXT("Evaluated family state, shading, exact parent or output count changed"));

        for (UObject* Output : Core.Outputs)
            CastChecked<UMaterialInterface>(Output)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (UObject* Output : Core.Outputs)
        {
            auto* Material = CastChecked<UMaterialInterface>(Output);
            FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
            FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            const bool Ready = Resource && Resource->IsGameThreadShaderMapComplete()
                && Resource->GetCompileErrors().Num() == 0 && Map
                && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedAmmonia] Ready=%d path=%s feature=%d expressions=%d source=76 spatial=3 fresnelConsumer=world palette=unchanged"),
                Ready, *Material->GetPathName(), int32(GMaxRHIFeatureLevel), FCore::Expressions(Master).Num());
            if (!Ready)
            {
                if (Resource) for (const FString& Error : Resource->GetCompileErrors())
                    UE_LOG(LogTemp, Error, TEXT("[APS.SharedAmmonia] %s"), *Error);
                return Fail(TEXT("Exact permutation incomplete, compile errors, or missing LocalVF"));
            }
        }
        for (UObject* Output : Core.Outputs)
        {
            UPackage* Package = Output->GetOutermost();
            if (!Package->GetName().StartsWith(FString(Destination) + TEXT("/")))
                return Fail(TEXT("Output escaped the new shared-liquid packages"));
            Package->MarkPackageDirty();
            const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone;
            Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, Output, *File, Args)) return Fail(TEXT("Save new candidate package"));
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.SharedAmmonia] Candidate saved, NOT runtime bound: outputs=2 parent=%s"), *Copy->GetPathName());
        return true;
    }
}
#endif
