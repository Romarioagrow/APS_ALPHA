#pragma once

#if WITH_EDITOR
#include "CoreMinimal.h"
#include "APS_ALPHA/Tests/APSContinuousWarpPixelAssets.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

// Read-only, fresh-commandlet diagnosis of the immutable 10:29 V2 packages.
// No export/archive traversal: EditorOnlyData::Serialize itself changes its
// owner's active pointer. Normal LoadObject/PostLoad is the only lifecycle work.
namespace APSContinuousWarpColdAudit
{
    // This forensic audit stays pinned to the failed 10:29 V2, independently
    // of the builder/probe's next diagnostic destination.
    inline constexpr const TCHAR* DestinationV2 = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousWarpPixel20261003V2");
    inline constexpr const TCHAR* MasterPathV2 = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousWarpPixel20261003V2/M_APS_ContinuousWarpPixel.M_APS_ContinuousWarpPixel");
    inline constexpr const TCHAR* TemplatePathV2 = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousWarpPixel20261003V2/MI_APS_ContinuousWarpPixel.MI_APS_ContinuousWarpPixel");
    struct FCandidate { const TCHAR* Name; const TCHAR* SHA1; };
    inline constexpr FCandidate Candidates[] = {
        {TEXT("MF_APS_ContinuousWarpPixelWAT"), TEXT("26C0D0D32B8C7DA36B1D4EC6E7BFB49FDD34186E")},
        {TEXT("MF_APS_ContinuousWarpPixelPlanetMap"), TEXT("DB56F835FF7B1BCCAE922035D6C1A162FEB5B33C")},
        {TEXT("MF_APS_ContinuousWarpPixelSlope"), TEXT("1219CCDB6DDD5172CA1B12E144278FE6EB715506")},
        {TEXT("MF_APS_ContinuousWarpPixelOrbital"), TEXT("6E3320603F30211690AF091F2EAEFD088FC7E216")},
        {TEXT("MF_APS_ContinuousWarpPixelNormalCoordinates"), TEXT("E2B081073CBE06B19F8D1FFA67D3E9C51186DEBB")},
        {TEXT("M_APS_ContinuousWarpPixel"), TEXT("D6AEB2601BF772299588741CCE1CCAC5921EA760")},
        {TEXT("MI_APS_ContinuousWarpPixel"), TEXT("2B170E8BBBFD5EC212F2CBF9E75F85C0773805E8")}
    };

    inline FString Package(const FCandidate& Entry)
    { return FString(DestinationV2) / Entry.Name; }

