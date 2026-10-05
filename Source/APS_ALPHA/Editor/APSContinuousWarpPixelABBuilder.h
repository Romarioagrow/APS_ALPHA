#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainLodABBuilder.h"
#include "APS_ALPHA/Tests/APSContinuousWarpPixelAssets.h"
#include "APS_ALPHA/Core/Planetary/APSTerrainContinuityMaterial.h"
#include "HAL/FileManager.h"
#include "MaterialTypes.h"
#include "UObject/UObjectHash.h"

// Exact-current Continuous diagnostic. Only five master coordinate VIs are
// bypassed. Five explicitly audited functions form a portable private closure;
// function mathematics, native slope VI, normals and distance gates stay.
namespace APSContinuousWarpPixelABBuilder
{
    // UE 5.4 EditorOnlyData::Serialize selects itself as the active object;
    // function PostLoad can then copy a source-named sibling over the canonical
    // object. The cold V2 audit proved stale lists survive beside full lists.
    // Only these private copies' two exact, audited direct siblings are touched.
    inline bool FunctionCollections(UMaterialFunction* Original, UMaterialFunction* Copy,
        const TArray<UMaterialExpression*>& Graph,
        TMap<UMaterialFunctionEditorOnlyData*, FMaterialExpressionCollection>& Expected,
        bool Initialize, const TCHAR* Stage, FString& Error, const TCHAR* DiagnosticDestination)
    {
        const auto Fail = [&](const FString& Why)
        { Error = TEXT("Private function EOD closure: ") + Why; return false; };
        if (!Original || !Copy || Original == Copy || Original->GetOutermost() == Copy->GetOutermost()
            || !Copy->GetOutermost()->GetName().StartsWith(FString(DiagnosticDestination) + TEXT("/")))
            return Fail(TEXT("Only new diagnostic-owned copies may be normalized"));
        TArray<UObject*> Children;
        GetObjectsWithOuter(Copy, Children, false);
        TArray<UMaterialFunctionEditorOnlyData*> Siblings;
        const FString OriginalName = Original->GetName() + TEXT("EditorOnlyData");
        const FString CopyName = Copy->GetName() + TEXT("EditorOnlyData");
        // The base EOD class is NO_API in UE 5.4. Obtain its reflected class
        // through the exported concrete class without importing its StaticClass.
        const UClass* EODBaseClass = UMaterialFunctionEditorOnlyData::StaticClass()->GetSuperClass();
        for (UObject* Child : Children)
            if (Child && Child->IsA(EODBaseClass))
            {
                auto* Data = Child;
                if (Data->GetClass() != UMaterialFunctionEditorOnlyData::StaticClass()
                    || (Data->GetName() != OriginalName && Data->GetName() != CopyName))
                    return Fail(TEXT("Unexpected direct EOD sibling: ") + Data->GetPathName());
                Siblings.Add(CastChecked<UMaterialFunctionEditorOnlyData>(Data));
            }
        if (Siblings.Num() != 2 || !Siblings.Contains(Copy->GetEditorOnlyData()))
            return Fail(Copy->GetPathName() + TEXT(" requires the exact source/copy EOD pair and an owned active pointer"));
        TSet<UMaterialExpression*> Unique;
        for (UMaterialExpression* E : Graph)
        {
            if (!E || !E->IsIn(Copy) || Unique.Contains(E)) return Fail(TEXT("Invalid ordered graph"));
            Unique.Add(E);
        }
        // Validate every existing list before changing either. No foreign or
        // extra nodes may be hidden by replacing a sibling's list.
        for (auto* Data : Siblings)
            for (UMaterialExpression* E : Data->ExpressionCollection.Expressions)
                if (!Unique.Contains(E)) return Fail(TEXT("Sibling contains an unaudited node: ") + Data->GetPathName());
        for (auto* Data : Siblings)
        {
            if (Initialize)
            {
                if (Expected.Contains(Data)) return Fail(TEXT("EOD already initialized"));
                auto& Snapshot = Expected.Add(Data, Data->ExpressionCollection);
                Snapshot.Expressions.Reset(Graph.Num());
                for (UMaterialExpression* E : Graph) Snapshot.Expressions.Add(E);
                // Only the expression list changes; comments/exec fields and
                // all expression objects, GUIDs, properties and edges stay.
                Data->ExpressionCollection.Expressions = Snapshot.Expressions;
            }
            const auto* Snapshot = Expected.Find(Data);
            const auto& Actual = Data->ExpressionCollection;
            if (!Snapshot || Actual.Expressions != Snapshot->Expressions
                || Actual.EditorComments != Snapshot->EditorComments
                || Actual.ExpressionExecBegin != Snapshot->ExpressionExecBegin
                || Actual.ExpressionExecEnd != Snapshot->ExpressionExecEnd
                || Actual.Expressions.Num() != Graph.Num())
                return Fail(TEXT("Saved EOD fields/order changed: ") + Data->GetPathName());
            for (int32 Index = 0; Index < Graph.Num(); ++Index)
                if (Actual.Expressions[Index] != Graph[Index]) return Fail(TEXT("EOD graph order changed"));
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Bake] eodClosure stage=%s object=%s active=%d expressions=%d orderedParity=1 commentsExecUnchanged=1"),
                Stage, *Data->GetPathName(), Data == Copy->GetEditorOnlyData(), Actual.Expressions.Num());
        }
        return true;
    }

    // DuplicateAsset preserves expression math; no function Patch is called.
    // Verify node/edge identities, parameter defaults and serialized call pins.
    // Function call pins are checked separately: their private object pointers
    // are transient, whereas GUIDs, ordering and connections must be identical.
    inline bool VerifyFunctionCopy(UMaterialFunction* Source, UMaterialFunction* Copy,
        const TArray<UMaterialExpression*>& Original, const TArray<UMaterialExpression*>& Duplicate,
        const TMap<UMaterialFunction*, UMaterialFunction*>& Copies, FString& Error)
    {
        const auto Fail = [&](const FString& Why)
        { Error = TEXT("Portable function copy parity: ") + Why; return false; };
        if (Original.Num() != Duplicate.Num()) return Fail(Source->GetPathName() + TEXT(" node count"));
        TMap<FString, UMaterialExpression*> Nodes;
        for (UMaterialExpression* E : Duplicate)
        {
            if (!E || !E->IsIn(Copy)) return Fail(TEXT("Foreign/null duplicate node"));
            const FString Name = E->GetPathName(Copy);
            if (Nodes.Contains(Name)) return Fail(TEXT("Ambiguous duplicate node: ") + Name);
            Nodes.Add(Name, E);
        }
        const auto SameInput = [&](const FExpressionInput& A, const FExpressionInput& B)
        {
            return ((!A.Expression && !B.Expression)
                || (A.Expression && B.Expression && A.Expression->IsIn(Source)
                    && Nodes.FindRef(A.Expression->GetPathName(Source)) == B.Expression))
                && A.OutputIndex == B.OutputIndex && A.InputName == B.InputName
                && A.Mask == B.Mask && A.MaskR == B.MaskR && A.MaskG == B.MaskG
                && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
        };
        for (UMaterialExpression* A : Original)
        {
            UMaterialExpression* D = A ? Nodes.FindRef(A->GetPathName(Source)) : nullptr;
            if (!A || !A->IsIn(Source) || !D || A->GetClass() != D->GetClass()
                || A->GetMaterialExpressionId() != D->GetMaterialExpressionId())
                return Fail(TEXT("Node identity/class/owner changed"));
            if (A->HasAParameterName() != D->HasAParameterName()) return Fail(TEXT("Parameter kind changed"));
            if (A->HasAParameterName())
            {
                FMaterialParameterMetadata AP, DP;
                if (!A->GetParameterValue(AP) || !D->GetParameterValue(DP)
                    || A->GetParameterName() != D->GetParameterName() || AP.Value != DP.Value
                    || AP.ExpressionGuid != DP.ExpressionGuid || AP.PrimitiveDataIndex != DP.PrimitiveDataIndex
                    || AP.bDynamicSwitchParameter != DP.bDynamicSwitchParameter
                    || AP.Group != DP.Group || AP.SortPriority != DP.SortPriority)
                    return Fail(A->GetPathName() + TEXT(" parameter name/GUID/default"));
            }
            const auto AInputs = A->GetInputsView(), DInputs = D->GetInputsView();
            if (AInputs.Num() != DInputs.Num()) return Fail(A->GetPathName() + TEXT(" input count"));
            for (int32 I = 0; I < AInputs.Num(); ++I)
                if ((!AInputs[I] != !DInputs[I]) || (AInputs[I] && !SameInput(*AInputs[I], *DInputs[I])))
                    return Fail(A->GetPathName() + TEXT(" input edge/mask/order"));
            if (auto* AC = Cast<UMaterialExpressionMaterialFunctionCall>(A))
            {
                auto* DC = CastChecked<UMaterialExpressionMaterialFunctionCall>(D);
                UMaterialFunctionInterface* Expected = AC->MaterialFunction;
                if (auto* Function = Cast<UMaterialFunction>(Expected))
                    if (auto* const* Relocated = Copies.Find(Function)) Expected = *Relocated;
                if (Expected != DC->MaterialFunction
                    || AC->FunctionInputs.Num() != DC->FunctionInputs.Num()
                    || AC->FunctionOutputs.Num() != DC->FunctionOutputs.Num())
                    return Fail(A->GetPathName() + TEXT(" function pin count"));
                for (int32 I = 0; I < AC->FunctionInputs.Num(); ++I)
                    if (AC->FunctionInputs[I].ExpressionInputId != DC->FunctionInputs[I].ExpressionInputId
                        || !SameInput(AC->FunctionInputs[I].Input, DC->FunctionInputs[I].Input))
                        return Fail(A->GetPathName() + TEXT(" function input GUID/order/edge"));
                for (int32 I = 0; I < AC->FunctionOutputs.Num(); ++I)
                {
                    const auto& AP = AC->FunctionOutputs[I]; const auto& DP = DC->FunctionOutputs[I];
                    if (AP.ExpressionOutputId != DP.ExpressionOutputId || AP.Output.OutputName != DP.Output.OutputName
                        || AP.Output.Mask != DP.Output.Mask || AP.Output.MaskR != DP.Output.MaskR
                        || AP.Output.MaskG != DP.Output.MaskG || AP.Output.MaskB != DP.Output.MaskB
                        || AP.Output.MaskA != DP.Output.MaskA)
                        return Fail(A->GetPathName() + TEXT(" function output GUID/order/mask"));
                }
            }
        }
        return true;
    }

    // Defaults retain the original immutable V3 experiment. A separately guarded
    // provenance wrapper may reuse the same cold-safe transform at a new path.
    inline bool Build(IAssetTools& Tools,
        const TCHAR* DiagnosticDestination = APSContinuousWarpPixelAssets::Destination,
        bool (*VerifyInputSources)(FString&) = APSContinuousWarpPixelAssets::VerifySources)
    {
        using namespace APSContinuousWarpPixelAssets;
        const auto Refuse = [](const FString& Error)
        { UE_LOG(LogTemp, Error, TEXT("[APS.ContinuousWarpPixel.Bake] Refused: %s"), *Error); return false; };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline commandlet only"));
        if (!DiagnosticDestination || !VerifyInputSources
            || !FString(DiagnosticDestination).StartsWith(TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/")))
            return Refuse(TEXT("Explicit diagnostic-only destination and provenance guard required"));
        FString Error;
        if (!VerifyInputSources(Error)) return Refuse(Error);
        const FString Directory = FPackageName::LongPackageNameToFilename(DiagnosticDestination);
        if (IFileManager::Get().DirectoryExists(*Directory))
            return Refuse(TEXT("Immutable diagnostic destination already exists"));
        auto* Source = LoadObject<UMaterial>(nullptr, APSTerrainContinuityMaterial::MasterPath);
        auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, APSTerrainContinuityMaterial::TemplatePath);
        if (!Source || !Template || Template->Parent != Source
            || Source->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty())
            return Refuse(TEXT("Saved exact Continuous master/template required"));
        APSSharedTerrainMaterialBuilder::FBuild B(Tools, DiagnosticDestination);
        APSSharedTerrainNormalContinuity::TTransform<decltype(B)> Reader(B);

        // Audit the actual input-linked closure against the serialized flat
        // collections. Cold PostLoad only hydrates calls in those collections.
        // Snapshot all production dependencies; never register nodes on them.
        TArray<UObject*> Pending{Source};
        TSet<UObject*> Seen;
        TMap<FString, FString> DependencyHashes;
        TMap<UObject*, TArray<UMaterialExpression*>> SourceGraphs;
        TMap<FString, UMaterialFunction*> SourceFunctions;
        FString AuditErrors;
        const auto AuditFailure = [&](const FString& Why) { AuditErrors += Why + TEXT("\n"); };
        for (int32 Index = 0; Index < Pending.Num(); ++Index)
        {
            UObject* Owner = Pending[Index];
            if (!Owner || Seen.Contains(Owner)) continue;
            if (Seen.Num() >= 128) return Refuse(TEXT("Oversized source closure"));
            if (Owner->GetOutermost()->IsDirty()) AuditFailure(TEXT("Dirty source dependency: ") + Owner->GetPathName());
            Seen.Add(Owner);
            const FString Package = Owner->GetOutermost()->GetName();
            const FString Before = Hash(Package);
            if (Before.IsEmpty()) AuditFailure(TEXT("Source dependency missing on disk: ") + Package);
            DependencyHashes.Add(Package, Before);
            const auto Graph = Reader.Graph(Owner);
            if (!B.Error.IsEmpty()) return Refuse(B.Error);
            if (Owner == Source)
                for (int32 Property = 0; Property < MP_MAX; ++Property)
                    if (auto* Input = Source->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Property));
                        Input && Input->Expression && !Graph.Contains(Input->Expression))
                        AuditFailure(TEXT("Material output lies outside audited expression closure"));
            SourceGraphs.Add(Owner, Graph);
            if (auto* Function = Cast<UMaterialFunction>(Owner)) SourceFunctions.Add(Package, Function);
            const auto Serialized = B.Expressions(Owner);
            int32 Missing = 0;
            for (UMaterialExpression* E : Graph)
            {
                Missing += !Serialized.Contains(E);
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E); Call && Call->MaterialFunction)
                {
                    auto* Function = Cast<UMaterialFunction>(Call->MaterialFunction);
                    if (!Function) { AuditFailure(TEXT("Function instances require explicit audit: ") + Call->GetPathName()); continue; }
                    Pending.AddUnique(Function);
                }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Bake] closure=%s serialized=%d reachable=%d missing=%d"),
                *Owner->GetPathName(), Serialized.Num(), Graph.Num(), Missing);
            const FSource* Audited = nullptr;
            for (const FSource& Entry : Sources) if (Package == Entry.Package) { Audited = &Entry; break; }
            if (!Audited || Serialized.Num() != Audited->Serialized || Graph.Num() != Audited->Reachable)
                AuditFailure(FString::Printf(TEXT("Source closure inventory changed: %s serialized=%d reachable=%d expected=%d/%d"),
                    *Owner->GetPathName(), Serialized.Num(), Graph.Num(),
                    Audited ? Audited->Serialized : -1, Audited ? Audited->Reachable : -1));
        }
        DependencyHashes.Add(Template->GetOutermost()->GetName(), Hash(Template->GetOutermost()->GetName()));
        if (Seen.Num() != static_cast<int32>(UE_ARRAY_COUNT(Sources)) - 1) AuditFailure(TEXT("Source closure package count changed"));
        for (const auto& Entry : PortableFunctions)
            if (!SourceFunctions.Contains(Entry.Package)) AuditFailure(TEXT("Missing portable dependency: ") + FString(Entry.Package));
        // Audit every reference to the five copied functions, including input-
        // reachable calls outside flat collections. No unaudited parent is copied.
        for (UObject* Owner : Pending)
            for (const auto& Entry : PortableFunctions)
            {
                int32 Calls = 0;
                for (UMaterialExpression* E : SourceGraphs.FindChecked(Owner))
                    if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E); Call && Call->MaterialFunction
                        && Call->MaterialFunction->GetOutermost()->GetName() == Entry.Package) ++Calls;
                const FString OwnerPackage = Owner->GetOutermost()->GetName();
                const int32 Expected = Owner == Source ? Entry.MasterCalls
                    : OwnerPackage == PortableFunctions[2].Package ? Entry.SlopeCalls
                    : OwnerPackage == PortableFunctions[3].Package ? Entry.OrbitalCalls : 0;
                if (Calls || Expected)
                    UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Bake] topology=%s -> %s calls=%d expected=%d"),
                        *OwnerPackage, Entry.Package, Calls, Expected);
                if (Calls != Expected) AuditFailure(FString::Printf(TEXT("Portable dependency topology changed: %s -> %s calls=%d expected=%d"),
                    *OwnerPackage, Entry.Package, Calls, Expected));
            }
        // Complete inventory and topology diagnostics precede any duplication.
        if (!AuditErrors.IsEmpty()) return Refuse(AuditErrors);
        TMap<UMaterialFunction*, UMaterialFunction*> Copies;
        TMap<UMaterialFunctionEditorOnlyData*, FMaterialExpressionCollection> ExpectedCollections;
        int32 RegisteredFunctionNodes = 0;
        for (const auto& Entry : PortableFunctions)
        {
            UMaterialFunction* Original = SourceFunctions.FindChecked(Entry.Package);
            auto* Copy = Cast<UMaterialFunction>(B.Duplicate(Original, Entry.Output));
            if (!Copy) return Refuse(B.Error);
            Copies.Add(Original, Copy);
            const auto FullGraph = Reader.Graph(Copy);
            if (!B.Error.IsEmpty()) return Refuse(B.Error);
            for (UMaterialExpression* E : FullGraph)
            {
                RegisteredFunctionNodes += !B.Expressions(Copy).Contains(E);
                Reader.Register(Copy, E);
            }
            if (!B.Error.IsEmpty()) return Refuse(B.Error);
            if (!FunctionCollections(Original, Copy, FullGraph, ExpectedCollections, true, TEXT("initialize"), B.Error, DiagnosticDestination)) return Refuse(B.Error);
        }
        if (RegisteredFunctionNodes != 245) return Refuse(FString::Printf(
            TEXT("Expected exactly 55+29+96+65 newly registered function nodes (245), got %d"), RegisteredFunctionNodes));
        const auto VerifyPrivateClosure = [&](const TCHAR* Stage)
        {
            if (Copies.Num() != 5 || ExpectedCollections.Num() != 10)
            { B.Error = TEXT("Expected exactly five private functions and ten audited EOD siblings"); return false; }
            for (const auto& Entry : PortableFunctions)
            {
                auto* Original = SourceFunctions.FindChecked(Entry.Package);
                auto* Copy = Copies.FindChecked(Original);
                const auto FullGraph = Reader.Graph(Copy);
                if (!B.Error.IsEmpty()
                    || !VerifyFunctionCopy(Original, Copy, SourceGraphs.FindChecked(Original), FullGraph, Copies, B.Error)
                    || !FunctionCollections(Original, Copy, FullGraph, ExpectedCollections, false, Stage, B.Error, DiagnosticDestination)) return false;
            }
            return true;
        };
        const auto RebindCalls = [&](UObject* Owner)
        {
            for (UMaterialExpression* E : Reader.Graph(Owner))
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                    if (auto* Original = Cast<UMaterialFunction>(Call->MaterialFunction))
                    {
                        auto** Copy = Copies.Find(Original);
                        // Also validate/hydrate unchanged callees by their exact
                        // interface GUIDs; never use name-based SetMaterialFunction.
                        if (!B.ReconnectFunctionById(Call, Original, Copy ? *Copy : Original)) return false;
                    }
            return B.Error.IsEmpty();
        };
        for (const auto& Entry : PortableFunctions)
        {
            UMaterialFunction* Original = SourceFunctions.FindChecked(Entry.Package);
            UMaterialFunction* Copy = Copies.FindChecked(Original);
            if (!RebindCalls(Copy)
                || !VerifyFunctionCopy(Original, Copy, SourceGraphs.FindChecked(Original), Reader.Graph(Copy), Copies, B.Error)) return Refuse(B.Error);
            UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
        }
        auto* Master = Cast<UMaterial>(B.Duplicate(Source, TEXT("M_APS_ContinuousWarpPixel")));
        if (!Master) return Refuse(B.Error);
        const auto Graph = Reader.Graph(Master);
        if (!B.Error.IsEmpty()) return Refuse(B.Error);
        for (UMaterialExpression* E : Graph) Reader.Register(Master, E);
        if (!RebindCalls(Master)) return Refuse(B.Error);
        TMap<UMaterialExpression*, UMaterialExpression*> Replacements;
        for (UMaterialExpression* E : Graph)
        {
            Reader.Register(Master, E);
            if (auto* VI = Cast<UMaterialExpressionVertexInterpolator>(E))
            {
                // Same identity-pin/swizzle guard as LodABBuilder::Patch.
                if (!VI->Input.Expression || VI->Input.OutputIndex != 0 || VI->Input.Mask)
                    return Refuse(TEXT("Unexpected coordinate VI input mapping"));
                // The five native warp DAGs are master-owned and contain no
                // functions or geometric-normal adapters. Never bypass slope.
                TArray<UMaterialExpression*> Warp{VI->Input.Expression};
                TSet<UMaterialExpression*> WarpSeen;
                for (int32 I = 0; I < Warp.Num(); ++I)
                {
                    auto* Node = Warp[I];
                    if (!Node || WarpSeen.Contains(Node)) continue;
                    if (WarpSeen.Num() >= 512 || !Node->IsIn(Master)
                        || Cast<UMaterialExpressionMaterialFunctionCall>(Node)
                        || Cast<UMaterialExpressionVertexNormalWS>(Node)
                        || APSSharedTerrainNormalContinuity::TTransform<decltype(B)>::NormalInput(Node))
                        return Refuse(TEXT("Audited master coordinate warp dependency changed"));
                    WarpSeen.Add(Node);
                    for (auto* Input : Node->GetInputsView()) if (Input && Input->Expression) Warp.Add(Input->Expression);
                }
                Replacements.Add(VI, VI->Input.Expression);
            }
        }
        if (Replacements.Num() != 5) return Refuse(FString::Printf(TEXT("Expected exactly five master warp VIs, got %d"), Replacements.Num()));
        int32 Consumers = 0;
        const auto Rewire = [&](FExpressionInput& Input)
        {
            TSet<UMaterialExpression*> Active;
            while (UMaterialExpression** Replacement = Replacements.Find(Input.Expression))
            {
                if (Input.OutputIndex != 0 || Active.Contains(Input.Expression))
                { B.Error = TEXT("Unexpected coordinate VI consumer mapping/cycle"); return; }
                Active.Add(Input.Expression);
                Input.Expression = *Replacement; // All masks/names/output indices retained.
                ++Consumers;
            }
        };
        for (UMaterialExpression* E : Graph)
            for (FExpressionInput* Input : E->GetInputsView()) if (Input) Rewire(*Input);
        for (int32 Property = 0; Property < MP_MAX; ++Property)
            if (auto* Input = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Property))) Rewire(*Input);
        if (!B.Error.IsEmpty() || Consumers < 5) return Refuse(B.Error.IsEmpty() ? TEXT("Not all five master warp VIs had consumers") : B.Error);
        for (UMaterialExpression* E : Graph)
            for (const FExpressionInput* Input : E->GetInputsView())
                if (Input && Replacements.Contains(Input->Expression)) return Refuse(TEXT("A master warp VI consumer remains"));

        auto* Instance = Cast<UMaterialInstanceConstant>(B.Duplicate(Template, TEXT("MI_APS_ContinuousWarpPixel")));
        if (!Instance || B.Outputs.Num() != 7)
            return Refuse(TEXT("Unexpected portable candidate outputs"));
        Instance->SetParentEditorOnly(Master, false);
        Instance->CopyMaterialUniformParametersEditorOnly(Template, true);
        Instance->PostEditChange();
        Master->PostEditChange();
        APSSharedTerrainLodABBuilder::FBuilder PinRepair(Tools);
        if (!PinRepair.RestoreTransientFunctionPins(Master)) return Refuse(PinRepair.B.Error);
        // Both serializable EOD siblings must retain the full ordered list;
        // checking only the currently selected pointer missed the V2 failure.
        for (const auto& Pair : Copies)
            if (!VerifyFunctionCopy(Pair.Key, Pair.Value, SourceGraphs.FindChecked(Pair.Key), Reader.Graph(Pair.Value), Copies, B.Error)) return Refuse(B.Error);
        for (UObject* Owner : B.Outputs)
        {
            if (Owner == Instance) continue;
            const auto Serialized = B.Expressions(Owner);
            for (UMaterialExpression* E : Reader.Graph(Owner))
            {
                if (!Serialized.Contains(E)) return Refuse(TEXT("Portable closure retains an unregistered node: ") + E->GetPathName());
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E); Call && Call->MaterialFunction)
                {
                    auto* Function = Cast<UMaterialFunction>(Call->MaterialFunction);
                    if (!Function || Copies.Contains(Function))
                        return Refuse(TEXT("Portable closure still references an incomplete production dependency"));
                }
            }
            if (!B.Error.IsEmpty()) return Refuse(B.Error);
        }
        // Finish and validate the master before the MIC can reuse its shader map.
        // Joining an in-progress map can retain the MIC's earlier translation errors;
        // the engine's completed-map cache path clears those errors normally.
        if (!VerifyPrivateClosure(TEXT("before-final-compile"))) return Refuse(B.Error);
        for (UMaterialInterface* M : {static_cast<UMaterialInterface*>(Master), static_cast<UMaterialInterface*>(Instance)})
        {
            M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
            auto* Resource = M->GetMaterialResource(GMaxRHIFeatureLevel);
            const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Bake] final material=%s resource=%d complete=%d errors=%d map=%d localVF=%d base=%s"),
                *M->GetPathName(), Resource != nullptr, Resource && Resource->IsGameThreadShaderMapComplete(),
                Resource ? Resource->GetCompileErrors().Num() : -1, Map != nullptr,
                Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType),
                *GetPathNameSafe(Resource ? Resource->GetMaterial() : nullptr));
            if (Resource)
                for (const FString& CompileError : Resource->GetCompileErrors())
                    UE_LOG(LogTemp, Error, TEXT("[APS.ContinuousWarpPixel.Bake] final material=%s compileError=%s"),
                        *M->GetPathName(), *CompileError);
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return Refuse(TEXT("Candidate shader incomplete/errors/missing LocalVF: ") + M->GetPathName());
        }
        if (!VerifyInputSources(Error)) return Refuse(Error);
        for (const auto& Before : DependencyHashes)
            if (Hash(Before.Key) != Before.Value) return Refuse(TEXT("Source dependency changed during bake: ") + Before.Key);
        for (UObject* Owner : Seen)
            if (Owner->GetOutermost()->IsDirty()) return Refuse(TEXT("Source dependency became dirty: ") + Owner->GetPathName());
        if (Template->GetOutermost()->IsDirty()) return Refuse(TEXT("Source template became dirty"));
        for (UObject* Output : B.Outputs)
        {
            if (!VerifyPrivateClosure(TEXT("before-save"))) return Refuse(B.Error);
            UPackage* Package = Output->GetOutermost();
            Package->MarkPackageDirty();
            FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            if (!UPackage::SavePackage(Package, Output, *File, Args)) return Refuse(TEXT("Candidate save failed: ") + File);
            if (!VerifyPrivateClosure(TEXT("after-save"))) return Refuse(B.Error);
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Bake] saved=%s SHA1=%s"), *File, *Hash(Package->GetName()));
        }
        if (!VerifyPrivateClosure(TEXT("after-all-saves"))) return Refuse(B.Error);
        if (!VerifyInputSources(Error)) return Refuse(Error);
        for (const auto& Before : DependencyHashes)
            if (Hash(Before.Key) != Before.Value) return Refuse(TEXT("Source dependency changed during saves: ") + Before.Key);
        for (UObject* Owner : Seen)
            if (Owner->GetOutermost()->IsDirty()) return Refuse(TEXT("Source dependency became dirty during saves: ") + Owner->GetPathName());
        if (Template->GetOutermost()->IsDirty()) return Refuse(TEXT("Source template became dirty during saves"));
        for (const auto& Before : DependencyHashes)
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Bake] source=%s SHA1=%s unchanged=1"), *Before.Key, *Before.Value);
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Bake] READY masterWarpVIs=5 consumers=%d outputAssets=7 privateFunctions=5 eodSiblings=10 registeredFunctionNodes=%d productionFunctionChanges=0; native fields/palette/scales/normal/slope/distance thresholds retained; separate cold reload and rendered validation required"),
            Consumers, RegisteredFunctionNodes);
        return true;
    }
}
#endif
