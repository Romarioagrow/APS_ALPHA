#pragma once
#if WITH_EDITOR
#include "APSSharedLavaMaterialBuilder.h"
#include "APSLavaDerivativeFix.h"

// Diagnostic only. Duplicate the already adapted graph, never adapt it twice.
namespace APSLavaSamplingABBuilder
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaSamplingAB");
    inline bool Build(IAssetTools& AssetTools, bool bExplicitDerivatives = false, bool bScaleIsolation = false)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        if (bExplicitDerivatives && bScaleIsolation) return false;
        const TCHAR* OutputRoot = bScaleIsolation
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaScaleAB") : bExplicitDerivatives
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/LavaDerivativeFix") : Destination;
        FCore Core(AssetTools, OutputRoot);
        const auto Fail = [&Core](const TCHAR* Reason)
        { UE_LOG(LogTemp, Error, TEXT("[APS.LavaSamplingAB] Refused: %s %s"), Reason, *Core.Error); return false; };
        auto* Source = LoadObject<UMaterial>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/M_APS_SharedLava.M_APS_SharedLava"));
        auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedLavaNative.MI_APS_SharedLavaNative"));
        auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedLava.MI_APS_SharedLava"));
        if (!Source || !Native || !APS || Native->Parent.Get() != Source || APS->Parent.Get() != Native
            || FCore::Expressions(Source).Num() != 115 || Source->GetBlendMode() != BLEND_Masked)
            return Fail(TEXT("Exported shared Lava graph/parent chain changed"));
        float Brightness = -1.0f;
        if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), Brightness) || Brightness != 0.0f)
            return Fail(TEXT("Sampling isolation requires the audited Brightness=0"));
        for (UMaterialExpression* E : FCore::Expressions(Source))
            if (auto* S = Cast<UMaterialExpressionScalarParameter>(E); S && S->ParameterName.ToString().StartsWith(TEXT("APS_DebugUse")))
                return Fail(TEXT("Source is already diagnostic"));

        auto* Master = Cast<UMaterial>(Core.Duplicate(Source, TEXT("M_APS_LavaSamplingAB")));
        if (!Master) return Fail(TEXT("Duplicate diagnostic master"));
        if (bExplicitDerivatives && !APSLavaDerivativeFix::Apply(Core, Master))
            return Fail(TEXT("Private WAT unwrapped derivative adaptation"));
        const TArray<UMaterialExpression*> Original = FCore::Expressions(Master);
        auto* Vnoise = FindObject<UMaterialExpressionAdd>(Master, TEXT("MaterialExpressionAdd_5"));
        auto* Crust = FindObject<UMaterialExpressionMultiply>(Master, TEXT("MaterialExpressionMultiply_105"));
        if (!Vnoise || !Crust || !Original.Contains(Vnoise) || !Original.Contains(Crust)
            || !Vnoise->A.Expression || Vnoise->A.Expression->GetName() != TEXT("MaterialExpressionTextureSampleParameterVolume_5")
            || !Vnoise->B.Expression || Vnoise->B.Expression->GetName() != TEXT("MaterialExpressionDivide_1")
            || !Crust->A.Expression || Crust->A.Expression->GetName() != TEXT("MaterialExpressionAdd_26")
            || !Crust->B.Expression || Crust->B.Expression->GetName() != TEXT("MaterialExpressionMultiply_104"))
            return Fail(TEXT("Exact Vnoise1/WAT branch roots changed"));

        // Capture original consumers before creating gates, preventing a self-cycle.
        TArray<FExpressionInput*> NoiseConsumers, CrustConsumers;
        TSet<FString> NoiseOwners, CrustOwners;
        for (UMaterialExpression* E : Original)
            for (FExpressionInput* Input : E->GetInputsView())
            {
                if (!Input) continue;
                if (Input->Expression == Vnoise) { NoiseConsumers.Add(Input); NoiseOwners.Add(E->GetName()); }
                if (Input->Expression == Crust) { CrustConsumers.Add(Input); CrustOwners.Add(E->GetName()); }
            }
        if (NoiseConsumers.Num() != 3 || NoiseOwners.Num() != 3
            || !NoiseOwners.Contains(TEXT("MaterialExpressionOneMinus_2"))
            || !NoiseOwners.Contains(TEXT("MaterialExpressionOneMinus_3"))
            || !NoiseOwners.Contains(TEXT("MaterialExpressionAdd_34"))
            || CrustConsumers.Num() != 3 || CrustOwners.Num() != 3
            || !CrustOwners.Contains(TEXT("MaterialExpressionComponentMask_1"))
            || !CrustOwners.Contains(TEXT("MaterialExpressionDivide_2"))
            || !CrustOwners.Contains(TEXT("MaterialExpressionReroute_3")))
            return Fail(TEXT("Audited six branch-consumer edges changed"));
        for (int32 P = 0; P < MP_MAX; ++P)
            if (const FExpressionInput* Input = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P));
                Input && (Input->Expression == Vnoise || Input->Expression == Crust))
                return Fail(TEXT("Unexpected direct material output consumer"));
        const auto Gate = [&Core, Master](UMaterialExpression* Input, const TCHAR* Name)
        {
            auto* Enable = Core.Add<UMaterialExpressionScalarParameter>(Master);
            Enable->ParameterName = Name; Enable->DefaultValue = 1.0f;
            Enable->UpdateParameterGuid(true, true);
            auto* Mix = Core.Add<UMaterialExpressionLinearInterpolate>(Master);
            Mix->ConstA = 0.5f; Mix->B.Expression = Input; Mix->Alpha.Expression = Enable;
            return Mix;
        };
        auto* NoiseGate = Gate(Vnoise, TEXT("APS_DebugUseVnoise1"));
        auto* CrustGate = Gate(Crust, TEXT("APS_DebugUseWATCrust"));
        for (FExpressionInput* Input : NoiseConsumers) Input->Expression = NoiseGate;
        for (FExpressionInput* Input : CrustConsumers) Input->Expression = CrustGate;
        if (bScaleIsolation)
        {
            auto* FineOffset = FindObject<UMaterialExpressionAdd>(Master, TEXT("MaterialExpressionAdd_26"));
            auto* CoarseProduct = FindObject<UMaterialExpressionMultiply>(Master, TEXT("MaterialExpressionMultiply_104"));
            if (!FineOffset || !CoarseProduct || FineOffset->ConstB != 0.3f)
                return Fail(TEXT("WAT scale product/offset topology changed"));
            FExpressionInput* Edges[] = {&FineOffset->A, &CoarseProduct->A, &CoarseProduct->B};
            const TCHAR* Calls[] = {TEXT("MaterialExpressionMaterialFunctionCall_14"),
                TEXT("MaterialExpressionMaterialFunctionCall_16"), TEXT("MaterialExpressionMaterialFunctionCall_17")};
            const TCHAR* Params[] = {TEXT("APS_DebugUseWAT400m"), TEXT("APS_DebugUseWAT5431m"), TEXT("APS_DebugUseWAT20km")};
            const TCHAR* SizeNodes[] = {TEXT("MaterialExpressionMultiply_18"), TEXT("MaterialExpressionMultiply_15"), TEXT("MaterialExpressionMultiply_6")};
            const float Scales[] = {400.0f, 5431.0f, 20000.0f};
            for (int32 I = 0; I < 3; ++I)
            {
                auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Edges[I]->Expression);
                if (!Call || Call->GetName() != Calls[I] || Edges[I]->OutputIndex != 2 || Edges[I]->Mask
                    || !Call->MaterialFunction || Call->MaterialFunction->GetPathName() !=
                        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MF_APS_WorldAlignedTexture_a83aa78c.MF_APS_WorldAlignedTexture_a83aa78c"))
                    return Fail(TEXT("Exact three original WAT XYZ edges changed"));
                const FFunctionExpressionInput* TextureSize = Call->FunctionInputs.FindByPredicate(
                    [](const FFunctionExpressionInput& P) { return P.Input.InputName == TEXT("TextureSize"); });
                auto* Size = TextureSize ? Cast<UMaterialExpressionMultiply>(TextureSize->Input.Expression) : nullptr;
                auto* Meters = Size ? Cast<UMaterialExpressionConstant>(Size->A.Expression) : nullptr;
                auto* Cm = Size ? Cast<UMaterialExpressionConstant>(Size->B.Expression) : nullptr;
                if (!Size || Size->GetName() != SizeNodes[I] || !Meters || Meters->R != Scales[I] || !Cm || Cm->R != 100.0f)
                    return Fail(TEXT("WAT 400m/5431m/20km physical scales changed"));
                auto* ScaleGate = Gate(Call, Params[I]);
                ScaleGate->B = *Edges[I]; // Keep XYZ output2 on the sampled branch.
                Edges[I]->Expression = ScaleGate;
                Edges[I]->OutputIndex = 0; // Lerp has one output, unlike WAT.
                if (ScaleGate->B.Expression != Call || ScaleGate->B.OutputIndex != 2)
                    return Fail(TEXT("Scale gate lost XYZ output or formed a self-cycle"));
            }
        }
        // Only Expression pointers changed. Masks/output indices are untouched.
        // Add_34 also feeds emission, intentionally disabled by Brightness=0.
        if (FCore::Expressions(Master).Num() != (bScaleIsolation ? 125 : 119) || NoiseGate->B.Expression != Vnoise || CrustGate->B.Expression != Crust)
            return Fail(TEXT("Unexpected gate graph or self-cycle"));
        auto* NativeCopy = Cast<UMaterialInstanceConstant>(Core.Duplicate(Native, TEXT("MI_APS_LavaSamplingABNative")));
        auto* APSCopy = Cast<UMaterialInstanceConstant>(Core.Duplicate(APS, TEXT("MI_APS_LavaSamplingAB")));
        if (!NativeCopy || !APSCopy) return Fail(TEXT("Duplicate exact instance chain"));
        NativeCopy->SetParentEditorOnly(Master, false); APSCopy->SetParentEditorOnly(NativeCopy, false);
        Master->PostEditChange(); NativeCopy->PostEditChange(); APSCopy->PostEditChange();
        if (!APSSharedLavaMaterialBuilder::SameTypeState(Native, NativeCopy)
            || !APSSharedLavaMaterialBuilder::SameTypeState(APS, APSCopy)
            || APSCopy->GetMaterial() != Master || Core.Outputs.Num() != (bExplicitDerivatives ? 4 : 3))
            return Fail(TEXT("Inherited authored parameter state changed"));
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaSamplingAB] Preflight sourceExpressions=115 outputExpressions=%d noiseEdges=3 crustEdges=3 clonedFunctions=%d explicitDerivatives=%d scaleIsolation=%d brightness=0"), FCore::Expressions(Master).Num(), bExplicitDerivatives ? 1 : 0, bExplicitDerivatives, bScaleIsolation);
        for (UObject* Output : Core.Outputs)
            if (auto* Material = Cast<UMaterialInterface>(Output))
                Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (UObject* Output : Core.Outputs)
        {
            auto* Material = Cast<UMaterialInterface>(Output);
            if (!Material) continue;
            FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
            FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            const bool Ready = Resource && Resource->IsGameThreadShaderMapComplete()
                && Resource->GetCompileErrors().Num() == 0 && Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
            UE_LOG(LogTemp, Display, TEXT("[APS.LavaSamplingAB] Ready=%d path=%s"), Ready, *Material->GetPathName());
            if (!Ready)
            {
                if (Resource) for (const FString& Error : Resource->GetCompileErrors()) UE_LOG(LogTemp, Error, TEXT("[APS.LavaSamplingAB] %s"), *Error);
                return Fail(TEXT("Incomplete shader map, compile errors, or absent LocalVF"));
            }
        }
        for (UObject* Output : Core.Outputs)
        {
            UPackage* Package = Output->GetOutermost();
            if (!Package->GetName().StartsWith(FString(OutputRoot) + TEXT("/"))) return Fail(TEXT("Package escaped diagnostic folder"));
            Package->MarkPackageDirty();
            const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, Output, *File, Args)) return Fail(TEXT("Save diagnostic-only package"));
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaSamplingAB] Saved %d new assets, explicitDerivatives=%d. NOT selected by production."), Core.Outputs.Num(), bExplicitDerivatives);
        return true;
    }
}
#endif