    inline bool Guard(const TCHAR* Stage)
    {
        bool Good = true;
        const auto Check = [&](const FString& Path, const TCHAR* Expected)
        {
            const FString Actual = APSContinuousWarpPixelAssets::Hash(Path);
            const UPackage* Loaded = FindPackage(nullptr, *Path);
            const bool Dirty = Loaded && Loaded->IsDirty();
            const bool Match = Actual == Expected;
            UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.Hash] stage=%s package=%s SHA1=%s expected=%s match=%d dirty=%d"),
                Stage, *Path, *Actual, Expected, Match, Dirty);
            Good &= Match && !Dirty;
        };
        for (const auto& Entry : Candidates) Check(Package(Entry), Entry.SHA1);
        for (const auto& Entry : APSContinuousWarpPixelAssets::Sources) Check(Entry.Package, Entry.SHA1);
        if (!Good) UE_LOG(LogTemp, Error, TEXT("[APS.WarpColdAudit] stage=%s immutable hash/dirty guard failed; no repair or save attempted"), Stage);
        return Good;
    }

    inline bool Snapshot(UObject* Owner, const TCHAR* Stage, int32& PinFindings)
    {
        auto* Function = Cast<UMaterialFunction>(Owner);
        auto* Material = Cast<UMaterial>(Owner);
        if (!Function && !Material) return false;
        const auto& Active = Function ? Function->GetExpressionCollection() : Material->GetExpressionCollection();
        const UObject* ActiveData = Function ? static_cast<UObject*>(Function->GetEditorOnlyData())
            : static_cast<UObject*>(Material->GetEditorOnlyData());
        const FString ActivePath = GetPathNameSafe(ActiveData);
        TArray<UMaterialExpression*> Pending;
        for (UMaterialExpression* E : Active.Expressions) if (E) Pending.Add(E);
        if (Material)
            for (int32 Property = 0; Property < MP_MAX; ++Property)
                if (const auto* Input = Material->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Property)); Input && Input->Expression)
                    Pending.Add(Input->Expression);
        TSet<UMaterialExpression*> Reachable;
        for (int32 Index = 0; Index < Pending.Num(); ++Index)
        {
            UMaterialExpression* E = Pending[Index];
            if (Reachable.Contains(E)) continue;
            if (Reachable.Num() >= 4096 || !E->IsIn(Owner))
            {
                UE_LOG(LogTemp, Error, TEXT("[APS.WarpColdAudit] graph ownership/bound failed owner=%s expression=%s"), *Owner->GetPathName(), *E->GetPathName());
                return false;
            }
            Reachable.Add(E);
            for (const FExpressionInput* Input : E->GetInputsView())
                if (Input && Input->Expression && !Reachable.Contains(Input->Expression)) Pending.Add(Input->Expression);
        }
        TArray<UObject*> Owned;
        GetObjectsWithOuter(Owner, Owned, true);
        if (Owned.Num() > 8192) return false;
        Owned.Sort([](const UObject& A, const UObject& B) { return A.GetPathName() < B.GetPathName(); });
        int32 Siblings = 0, Missing = 0, Calls = 0;
        for (UMaterialExpression* E : Reachable) Missing += !Active.Expressions.Contains(E);
        UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.Owner] stage=%s owner=%s active=%s registered=%d reachable=%d missing=%d owned=%d"),
            Stage, *Owner->GetPathName(), *ActivePath, Active.Expressions.Num(), Reachable.Num(), Missing, Owned.Num());
        for (UObject* Child : Owned)
            if (auto* Data = Cast<UMaterialFunctionEditorOnlyData>(Child); Data && Data->GetOuter() == Owner)
            {
                ++Siblings;
                bool HasFarSlope = false;
                int32 MissingReachable = 0;
                for (UMaterialExpression* E : Reachable) MissingReachable += !Data->ExpressionCollection.Expressions.Contains(E);
                for (UMaterialExpression* E : Data->ExpressionCollection.Expressions)
                    HasFarSlope |= E && E->GetFName() == FName(TEXT("APS_FarSlopePixel_1"));
                UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.EOD] stage=%s owner=%s object=%s active=%d registered=%d missingReachable=%d farSlopePixelMember=%d flags=%u"),
                    Stage, *Owner->GetPathName(), *Data->GetPathName(), Data == ActiveData,
                    Data->ExpressionCollection.Expressions.Num(), MissingReachable, HasFarSlope, uint32(Data->GetFlags()));
                for (int32 Index = 0; Index < Data->ExpressionCollection.Expressions.Num(); ++Index)
                {
                    UMaterialExpression* E = Data->ExpressionCollection.Expressions[Index];
                    UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.Member] stage=%s eod=%s index=%d node=%s guid=%s reachable=%d owned=%d"),
                        Stage, *Data->GetPathName(), Index, *GetPathNameSafe(E),
                        E ? *E->MaterialExpressionGuid.ToString() : TEXT("None"), E && Reachable.Contains(E), E && E->IsIn(Owner));
                }
            }
        for (UObject* Child : Owned)
            if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Child))
            {
                ++Calls;
                TArray<FFunctionExpressionInput> ExpectedInputs;
                TArray<FFunctionExpressionOutput> ExpectedOutputs;
                // Pure interface enumeration, never UpdateFromFunctionResource.
                if (Call->MaterialFunction) Call->MaterialFunction->GetInputsAndOutputs(ExpectedInputs, ExpectedOutputs);
                const bool CountsMatch = Call->FunctionInputs.Num() == ExpectedInputs.Num() && Call->FunctionOutputs.Num() == ExpectedOutputs.Num();
                PinFindings += !Call->MaterialFunction || !CountsMatch;
                UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.Call] stage=%s node=%s guid=%s callee=%s activeMember=%d reachable=%d inputs=%d expectedInputs=%d outputs=%d expectedOutputs=%d countsMatch=%d farSlopePixel=%d"),
                    Stage, *Call->GetPathName(), *Call->MaterialExpressionGuid.ToString(), *GetPathNameSafe(Call->MaterialFunction),
                    Active.Expressions.Contains(Call), Reachable.Contains(Call), Call->FunctionInputs.Num(), ExpectedInputs.Num(),
                    Call->FunctionOutputs.Num(), ExpectedOutputs.Num(), CountsMatch, Call->GetFName() == FName(TEXT("APS_FarSlopePixel_1")));
                for (int32 Index = 0; Index < Call->FunctionInputs.Num(); ++Index)
                {
                    const auto& Pin = Call->FunctionInputs[Index];
                    const auto* Expected = ExpectedInputs.FindByPredicate([&](const FFunctionExpressionInput& P) { return P.ExpressionInputId == Pin.ExpressionInputId; });
                    const bool Match = Expected && Pin.ExpressionInput && Pin.ExpressionInput == Expected->ExpressionInput && Pin.Input.InputName == Expected->Input.InputName;
                    PinFindings += !Match;
                    UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.Input] stage=%s call=%s index=%d guid=%s name=%s transient=%s expected=%s match=%d edge=%s output=%d mask=%d rgba=%d%d%d%d"),
                        Stage, *Call->GetPathName(), Index, *Pin.ExpressionInputId.ToString(), *Pin.Input.InputName.ToString(),
                        *GetPathNameSafe(Pin.ExpressionInput), *GetPathNameSafe(Expected ? Expected->ExpressionInput.Get() : nullptr), Match,
                        *GetPathNameSafe(Pin.Input.Expression), Pin.Input.OutputIndex, Pin.Input.Mask, Pin.Input.MaskR, Pin.Input.MaskG, Pin.Input.MaskB, Pin.Input.MaskA);
                }
                for (int32 Index = 0; Index < Call->FunctionOutputs.Num(); ++Index)
                {
                    const auto& Pin = Call->FunctionOutputs[Index];
                    const auto* Expected = ExpectedOutputs.FindByPredicate([&](const FFunctionExpressionOutput& P) { return P.ExpressionOutputId == Pin.ExpressionOutputId; });
                    const bool Match = Expected && Pin.ExpressionOutput && Pin.ExpressionOutput == Expected->ExpressionOutput && Pin.Output.OutputName == Expected->Output.OutputName;
                    PinFindings += !Match;
                    UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.Output] stage=%s call=%s index=%d guid=%s name=%s transient=%s expected=%s match=%d"),
                        Stage, *Call->GetPathName(), Index, *Pin.ExpressionOutputId.ToString(), *Pin.Output.OutputName.ToString(),
                        *GetPathNameSafe(Pin.ExpressionOutput), *GetPathNameSafe(Expected ? Expected->ExpressionOutput.Get() : nullptr), Match);
                }
            }
        const UObject* ActiveAfter = Function ? static_cast<UObject*>(Function->GetEditorOnlyData())
            : static_cast<UObject*>(Material->GetEditorOnlyData());
        UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.OwnerEnd] stage=%s owner=%s eodSiblings=%d calls=%d activeStable=%d activeAfter=%s"),
            Stage, *Owner->GetPathName(), Siblings, Calls, ActiveData == ActiveAfter, *GetPathNameSafe(ActiveAfter));
        return ActiveData == ActiveAfter;
    }

    inline bool Run()
    {
        if (!IsRunningCommandlet())
        { UE_LOG(LogTemp, Error, TEXT("[APS.WarpColdAudit] Refused: fresh commandlet only")); return false; }
        if (!Guard(TEXT("before"))) return false;
        for (const auto& Entry : Candidates)
        {
            const FString Path = Package(Entry) + TEXT(".") + Entry.Name;
            if (FindObject<UObject>(nullptr, *Path))
            { UE_LOG(LogTemp, Error, TEXT("[APS.WarpColdAudit] Refused: candidate already loaded %s"), *Path); return false; }
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit] BEGIN runtime MIC-first LoadObject<UMaterialInstance>; log-only; no export/archive, force compile, pin repair, update, dirty write or save"));
        bool Complete = true;
        // Match the runtime probe's cold entry exactly. Preloading the five
        // functions would change PostLoad ordering and could hide the defect.
        auto* Instance = LoadObject<UMaterialInstance>(nullptr, TemplatePathV2);
        auto* Master = FindObject<UMaterial>(nullptr, MasterPathV2);
        const bool ParentMatch = Master && Instance && Instance->Parent == Master;
        Complete &= ParentMatch;
        UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit.Binding] master=%s instance=%s parent=%s parentMatch=%d"),
            *GetPathNameSafe(Master), *GetPathNameSafe(Instance), *GetPathNameSafe(Instance ? Instance->Parent.Get() : nullptr), ParentMatch);
        TArray<UMaterialFunction*> Functions;
        TArray<FString> MissingFunctions;
        for (int32 Index = 0; Index < 5; ++Index)
        {
            const FString Path = Package(Candidates[Index]) + TEXT(".") + Candidates[Index].Name;
            auto* Function = FindObject<UMaterialFunction>(nullptr, *Path);
            if (Function) Functions.Add(Function);
            else
            {
                MissingFunctions.Add(Path);
                UE_LOG(LogTemp, Warning, TEXT("[APS.WarpColdAudit] stage=runtime-mic-first functionNotLoaded=%s; no load attempted before first snapshot"), *Path);
            }
        }
        int32 RuntimePins = 0, SecondPins = 0;
        for (auto* Function : Functions) Complete &= Snapshot(Function, TEXT("runtime-mic-first"), RuntimePins);
        if (Master) Complete &= Snapshot(Master, TEXT("runtime-mic-first"), RuntimePins);
        UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit] runtimeSnapshotComplete functions=%d missing=%d pinFindings=%d; explicit function loads only begin below"),
            Functions.Num(), MissingFunctions.Num(), RuntimePins);
        for (const FString& Path : MissingFunctions)
        {
            auto* Function = LoadObject<UMaterialFunction>(nullptr, *Path);
            if (Function) Functions.Add(Function);
            else { UE_LOG(LogTemp, Error, TEXT("[APS.WarpColdAudit] Could not load %s"), *Path); Complete = false; }
        }
        for (auto* Function : Functions) Complete &= Snapshot(Function, TEXT("after-missing-function-loads"), SecondPins);
        if (Master) Complete &= Snapshot(Master, TEXT("after-missing-function-loads"), SecondPins);
        const bool Protected = Guard(TEXT("after")); // Always after every attempted load/snapshot.
        UE_LOG(LogTemp, Display, TEXT("[APS.WarpColdAudit] END auditComplete=%d protected=%d functions=%d explicitFunctionLoads=%d runtimePinFindings=%d secondPinFindings=%d; findings are evidence, not shader/visual acceptance"),
            Complete, Protected, Functions.Num(), MissingFunctions.Num(), RuntimePins, SecondPins);
        // Success means evidence collection completed without touching packages,
        // not that the broken candidate passed a material correctness gate.
        return Complete && Protected;
    }
}
#endif
