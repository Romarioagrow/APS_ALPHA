#pragma once

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "APSContinuousWarpPixelAssets.h"
#include "APS_ALPHA/Core/Planetary/APSTerrainContinuityMaterial.h"
#include "AssetCompilingManager.h"
#include "Engine/World.h"
#include "LocalVertexFactory.h"
#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderCommandFence.h"
#include "Settings/EditorLoadingSavingSettings.h"
#include "ShaderCompiler.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

// Caller MUST first accept the existing exact-save/private-UserDir FGuard and
// identify the actual saved Lidim/root/MID. Process-wide experiment, not per-body.
// No cloning, MID replacement, graph-link changes, SavePackage or SaveConfig.
namespace APSSavedSlopeIsolation
{
    enum class EReadiness : uint8 { Pending, Ready, Failed };
    inline constexpr const TCHAR* FunctionPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_SlopeBlock_f6be5407.MF_APS_MF_SlopeBlock_f6be5407");
    inline const FString& OriginalCode()
    {
        static const FString Code = TEXT("float d=length(CameraDelta.xyz)*max(InverseScale.x,0.0);\n")
            TEXT("float t=saturate((d-StartCm)/max(EndCm-StartCm,1.0));\n")
            TEXT("if(t<=0.0) return NativeSlope;\nif(t>=1.0) return PixelSlope;\n")
            TEXT("float w=t*t*(3.0-2.0*t);\nreturn lerp(NativeSlope,PixelSlope,w);");
        return Code;
    }

    class FScope
    {
    public:
        FScope() = default;
        FScope(const FScope&) = delete;
        FScope& operator=(const FScope&) = delete;
        ~FScope() { if (bActive && !bReleaseAttempted) ReleaseAfterCapture(); }
        bool Prepared() const { return bPrepared; }
        bool Active() const { return bActive; }

