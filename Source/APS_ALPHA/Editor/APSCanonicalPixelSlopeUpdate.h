#pragma once
#if WITH_EDITOR
#include "APSPlanetReliefNormalUpdate.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "UObject/UObjectHash.h"

// Offline in-memory transform only. Caller owns exact file hashes/backups,
// transient call-pin hydration, compile, save and independent cold-load checks.
// A false result after a postcondition failure must NEVER be compiled/saved.
namespace APSCanonicalPixelSlopeUpdate
{
    inline constexpr const TCHAR* InputName = TEXT("CanonicalPixelSlopeNormal");
    inline constexpr const TCHAR* SlopePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_SlopeBlock_f6be5407.MF_APS_MF_SlopeBlock_f6be5407");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.M_APS_ContinuousTerrain");
    namespace Private
    {
        inline bool GuidIs(const FGuid& Id, const TCHAR* Text)
        { FGuid Expected; return FGuid::Parse(Text, Expected) && Id == Expected; }
        inline bool SameOutput(const FExpressionOutput& A, const FExpressionOutput& B)
        {
            return A.OutputName == B.OutputName && A.Mask == B.Mask && A.MaskR == B.MaskR
                && A.MaskG == B.MaskG && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
        }
        inline const TCHAR* BlendCode()
        {
            return TEXT("float d=length(CameraDelta.xyz)*max(InverseScale.x,0.0);\n")
                TEXT("float t=saturate((d-StartCm)/max(EndCm-StartCm,1.0));\n")
                TEXT("if(t<=0.0) return NativeSlope;\nif(t>=1.0) return PixelSlope;\n")
                TEXT("float w=t*t*(3.0-2.0*t);\nreturn lerp(NativeSlope,PixelSlope,w);");
        }
        inline const TCHAR* SelectCode()
        {
            return TEXT("if(!(Canonical.w>0.0)) return Old.xyz;\n")
                TEXT("if(!all(isfinite(Canonical))) return Old.xyz;\n")
                TEXT("float3 n=lerp(Old.xyz,Canonical.xyz,saturate(Canonical.w));\n")
                TEXT("float q=dot(n,n);\nif(!(q>1.e-12)) return Old.xyz;\nreturn n*rsqrt(q);");
        }
        inline FExpressionInput* Named(UMaterialExpressionCustom* Node, const TCHAR* Name)
        {
            FExpressionInput* Result = nullptr;
            if (Node) for (auto& Input : Node->Inputs) if (Input.InputName == Name)
            { if (Result) return nullptr; Result = &Input.Input; }
            return Result;
        }
        inline bool Closure(UObject* Owner, TArray<UMaterialExpression*>& Graph, FString& Error)
        {
            TSet<UMaterialExpression*> Seen;
            for (int32 I = 0; I < Graph.Num(); ++I)
            {
                auto* E = Graph[I];
                if (!IsValid(E) || E->GetOuter() != Owner || Graph.Num() > 512)
                { Error = TEXT("Pixel slope closure contains foreign/missing nodes or exceeds bound"); return false; }
                if (Seen.Contains(E)) continue;
                Seen.Add(E);
                for (auto* Input : E->GetInputsView()) if (Input && Input->Expression)
                    Graph.AddUnique(Input->Expression);
            }
            return true;
        }
        struct FWire { FExpressionInput* Pointer; FExpressionInput Value; };
        struct FEOD { UMaterialFunctionEditorOnlyData* Data; FMaterialExpressionCollection Before; };
    }

    // CanonicalWorldNormal is FLOAT4: XYZ world normal, W combined availability.
    // W==0 selects this function's exact old filter, not a master approximation.
    // It is evaluated only by the far pixel-slope branch, never NativeVI.
    inline bool Patch(UMaterial* Master, UMaterialFunction* Slope,
        const FExpressionInput& CanonicalWorldNormal, FString& Error)
    {
        using APSPlanetReliefNormalUpdate::Private::Same;
        using APSPlanetReliefNormalUpdate::Private::Whole;
        const auto Fail = [&](const FString& Why) { Error = Why; return false; };
        if (!Error.IsEmpty()) return false;
        if (!IsRunningCommandlet() || !IsInGameThread() || !IsValid(Master) || !IsValid(Slope)
            || Master->GetPathName() != MasterPath || Slope->GetPathName() != SlopePath
            || Master->bTangentSpaceNormal || Master->bUseMaterialAttributes || Master->MaterialDomain != MD_Surface
            || !Whole(CanonicalWorldNormal) || CanonicalWorldNormal.Expression->GetOuter() != Master)
            return Fail(TEXT("Pixel slope patch requires the exact Continuous master/source Slope in an owned offline commandlet"));

        TSet<UMaterialExpression*> CanonicalTree;
        if (!APSPlanetReliefNormalUpdate::Private::Tree(Master, CanonicalWorldNormal.Expression,
            CanonicalTree, true, Error)) return false;
        int32 CoverageSamples = 0;
        for (auto* E : CanonicalTree) if (const auto* C = Cast<UMaterialExpressionCustom>(E))
            CoverageSamples += C->Description == TEXT("APS canonical stereo coverage sample v1");
        if (CoverageSamples != 1) return Fail(TEXT("Supplied pixel normal must depend on the one existing coverage sample"));

        TArray<Private::FEOD> Collections;
        TArray<UObject*> Children; GetObjectsWithOuter(Slope, Children, false);
        const UClass* EODBase = UMaterialFunctionEditorOnlyData::StaticClass()->GetSuperClass(); // base is NO_API
        for (UObject* Child : Children) if (Child && Child->IsA(EODBase))
        {
            if (Child->GetClass() != UMaterialFunctionEditorOnlyData::StaticClass()
                || (Child->GetFName() != TEXT("MF_APS_MF_SlopeBlock_f6be5407EditorOnlyData")
                    && Child->GetFName() != TEXT("MF_SlopeBlockEditorOnlyData")))
                return Fail(TEXT("Unaudited Slope EditorOnlyData sibling"));
            auto* Data = CastChecked<UMaterialFunctionEditorOnlyData>(Child);
            Collections.Add({Data, Data->ExpressionCollection});
        }
        if (Collections.Num() != 2 || !Collections.ContainsByPredicate([&](const Private::FEOD& C)
            { return C.Data == Slope->GetEditorOnlyData(); }))
            return Fail(TEXT("Slope must retain its exact canonical/source-named EOD pair"));
        TArray<UMaterialExpression*> Graph;
        for (UMaterialExpression* E : Slope->GetExpressions()) Graph.AddUnique(E);
        for (const auto& C : Collections)
            for (UMaterialExpression* E : C.Before.Expressions) Graph.AddUnique(E);
        if (!Private::Closure(Slope, Graph, Error)) return false;
        if (Graph.Num() != 143) return Fail(FString::Printf(TEXT("Audited Slope closure changed: expected143, got%d; do not relax this guard"), Graph.Num()));

        UMaterialExpressionCustom *Filter = nullptr, *Blend = nullptr;
        UMaterialExpressionVertexInterpolator* NativeVI = nullptr;
        TMap<FName, UMaterialExpression*> ByName;
        int32 FarClones = 0, Interpolators = 0;
        TArray<Private::FWire> Wires;
        for (auto* E : Graph)
        {
            if (ByName.Contains(E->GetFName())) return Fail(TEXT("Duplicate Slope node name"));
            ByName.Add(E->GetFName(), E);
            FarClones += E->GetName().StartsWith(TEXT("APS_FarSlopePixel_"));
            if (auto* I = Cast<UMaterialExpressionFunctionInput>(E); I && I->InputName == InputName)
                return Fail(TEXT("Canonical pixel-slope input already exists; refuse repeated patch"));
            if (auto* V = Cast<UMaterialExpressionVertexInterpolator>(E)) { NativeVI = V; ++Interpolators; }
            if (auto* C = Cast<UMaterialExpressionCustom>(E))
            {
                if (C->Description == TEXT("APS physical-distance geometric normal continuity v1"))
                { if (Filter) return Fail(TEXT("Ambiguous Slope normal filter")); Filter = C; }
                if (C->Description == TEXT("APS preserve native near slope; smooth far pixel slope v1"))
                { if (Blend) return Fail(TEXT("Ambiguous native/pixel slope blend")); Blend = C; }
            }
            for (auto* Input : E->GetInputsView()) if (Input) Wires.Add({Input, *Input});
        }
        if (FarClones != 9 || Interpolators != 1 || !NativeVI || !Whole(NativeVI->Input)
            || !Filter || Filter->Code != APSPlanetReliefNormalUpdate::Private::BaselineCode()
            || Filter->OutputType != CMOT_Float3 || Filter->Inputs.Num() != 6
            || !Blend || Blend->Code != Private::BlendCode() || Blend->OutputType != CMOT_Float1 || Blend->Inputs.Num() != 6)
            return Fail(TEXT("Exact nine-clone/native-VI/filter/blend contract changed"));
        const TCHAR* FilterNames[] = {TEXT("NativeNormal"),TEXT("Relative"),TEXT("CameraDelta"),TEXT("InverseScale"),TEXT("StartCm"),TEXT("EndCm")};
        const TCHAR* BlendNames[] = {TEXT("NativeSlope"),TEXT("PixelSlope"),TEXT("CameraDelta"),TEXT("InverseScale"),TEXT("StartCm"),TEXT("EndCm")};
        for (int32 I = 0; I < 6; ++I)
            if (Filter->Inputs[I].InputName != FilterNames[I] || !Whole(Filter->Inputs[I].Input)
                || Blend->Inputs[I].InputName != BlendNames[I] || !Whole(Blend->Inputs[I].Input))
                return Fail(TEXT("Exact filter/blend input mapping changed"));
        if (!Cast<UMaterialExpressionVertexNormalWS>(Filter->Inputs[0].Input.Expression)
            || Blend->Inputs[0].Input.Expression != NativeVI
            || Blend->Inputs[1].Input.Expression != ByName.FindRef(TEXT("APS_FarSlopePixel_0")))
            return Fail(TEXT("Native VI or far pixel-clamp linkage changed"));
        const TCHAR* CloneTypes[] = {TEXT("MaterialExpressionClamp"),TEXT("MaterialExpressionMaterialFunctionCall"),
            TEXT("MaterialExpressionStaticSwitch"),TEXT("MaterialExpressionAdd"),TEXT("MaterialExpressionDotProduct"),
            TEXT("MaterialExpressionCustom"),TEXT("MaterialExpressionAdd"),TEXT("MaterialExpressionDotProduct"),TEXT("MaterialExpressionCustom")};
        for (int32 I = 0; I < 9; ++I)
        {
            auto* E = ByName.FindRef(*FString::Printf(TEXT("APS_FarSlopePixel_%d"), I));
            if (!E || E->GetClass()->GetName() != CloneTypes[I] || !E->MaterialExpressionGuid.IsValid())
                return Fail(TEXT("Audited nine pixel clone classes/GUIDs changed"));
        }
        FExpressionInput* Targets[2] = {};
        for (int32 I = 0; I < 2; ++I)
        {
            auto* A = Cast<UMaterialExpressionCustom>(ByName.FindRef(I ? TEXT("APS_FarSlopePixel_8") : TEXT("APS_FarSlopePixel_5")));
            if (!A || A->Description != TEXT("APS rendered vector to planet frame")
                || A->Code != TEXT("return float3(dot(V.xyz,X.xyz),dot(V.xyz,Y.xyz),dot(V.xyz,Z.xyz));"))
                return Fail(TEXT("Far pixel normal adapter math changed"));
            Targets[I] = Private::Named(A, TEXT("V"));
            if (!Targets[I] || !Whole(*Targets[I]) || Targets[I]->Expression != Filter)
                return Fail(TEXT("Far pixel adapter no longer reads the exact old filter"));
        }
        int32 FilterConsumers = 0;
        for (const auto& W : Wires) if (W.Value.Expression == Filter)
        { ++FilterConsumers; if (W.Pointer != Targets[0] && W.Pointer != Targets[1]) return Fail(TEXT("Filter gained another consumer")); }
        if (FilterConsumers != 2) return Fail(TEXT("Expected exactly two far filter consumers"));
        TArray<UMaterialExpression*> NativeTree{NativeVI};
        if (!Private::Closure(Slope, NativeTree, Error)) return false;
        for (auto* E : NativeTree) if (E == Filter || E == Blend || E->GetName().StartsWith(TEXT("APS_FarSlopePixel_")))
            return Fail(TEXT("Native VI unexpectedly reaches the pixel/filter graph"));

        TArray<FFunctionExpressionInput> InterfaceInputs;
        TArray<FFunctionExpressionOutput> InterfaceOutputs;
        Slope->GetInputsAndOutputs(InterfaceInputs, InterfaceOutputs);
        if (InterfaceInputs.Num() != 1 || InterfaceOutputs.Num() != 2
            || !Private::GuidIs(InterfaceInputs[0].ExpressionInputId,TEXT("52D8BCF148FA350531FB39862F24D6CF"))
            || !InterfaceInputs[0].ExpressionInput || InterfaceInputs[0].ExpressionInput->InputName != TEXT("In"))
            return Fail(TEXT("Audited Slope input interface changed"));
        TSet<FGuid> OutputIds;
        for (const auto& O : InterfaceOutputs)
        {
            if (!O.ExpressionOutput || OutputIds.Contains(O.ExpressionOutputId)
                || (!Private::GuidIs(O.ExpressionOutputId,TEXT("4E870CE3478B3E536B80D4BBCD8A0F22"))
                    && !Private::GuidIs(O.ExpressionOutputId,TEXT("F1D68DC34E67CE57967C3D8967D51EFB"))))
                return Fail(TEXT("Audited Slope output GUIDs changed"));
            OutputIds.Add(O.ExpressionOutputId);
        }
        UMaterialExpressionMaterialFunctionCall* Call = nullptr;
        TArray<UObject*> MasterChildren; GetObjectsWithOuter(Master, MasterChildren, false);
        for (UObject* E : MasterChildren) if (auto* C = Cast<UMaterialExpressionMaterialFunctionCall>(E); C && C->MaterialFunction == Slope)
        { if (Call) return Fail(TEXT("Continuous has multiple Slope callers")); Call = C; }
        if (!Call || Call->FunctionInputs.Num() != 1 || Call->FunctionOutputs.Num() != 2 || Call->Outputs.Num() != 2
            || Call->FunctionInputs[0].ExpressionInputId != InterfaceInputs[0].ExpressionInputId
            || Call->FunctionInputs[0].Input.InputName != TEXT("In") || !Call->FunctionInputs[0].Input.Expression)
            return Fail(TEXT("Exact Continuous Slope caller pins changed"));
        const auto OldInputs = Call->FunctionInputs;
        const auto OldOutputs = Call->FunctionOutputs;
        const auto OldDisplayOutputs = Call->Outputs;
        TSet<FGuid> CallerOutputIds;
        for (int32 I = 0; I < OldOutputs.Num(); ++I)
        {
            const auto& O = OldOutputs[I];
            if (!OutputIds.Contains(O.ExpressionOutputId) || CallerOutputIds.Contains(O.ExpressionOutputId)
                || !Private::SameOutput(O.Output, OldDisplayOutputs[I])) return Fail(TEXT("Caller output ordering/mapping changed"));
            CallerOutputIds.Add(O.ExpressionOutputId);
        }
        // ALL source, closure, stage and pin checks precede the first mutation.
        auto* Optional = NewObject<UMaterialExpressionFunctionInput>(Slope, NAME_None, RF_Transactional);
        Optional->Function = Slope; Optional->InputName = InputName; Optional->InputType = FunctionInput_Vector4;
        Optional->Description = TEXT("Optional Continuous-only canonical pixel slope; untouched callers retain the original normal filter");
        Optional->Id = FGuid::NewGuid(); Optional->MaterialExpressionGuid = FGuid::NewGuid();
        Optional->SortPriority = 1000000; Optional->bUsePreviewValueAsDefault = true;
        Optional->PreviewValue = FVector4f(0,0,1,0);
        auto* Select = NewObject<UMaterialExpressionCustom>(Slope, NAME_None, RF_Transactional);
        Select->Function = Slope; Select->MaterialExpressionGuid = FGuid::NewGuid();
        Select->Description = TEXT("APS optional canonical pixel slope: exact local fallback v1");
        Select->OutputType = CMOT_Float3;
        Select->Code = Private::SelectCode();
        Select->Outputs.Reset(); Select->Outputs.Add(FExpressionOutput());
        Select->Inputs.Reset();
        FCustomInput Old; Old.InputName = TEXT("Old"); Old.Input = *Targets[0]; Select->Inputs.Add(Old);
        FCustomInput Canonical; Canonical.InputName = TEXT("Canonical"); Canonical.Input.Expression = Optional;
        Select->Inputs.Add(Canonical);
        Graph.Add(Optional); Graph.Add(Select);
        for (auto& C : Collections)
        {
            // One common complete order prevents the source-named sibling from
            // restoring a stale47-node list at cold PostLoad. Keep comments/exec.
            C.Data->ExpressionCollection.Expressions.Reset(Graph.Num());
            for (auto* E : Graph) C.Data->ExpressionCollection.Expressions.Add(E);
        }
        for (auto* Target : Targets) Target->Expression = Select; // Exactly two old wires.

        // Do NOT call UpdateFromFunctionResource: it recursively updates shared
        // callees and fixes up unrelated consumers. Hydrate this caller explicitly
        // by GUID while retaining serialized old pin order/links/output masks.
        Call->FunctionInputs[0].ExpressionInput = InterfaceInputs[0].ExpressionInput;
        FFunctionExpressionInput Added; Added.ExpressionInput = Optional; Added.ExpressionInputId = Optional->Id;
        Added.Input = CanonicalWorldNormal; Added.Input.InputName = InputName;
        Call->FunctionInputs.Add(Added);
        for (auto& O : Call->FunctionOutputs)
            for (const auto& Known : InterfaceOutputs) if (Known.ExpressionOutputId == O.ExpressionOutputId)
                O.ExpressionOutput = Known.ExpressionOutput;

        for (const auto& W : Wires)
        {
            FExpressionInput Expected = W.Value;
            if (W.Pointer == Targets[0] || W.Pointer == Targets[1]) Expected.Expression = Select;
            if (!Same(*W.Pointer, Expected)) return Fail(TEXT("POSTCONDITION: unintended source wire change; refuse save"));
        }
        if (!Same(Call->FunctionInputs[0].Input, OldInputs[0].Input)
            || Call->FunctionInputs[0].ExpressionInputId != OldInputs[0].ExpressionInputId
            || Call->FunctionOutputs.Num() != OldOutputs.Num() || Call->Outputs.Num() != OldDisplayOutputs.Num())
            return Fail(TEXT("POSTCONDITION: original caller pins changed; refuse save"));
        for (int32 I = 0; I < OldOutputs.Num(); ++I)
            if (Call->FunctionOutputs[I].ExpressionOutputId != OldOutputs[I].ExpressionOutputId
                || !Private::SameOutput(Call->FunctionOutputs[I].Output, OldOutputs[I].Output)
                || !Private::SameOutput(Call->Outputs[I], OldDisplayOutputs[I]))
                return Fail(TEXT("POSTCONDITION: original caller output order changed; refuse save"));
        TArray<FFunctionExpressionInput> NewInputs; TArray<FFunctionExpressionOutput> NewOutputs;
        Slope->GetInputsAndOutputs(NewInputs, NewOutputs);
        if (NewInputs.Num() != 2 || NewOutputs.Num() != 2 || !NewInputs.ContainsByPredicate([&](const FFunctionExpressionInput& I)
            { return I.ExpressionInputId == Optional->Id && I.ExpressionInput == Optional; }))
            return Fail(TEXT("POSTCONDITION: optional function interface not registered; refuse save"));
        for (const auto& C : Collections)
        {
            const auto& A = C.Data->ExpressionCollection;
            if (A.Expressions.Num() != 145 || A.EditorComments != C.Before.EditorComments
                || A.ExpressionExecBegin != C.Before.ExpressionExecBegin || A.ExpressionExecEnd != C.Before.ExpressionExecEnd)
                return Fail(TEXT("POSTCONDITION: EOD metadata/closure changed; refuse save"));
            for (int32 I = 0; I < Graph.Num(); ++I) if (A.Expressions[I] != Graph[I])
                return Fail(TEXT("POSTCONDITION: EOD ordered parity failed; refuse save"));
            UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalPixelSlope] eod=%s before=%d after=145 orderedParity=1 commentsExecUnchanged=1"),
                *C.Data->GetPathName(),C.Before.Expressions.Num());
        }
        Master->MarkPackageDirty(); Slope->MarkPackageDirty();
        UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalPixelSlope] PATCH master=%s function=%s newInput=%s id=%s changedFarNormalLinks=2 nativeVIUnchanged=1 oldCallerPinsByGuid=1 optionalFloat4DefaultW0ExactLocalFilter=1; no WAT/offset/palette/normal-output edits; compile/save/cold validation still required"),
            *Master->GetPathName(),*Slope->GetPathName(),InputName,*Optional->Id.ToString(EGuidFormats::Digits));
        return true;
    }

    // Read-only structural verification after an independent MIC-first cold
    // load. Does not hydrate any pin, update a function, compile, or save.
    // Caller still owns disk SHA and complete shader/LocalVF/compile-error gates.
    inline bool VerifyPatched(UMaterial* Master, UMaterialFunction* Slope, FString& Error)
    {
        using APSPlanetReliefNormalUpdate::Private::Whole;
        const auto Fail = [&](const TCHAR* Why) { Error = Why; return false; };
        if (!Error.IsEmpty()) return false;
        if (!IsInGameThread() || !IsRunningCommandlet() || !IsValid(Master) || !IsValid(Slope)
            || Master->GetPathName() != MasterPath || Slope->GetPathName() != SlopePath
            || Master->bTangentSpaceNormal || Master->bUseMaterialAttributes || Master->MaterialDomain != MD_Surface)
            return Fail(TEXT("Pixel slope verification requires exact cold-loaded Continuous/Slope commandlet objects"));
        TArray<UObject*> Children; GetObjectsWithOuter(Slope, Children, false);
        TArray<UMaterialFunctionEditorOnlyData*> Collections;
        const UClass* EODBase = UMaterialFunctionEditorOnlyData::StaticClass()->GetSuperClass();
        for (UObject* Child : Children) if (Child && Child->IsA(EODBase))
        {
            if (Child->GetClass() != UMaterialFunctionEditorOnlyData::StaticClass()
                || (Child->GetFName() != TEXT("MF_APS_MF_SlopeBlock_f6be5407EditorOnlyData")
                    && Child->GetFName() != TEXT("MF_SlopeBlockEditorOnlyData")))
                return Fail(TEXT("Cold Slope has an unaudited EOD sibling"));
            Collections.Add(CastChecked<UMaterialFunctionEditorOnlyData>(Child));
        }
        auto* Active=Slope->GetEditorOnlyData();
        FString Inventory;
        for (const auto* C:Collections)
            Inventory+=FString::Printf(TEXT(" %s:%d%s"),*C->GetName(),C->ExpressionCollection.Expressions.Num(),C==Active?TEXT("(active)"):TEXT(""));
        UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalPixelSlope] VERIFY EOD count=%d active=%s inventory=%s"),
            Collections.Num(),*GetPathNameSafe(Active),*Inventory);
        // UE PostLoad copies a source-named EOD into the canonical object and
        // retains only the latter through the function's UPROPERTY. The old
        // source sibling may legitimately disappear during editor compile GC.
        // Require the canonical active object; never retain/recreate the ghost.
        if ((Collections.Num()!=1 && Collections.Num()!=2) || !IsValid(Active)
            || Active->GetFName()!=TEXT("MF_APS_MF_SlopeBlock_f6be5407EditorOnlyData")
            || !Collections.Contains(Active))
            return Fail(TEXT("Cold Slope requires its canonical active EOD and at most the known source sibling"));
        const auto& A=Active->ExpressionCollection.Expressions;
        if (A.Num()!=145) return Fail(TEXT("Cold canonical Slope EOD closure is not145"));
        for (const auto* C:Collections)
        {
            const auto& B=C->ExpressionCollection.Expressions;
            if (B.Num()!=145) return Fail(TEXT("Cold surviving Slope EOD sibling closure is not145"));
            for (int32 I=0;I<A.Num();++I) if (A[I]!=B[I])
                return Fail(TEXT("Cold surviving Slope EOD sibling ordered parity failed"));
        }
        TArray<UMaterialExpression*> Graph;
        for (int32 I=0; I<A.Num(); ++I)
        {
            if (!A[I] || Graph.Contains(A[I])) return Fail(TEXT("Cold canonical Slope EOD uniqueness failed"));
            Graph.Add(A[I]);
        }
        if (!Private::Closure(Slope,Graph,Error)) return false;
        if (Graph.Num()!=145) return Fail(TEXT("Cold Slope still has unregistered reachable nodes"));
        UMaterialExpressionFunctionInput* Optional=nullptr;
        UMaterialExpressionCustom *Select=nullptr, *Filter=nullptr, *Blend=nullptr;
        UMaterialExpressionVertexInterpolator* NativeVI=nullptr;
        TMap<FName,UMaterialExpression*> ByName;
        int32 Interpolators=0,FarClones=0;
        for (UMaterialExpression* E:Graph)
        {
            ByName.Add(E->GetFName(),E);
            FarClones += E->GetName().StartsWith(TEXT("APS_FarSlopePixel_"));
            if (auto* I=Cast<UMaterialExpressionFunctionInput>(E); I && I->InputName==InputName)
            { if (Optional) return Fail(TEXT("Cold duplicate optional slope input")); Optional=I; }
            if (auto* V=Cast<UMaterialExpressionVertexInterpolator>(E)) { NativeVI=V; ++Interpolators; }
            if (auto* C=Cast<UMaterialExpressionCustom>(E))
            {
                if (C->Description==TEXT("APS optional canonical pixel slope: exact local fallback v1"))
                { if (Select) return Fail(TEXT("Cold duplicate pixel-slope selector")); Select=C; }
                if (C->Description==TEXT("APS physical-distance geometric normal continuity v1"))
                { if (Filter) return Fail(TEXT("Cold duplicate old normal filter")); Filter=C; }
                if (C->Description==TEXT("APS preserve native near slope; smooth far pixel slope v1"))
                { if (Blend) return Fail(TEXT("Cold duplicate native/far slope blend")); Blend=C; }
            }
        }
        if (!Optional || !Optional->Id.IsValid() || Optional->InputType!=FunctionInput_Vector4
            || !Optional->bUsePreviewValueAsDefault || Optional->Preview.Expression
            || Optional->PreviewValue!=FVector4f(0,0,1,0)
            || !Select || Select->Code!=Private::SelectCode() || Select->OutputType!=CMOT_Float3 || Select->Inputs.Num()!=2
            || !Filter || Filter->Code!=APSPlanetReliefNormalUpdate::Private::BaselineCode()
            || !Blend || Blend->Code!=Private::BlendCode() || Blend->Inputs.Num()!=6
            || Interpolators!=1 || FarClones!=9 || !NativeVI || !Whole(NativeVI->Input))
            return Fail(TEXT("Cold optional FLOAT4/default/local-filter/native-VI contract changed"));
        const auto* Old=Private::Named(Select,TEXT("Old"));
        const auto* Canonical=Private::Named(Select,TEXT("Canonical"));
        if (!Old || !Canonical || !Whole(*Old) || !Whole(*Canonical)
            || Old->Expression!=Filter || Canonical->Expression!=Optional
            || Blend->Inputs[0].Input.Expression!=NativeVI
            || Blend->Inputs[1].Input.Expression!=ByName.FindRef(TEXT("APS_FarSlopePixel_0")))
            return Fail(TEXT("Cold local fallback or existing slope blend links changed"));
        FExpressionInput* AdapterLinks[2]={};
        for (int32 I=0; I<2; ++I)
        {
            auto* Adapter=Cast<UMaterialExpressionCustom>(ByName.FindRef(I ? TEXT("APS_FarSlopePixel_8") : TEXT("APS_FarSlopePixel_5")));
            if (!Adapter || Adapter->Description!=TEXT("APS rendered vector to planet frame")
                || Adapter->Code!=TEXT("return float3(dot(V.xyz,X.xyz),dot(V.xyz,Y.xyz),dot(V.xyz,Z.xyz));"))
                return Fail(TEXT("Cold far-adapter math changed"));
            AdapterLinks[I]=Private::Named(Adapter,TEXT("V"));
            if (!AdapterLinks[I] || !Whole(*AdapterLinks[I]) || AdapterLinks[I]->Expression!=Select)
                return Fail(TEXT("Cold far-adapter selector link missing"));
        }
        int32 SelectConsumers=0;
        for (UMaterialExpression* E:Graph) for (FExpressionInput* I:E->GetInputsView()) if (I && I->Expression==Select)
        { ++SelectConsumers; if (I!=AdapterLinks[0] && I!=AdapterLinks[1]) return Fail(TEXT("Cold selector reaches an unaudited consumer")); }
        if (SelectConsumers!=2) return Fail(TEXT("Cold selector must have exactly two consumers"));
        TArray<UMaterialExpression*> NativeTree{NativeVI};
        if (!Private::Closure(Slope,NativeTree,Error)) return false;
        if (NativeTree.Contains(Optional) || NativeTree.Contains(Select) || NativeTree.Contains(Filter) || NativeTree.Contains(Blend))
            return Fail(TEXT("Cold native VI now depends on the pixel slope branch"));
        TArray<FFunctionExpressionInput> Inputs; TArray<FFunctionExpressionOutput> Outputs;
        Slope->GetInputsAndOutputs(Inputs,Outputs);
        if (Inputs.Num()!=2 || Outputs.Num()!=2
            || !Inputs.ContainsByPredicate([&](const FFunctionExpressionInput& I){return I.ExpressionInputId==Optional->Id && I.ExpressionInput==Optional;})
            || !Inputs.ContainsByPredicate([](const FFunctionExpressionInput& I){return Private::GuidIs(I.ExpressionInputId,TEXT("52D8BCF148FA350531FB39862F24D6CF"));}))
            return Fail(TEXT("Cold optional/original input GUID interface changed"));
        TSet<FGuid> Ids;
        for (const auto& O:Outputs)
        {
            if (Ids.Contains(O.ExpressionOutputId) || (!Private::GuidIs(O.ExpressionOutputId,TEXT("4E870CE3478B3E536B80D4BBCD8A0F22"))
                && !Private::GuidIs(O.ExpressionOutputId,TEXT("F1D68DC34E67CE57967C3D8967D51EFB"))))
                return Fail(TEXT("Cold original output GUIDs changed"));
            Ids.Add(O.ExpressionOutputId);
        }
        UMaterialExpressionMaterialFunctionCall* Call=nullptr;
        Children.Reset(); GetObjectsWithOuter(Master,Children,false);
        for (UObject* E:Children) if (auto* C=Cast<UMaterialExpressionMaterialFunctionCall>(E); C && C->MaterialFunction==Slope)
        { if (Call) return Fail(TEXT("Cold master has multiple Slope callers")); Call=C; }
        if (!Call || Call->FunctionInputs.Num()!=2 || Call->FunctionOutputs.Num()!=2 || Call->Outputs.Num()!=2
            || !Private::GuidIs(Call->FunctionInputs[0].ExpressionInputId,TEXT("52D8BCF148FA350531FB39862F24D6CF"))
            || !Call->FunctionInputs[0].Input.Expression
            || Call->FunctionInputs[1].ExpressionInputId!=Optional->Id || Call->FunctionInputs[1].ExpressionInput!=Optional
            || !Whole(Call->FunctionInputs[1].Input) || Call->FunctionInputs[1].Input.Expression->GetOuter()!=Master
            || !Private::GuidIs(Call->FunctionOutputs[0].ExpressionOutputId,TEXT("4E870CE3478B3E536B80D4BBCD8A0F22"))
            || !Private::GuidIs(Call->FunctionOutputs[1].ExpressionOutputId,TEXT("F1D68DC34E67CE57967C3D8967D51EFB")))
            return Fail(TEXT("Cold master original/optional caller GUID links or output order changed"));
        TSet<UMaterialExpression*> PackTree;
        if (!APSPlanetReliefNormalUpdate::Private::Tree(Master,Call->FunctionInputs[1].Input.Expression,PackTree,true,Error)) return false;
        int32 CoverageSamples=0;
        for (UMaterialExpression* E:PackTree) if (auto* C=Cast<UMaterialExpressionCustom>(E))
            CoverageSamples += C->Description==TEXT("APS canonical stereo coverage sample v1");
        if (CoverageSamples!=1) return Fail(TEXT("Cold master optional input lost its coverage sample dependency"));
        UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalPixelSlope] VERIFY COLD PASS eodSiblings=%d canonicalActive=1 orderedClosure=145 optionalFloat4W0=1 nativeVIExcluded=1 farAdapters=2 originalOutputGuids=1 masterPackLinked=1 noMutations=1"),Collections.Num());
        return true;
    }
}
#endif