        EReadiness Prepare(UWorld* InWorld, UMaterialInstanceDynamic* ActualLidimMID,
            bool bNativeSlope, FString& Error)
        {
            check(IsInGameThread()); Error.Reset();
            const auto Fail = [&](const FString& Why) { bFailed = true; Error = Why; return EReadiness::Failed; };
            if (bFailed || bReleaseAttempted) return Fail(TEXT("Slope isolation scope already failed/released"));
            if (!bActive)
            {
                const TCHAR* Cmd = FCommandLine::Get();
                if (!IsValid(InWorld) || !IsValid(ActualLidimMID) || !FApp::IsUnattended()
                    || !GIsAutomationTesting || IsRunningCommandlet() || FParse::Param(Cmd, TEXT("NullRHI"))
                    || !FParse::Param(Cmd, TEXT("APSSavedPlanetReplay"))
                    || !FParse::Param(Cmd, TEXT("APSSavedPlanetLidimApproach")))
                    return Fail(TEXT("Slope isolation requires guarded rendered saved-Lidim replay"));
                if (!APSContinuousWarpPixelAssets::VerifySources(Error)) { bFailed = true; return EReadiness::Failed; }
                Master.Reset(ActualLidimMID->GetMaterial());
                Template.Reset(Cast<UMaterialInstance>(ActualLidimMID->Parent.Get()));
                Function.Reset(LoadObject<UMaterialFunction>(nullptr, FunctionPath));
                if (!Master.IsValid() || Master->GetPathName() != APSTerrainContinuityMaterial::MasterPath
                    || !Template.IsValid() || Template->GetPathName() != APSTerrainContinuityMaterial::TemplatePath
                    || Template->Parent.Get() != Master.Get() || !Function.IsValid()
                    || Master->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty()
                    || Function->GetOutermost()->IsDirty())
                    return Fail(TEXT("Slope isolation requires exact clean Continuous hierarchy and source function"));
                int32 Calls = 0;
                for (UMaterialExpression* E : Master->GetExpressions())
                    if (auto* C = Cast<UMaterialExpressionMaterialFunctionCall>(E); C && C->MaterialFunction == Function.Get()) ++Calls;
                TArray<UMaterialExpression*> Pending; TSet<UMaterialExpression*> Seen;
                for (UMaterialExpression* E : Function->GetExpressions()) Pending.Add(E);
                for (int32 I = 0; I < Pending.Num(); ++I)
                {
                    UMaterialExpression* E = Pending[I]; if (!E || Seen.Contains(E)) continue;
                    if (Seen.Num() >= 512 || !E->IsIn(Function.Get())) return Fail(TEXT("Slope expression ownership/bound changed"));
                    Seen.Add(E);
                    if (E->GetFName() == TEXT("APS_FarSlopePixel_1"))
                    {
                        if (LegacyCall || !Cast<UMaterialExpressionMaterialFunctionCall>(E))
                            return Fail(TEXT("Ambiguous legacy far-slope call"));
                        LegacyCall = CastChecked<UMaterialExpressionMaterialFunctionCall>(E);
                    }
                    if (auto* C = Cast<UMaterialExpressionCustom>(E);
                        C && C->Description == TEXT("APS preserve native near slope; smooth far pixel slope v1"))
                    { if (Blend) return Fail(TEXT("Ambiguous slope blend")); Blend = C; }
                    for (FExpressionInput* Input : E->GetInputsView()) if (Input && Input->Expression) Pending.Add(Input->Expression);
                }
                if (Calls != 1 || !Blend || Blend->Code != OriginalCode() || Blend->OutputType != CMOT_Float1 || Blend->Inputs.Num() != 6)
                    return Fail(TEXT("Exact one-call/six-input/original-Code slope contract changed"));
                const TCHAR* Names[] = {TEXT("NativeSlope"), TEXT("PixelSlope"), TEXT("CameraDelta"), TEXT("InverseScale"), TEXT("StartCm"), TEXT("EndCm")};
                for (int32 I = 0; I < 6; ++I)
                    if (Blend->Inputs[I].InputName != Names[I] || !Blend->Inputs[I].Input.Expression
                        || Blend->Inputs[I].Input.OutputIndex || Blend->Inputs[I].Input.Mask)
                        return Fail(TEXT("Slope input name/expression/mapping changed"));
                if (!Cast<UMaterialExpressionVertexInterpolator>(Blend->Inputs[0].Input.Expression))
                    return Fail(TEXT("Native slope is not the retained vertex interpolator"));
                if (!LegacyCall || Function->GetExpressions().Contains(LegacyCall))
                    return Fail(TEXT("Expected reachable unregistered legacy far-slope call"));
                World = InWorld; Actual.Reset(ActualLidimMID); FeatureLevel = InWorld->GetFeatureLevel();
                bNative = bNativeSlope; Started = FPlatformTime::Seconds();
                Settings = GetMutableDefault<UEditorLoadingSavingSettings>();
                SavedAuto = Settings->bAutoSaveEnable; SavedMaps = Settings->bAutoSaveMaps; SavedContent = Settings->bAutoSaveContent;
                PriorSaveVeto = FCoreUObjectDelegates::IsPackageOKToSaveDelegate;
                FCoreUObjectDelegates::IsPackageOKToSaveDelegate.BindLambda([](UPackage* P, const FString&, FOutputDevice*)
                { UE_LOG(LogTemp, Error, TEXT("[APS.SavedSlopeIsolation] Package save vetoed: %s"), *GetNameSafe(P)); return false; });
                SaveVetoHandle = FCoreUObjectDelegates::IsPackageOKToSaveDelegate.GetHandle();
                Settings->bAutoSaveEnable = Settings->bAutoSaveMaps = Settings->bAutoSaveContent = false;
                bActive = true; // Own cleanup before the first graph mutation.
                // Explicit diagnostic preparation, identical in both modes. This
                // one legacy call has serialized wires/GUIDs but is absent from
                // the flat collection used by UE's transient-pin hydration.
                // Do not change the collection, persisted links or function math.
                if (!CheckLegacyPins(true, Error)) { bFailed = true; return EReadiness::Failed; }
                if (bNative) Blend->Code = TEXT("return NativeSlope;");
                Recompile(); // Identical update/recompile path in control and candidate.
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedSlopeIsolation] begin mode=%s actualMID=%s sharedFunction=%s; process-wide Code-only, no MID/UV/normal/palette replacement; prepareCap=120s"),
                    bNative ? TEXT("native-slope") : TEXT("control"), *Actual->GetPathName(), FunctionPath);
            }
            if (World.Get() != InWorld || Actual.Get() != ActualLidimMID || bNative != bNativeSlope)
                return Fail(TEXT("Slope isolation preparation identity/mode changed"));
            if (!CheckIdentity(Error)) { bFailed = true; return EReadiness::Failed; }
            if (bPrepared) return Verify(Error) ? EReadiness::Ready : Fail(Error);
            if (FPlatformTime::Seconds() - Started > 120.0) return Fail(TEXT("Slope isolation shader/fence preparation exceeded 120s"));
            FMaterialResource* R = Actual->GetMaterialResource(FeatureLevel); // UE5.4 follows MID Parent to static permutation.
            if (!R) return Fail(TEXT("Actual Continuous MID has no feature-level material resource"));
            if (R->GetCompileErrors().Num()) return Fail(CompileError(R));
            if (Submitted != R) { Submitted = R; bFence = false; R->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal); }
            if (!R->IsGameThreadShaderMapComplete()) return EReadiness::Pending;
            const FMaterialShaderMap* Map = R->GetGameThreadShaderMap();
            if (!Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Fail(TEXT("Actual Continuous permutation lacks shader map/LocalVF"));
            if (!bFence) { Fence.BeginFence(); bFence = true; return EReadiness::Pending; }
            if (!Fence.IsFenceComplete()) return EReadiness::Pending;
            if (!APSContinuousWarpPixelAssets::VerifySources(Error)) { bFailed = true; return EReadiness::Failed; }
            bPrepared = true;
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedSlopeIsolation] ready mode=%s shaderComplete=1 LocalVF=1 renderFence=1 diskSourcesUnchanged=1 elapsed=%.3f"), bNative ? TEXT("native-slope") : TEXT("control"), FPlatformTime::Seconds() - Started);
            return EReadiness::Ready;
        }

        bool Verify(FString& Error) const
        {
            check(IsInGameThread()); Error.Reset();
            if (!bPrepared || bFailed || !CheckIdentity(Error))
            { if (Error.IsEmpty()) Error = TEXT("Slope isolation not prepared/failed"); return false; }
            FMaterialResource* R = Actual->GetMaterialResource(FeatureLevel);
            const FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
            if (R != Submitted || !R || R->GetCompileErrors().Num() || !R->IsGameThreadShaderMapComplete()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType) || !Fence.IsFenceComplete())
            { Error = TEXT("Slope isolation actual shader/fence changed during capture"); return false; }
            return true;
        }

        // Call strictly after the final synchronous readback. This isolated run
        // ends immediately after its one test: keep saving vetoed until exit even
        // after restoration, since recompiling can dirty other dependent packages.
        bool ReleaseAfterCapture()
        {
            check(IsInGameThread());
            if (bReleaseAttempted) return bRestored;
            const bool bCanRestoreShader = bPrepared && !bFailed;
            bReleaseAttempted = true; bPrepared = false;
            if (!bActive) return true;
            FString Error;
            const bool bOwned = CheckIdentity(Error);
            if (bOwned && IsValid(Blend) && Function.IsValid() && Master.IsValid())
            {
                Blend->Code = OriginalCode();
                if (!bCanRestoreShader)
                {
                    UE_LOG(LogTemp, Error, TEXT("[APS.SavedSlopeIsolation] pending/failed preparation: original Code restored but compiled shader NOT restored; no further capture, saving vetoed until isolated process exit; no second blocking compile"));
                    return false;
                }
                Recompile();
                if (FMaterialResource* R = Actual.IsValid() ? Actual->GetMaterialResource(FeatureLevel) : nullptr)
                    R->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
                FAssetCompilingManager::Get().FinishAllCompilation();
                if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
                Fence.BeginFence(); Fence.Wait();
                FMaterialResource* R = Actual.IsValid() ? Actual->GetMaterialResource(FeatureLevel) : nullptr;
                const FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
                FString DiskError;
                if (R && R->GetCompileErrors().Num()) Error += CompileError(R);
                bRestored = bOwned && Blend->Code == OriginalCode() && R && !R->GetCompileErrors().Num()
                    && R->IsGameThreadShaderMapComplete() && Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
                    && FCoreUObjectDelegates::IsPackageOKToSaveDelegate.GetHandle() == SaveVetoHandle
                    && !Settings->bAutoSaveEnable && !Settings->bAutoSaveMaps && !Settings->bAutoSaveContent
                    && APSContinuousWarpPixelAssets::VerifySources(DiskError);
                Error += DiskError;
            }
            if (bRestored)
            {
                bActive = false;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedSlopeIsolation] restored original Code/shader; source hashes unchanged; package-save veto/autosave OFF retained until isolated process exit; no dirty flags or StateIds reset"));
            }
            else UE_LOG(LogTemp, Error, TEXT("[APS.SavedSlopeIsolation] restore failed; keep save veto/autosave OFF until process exit: %s"), Error.IsEmpty() ? TEXT("original shader readiness/ownership check failed") : *Error);
            return bRestored;
        }

    private:
        bool CheckLegacyPins(bool bHydrate, FString& Error) const
        {
            const auto Fail = [&]() { Error = TEXT("Exact diagnostic legacy-call GUID/wire/transient-pin contract changed"); return false; };
            if (!IsValid(LegacyCall) || !Function.IsValid() || !LegacyCall->IsIn(Function.Get())
                || LegacyCall->GetFName() != TEXT("APS_FarSlopePixel_1")) return Fail();
            auto* Callee = Cast<UMaterialFunction>(LegacyCall->MaterialFunction);
            if (!Callee || Callee->GetPathName() != TEXT("/Game/Ressources/Materials/WorldScapeMaterials/Functions/Customizatation/MF_CheapContrastNoClamp.MF_CheapContrastNoClamp")
                || LegacyCall->FunctionInputs.Num() != 2 || LegacyCall->FunctionOutputs.Num() != 1) return Fail();
            TArray<FFunctionExpressionInput> Inputs;
            TArray<FFunctionExpressionOutput> Outputs;
            Callee->GetInputsAndOutputs(Inputs, Outputs);
            if (Inputs.Num() != 2 || Outputs.Num() != 1) return Fail();
            const TCHAR* Ids[] = {TEXT("95C9DA0E48D34168E34AD3A607BAA34E"), TEXT("3C3E177E485D8FFFC35D1482F0A20020")};
            const TCHAR* Names[] = {TEXT("In"), TEXT("Contrast")};
            const TCHAR* Wires[] = {TEXT("APS_FarSlopePixel_2"), TEXT("MaterialExpressionConstant_10")};
            UMaterialExpressionFunctionInput* Resolved[2] = {};
            int32 Missing = 0;
            for (int32 I = 0; I < 2; ++I)
            {
                const auto& P = LegacyCall->FunctionInputs[I];
                const auto& Wire = P.Input;
                if (P.ExpressionInputId.ToString(EGuidFormats::Digits) != Ids[I] || Wire.InputName != Names[I]
                    || !Wire.Expression || !Wire.Expression->IsIn(Function.Get()) || Wire.Expression->GetFName() != Wires[I]
                    || Wire.OutputIndex || Wire.Mask || Wire.MaskR || Wire.MaskG || Wire.MaskB || Wire.MaskA) return Fail();
                int32 Matches = 0;
                for (const auto& Target : Inputs)
                    if (Target.ExpressionInputId == P.ExpressionInputId && Target.Input.InputName == Wire.InputName)
                    { ++Matches; Resolved[I] = Target.ExpressionInput; }
                if (Matches != 1 || !Resolved[I] || !Resolved[I]->IsIn(Callee)
                    || (P.ExpressionInput && P.ExpressionInput != Resolved[I]) || (!bHydrate && !P.ExpressionInput)) return Fail();
                Missing += P.ExpressionInput == nullptr;
            }
            const auto& P = LegacyCall->FunctionOutputs[0];
            const auto& Target = Outputs[0];
            if (P.ExpressionOutputId.ToString(EGuidFormats::Digits) != TEXT("5067ED8C4A2EFE82DB33F99EBE37E80E")
                || P.Output.OutputName != TEXT("Result") || P.Output.Mask || P.Output.MaskR || P.Output.MaskG || P.Output.MaskB || P.Output.MaskA
                || Target.ExpressionOutputId != P.ExpressionOutputId || Target.Output.OutputName != P.Output.OutputName
                || !Target.ExpressionOutput || !Target.ExpressionOutput->IsIn(Callee)
                || (P.ExpressionOutput && P.ExpressionOutput != Target.ExpressionOutput) || (!bHydrate && !P.ExpressionOutput)) return Fail();
            Missing += P.ExpressionOutput == nullptr;
            if (bHydrate)
            {
                LegacyCall->FunctionInputs[0].ExpressionInput = Resolved[0];
                LegacyCall->FunctionInputs[1].ExpressionInput = Resolved[1];
                LegacyCall->FunctionOutputs[0].ExpressionOutput = Target.ExpressionOutput;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedSlopeIsolation] explicit test-only transient hydration call=APS_FarSlopePixel_1 missing=%d; exact2inputs/1output, serialized wires/GUIDs/collections untouched; identical Control/Native preparation"), Missing);
            }
            return true;
        }
        void Recompile() { UMaterialEditingLibrary::UpdateMaterialFunction(Function.Get()); UMaterialEditingLibrary::RecompileMaterial(Master.Get()); }
        static FString CompileError(FMaterialResource* R)
        { FString S(TEXT("Slope isolation shader errors:")); for (const FString& E : R->GetCompileErrors()) S += TEXT("\n") + E; return S; }
        bool CheckIdentity(FString& Error) const
        {
            if (!bActive || !World.IsValid() || !Actual.IsValid() || !Master.IsValid() || !Template.IsValid() || !Function.IsValid()
                || Actual->Parent.Get() != Template.Get() || Actual->GetMaterial() != Master.Get()
                || !IsValid(Blend) || !Blend->IsIn(Function.Get()) || Blend->Code != (bNative ? FString(TEXT("return NativeSlope;")) : OriginalCode())
                || !Settings || Settings->bAutoSaveEnable || Settings->bAutoSaveMaps || Settings->bAutoSaveContent
                || FCoreUObjectDelegates::IsPackageOKToSaveDelegate.GetHandle() != SaveVetoHandle)
            { Error = TEXT("Slope isolation identity/Code/save-veto/autosave ownership changed"); return false; }
            return CheckLegacyPins(false, Error);
        }
        TStrongObjectPtr<UMaterial> Master;
        TStrongObjectPtr<UMaterialInstance> Template;
        TStrongObjectPtr<UMaterialFunction> Function;
        TStrongObjectPtr<UMaterialInstanceDynamic> Actual;
        TWeakObjectPtr<UWorld> World;
        UMaterialExpressionCustom* Blend = nullptr;
        UMaterialExpressionMaterialFunctionCall* LegacyCall = nullptr;
        UEditorLoadingSavingSettings* Settings = nullptr;
        FCoreUObjectDelegates::FIsPackageOKToSaveDelegate PriorSaveVeto;
        FDelegateHandle SaveVetoHandle;
        FRenderCommandFence Fence;
        FMaterialResource* Submitted = nullptr;
        ERHIFeatureLevel::Type FeatureLevel = ERHIFeatureLevel::Num;
        double Started = 0.0;
        bool bNative = false, bActive = false, bPrepared = false, bFailed = false, bFence = false;
        bool bReleaseAttempted = false, bRestored = false, SavedAuto = false, SavedMaps = false, SavedContent = false;
    };
}
#endif
