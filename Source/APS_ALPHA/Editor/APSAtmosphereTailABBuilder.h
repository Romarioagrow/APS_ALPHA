#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "APS_ALPHA/Tests/APSAtmosphereTailAssets.h"
#include "APS_ALPHA/Core/Rendering/APSAtmosphereTailMaterial.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"

// One new immutable master. No function copies/repairs, alpha rewiring, radius,
// scale-height, colour, geometry or production material changes are permitted.
namespace APSAtmosphereTailABBuilder
{
    using FBuild = APSSharedTerrainMaterialBuilder::FBuild;

    inline bool Collect(UObject* Owner, TArray<UMaterialExpression*> Pending,
        TArray<UMaterialExpression*>& Out, FString& Error)
    {
        Out.Reset();
        TSet<UMaterialExpression*> Seen;
        for (int32 I = 0; I < Pending.Num(); ++I)
        {
            UMaterialExpression* E = Pending[I];
            if (!E || Seen.Contains(E)) continue;
            if (Seen.Num() >= 4096 || !E->IsIn(Owner))
            { Error = TEXT("Invalid/oversized expression closure: ") + GetPathNameSafe(E); return false; }
            const FString Class = E->GetClass()->GetName();
            if (Class.Contains(TEXT("NamedReroute")) || Class.Contains(TEXT("MaterialAttributeLayers")))
            { Error = TEXT("Unaudited non-input dependency: ") + E->GetPathName(); return false; }
            Seen.Add(E); Out.Add(E);
            for (FExpressionInput* Input : E->GetInputsView())
                if (Input && Input->Expression) Pending.Add(Input->Expression);
        }
        return true;
    }

    inline bool Graph(UObject* Owner, TArray<UMaterialExpression*>& Out, FString& Error)
    {
        auto Seeds = FBuild::Expressions(Owner);
        if (auto* M = Cast<UMaterial>(Owner))
            for (int32 P = 0; P < MP_MAX; ++P)
            {
                FMaterialInputDescription D;
                if (M->GetExpressionInputDescription(static_cast<EMaterialProperty>(P), D)
                    && D.Input && D.Input->Expression) Seeds.Add(D.Input->Expression);
            }
        return Collect(Owner, MoveTemp(Seeds), Out, Error);
    }

    // Four exact substitutions; the remaining source text is retained verbatim.
    inline bool MakeCode(const FString& Source, FString& Out, FString& Error)
    {
        if (Source.Contains(TEXT("APS_Diag")))
        { Error = TEXT("Density source is already diagnostic"); return false; }
        Out = Source;
        for (const TCHAR* Altitude : {TEXT("cam_alt"), TEXT("light_alt")})
            for (const TCHAR* Scale : {TEXT("scale_height_r"), TEXT("scale_height_m")})
            {
                const FString Term = FString::Printf(TEXT("exp(-%s / %s)"), Altitude, Scale);
                const int32 At = Out.Find(Term, ESearchCase::CaseSensitive);
                if (At == INDEX_NONE || Out.Find(Term, ESearchCase::CaseSensitive,
                    ESearchDir::FromStart, At + Term.Len()) != INDEX_NONE)
                { Error = TEXT("Expected exactly one density term: ") + Term; return false; }
                const FString Replacement = FString::Printf(
                    TEXT("(%s * lerp(1.0, 1.0 - smoothstep(0.8 * APS_DiagTailHeight, APS_DiagTailHeight, %s), APS_DiagTailWeight))"),
                    *Term, Altitude);
                Out = Out.Left(At) + Replacement + Out.Mid(At + Term.Len());
            }
        Out = TEXT("const float APS_DiagTailHeight = AtmosRadius - earth_radius;\n")
            TEXT("const float APS_DiagTailWeight = saturate(APS_DiagAtmosphereTail);\n") + Out;
        return true;
    }

    inline bool SameInput(const FExpressionInput& A, const FExpressionInput& B,
        UMaterial* Source, UMaterial* Copy)
    {
        return ((!A.Expression && !B.Expression) || (A.Expression && B.Expression
            && A.Expression->IsIn(Source) && B.Expression->IsIn(Copy)
            && A.Expression->GetPathName(Source) == B.Expression->GetPathName(Copy)))
            && A.OutputIndex == B.OutputIndex && A.InputName == B.InputName
            && A.Mask == B.Mask && A.MaskR == B.MaskR && A.MaskG == B.MaskG
            && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
    }

    // Compare serialized expression properties, including constants, sampler
    // settings and unused Custom code. Only the selected Code/Inputs may differ.
    inline bool VerifyCopy(UMaterial* Source, UMaterial* Copy,
        const TArray<UMaterialExpression*>& Original, UMaterialExpressionCustom* Changed,
        const FString& ExpectedCode, int32 AddedInputs, FString& Error)
    {
        const auto Fail = [&](const FString& Why) { Error = TEXT("Clone parity: ") + Why; return false; };
        TArray<UMaterialExpression*> Duplicate;
        if (!Graph(Copy, Duplicate, Error)) return false;
        TMap<FString, UMaterialExpression*> Nodes;
        for (UMaterialExpression* E : Duplicate)
        {
            const FString Name = E->GetPathName(Copy);
            if (Nodes.Contains(Name)) return Fail(TEXT("Duplicate node identity: ") + Name);
            Nodes.Add(Name, E);
        }
        if (Duplicate.Num() != Original.Num() + (Changed ? 1 : 0)) return Fail(TEXT("Node count changed"));
        for (UMaterialExpression* A : Original)
        {
            UMaterialExpression* D = Nodes.FindRef(A->GetPathName(Source));
            if (!D || A->GetClass() != D->GetClass() || A->MaterialExpressionGuid != D->MaterialExpressionGuid)
                return Fail(A->GetPathName() + TEXT(" identity/class/GUID"));
            for (TFieldIterator<FProperty> It(A->GetClass()); It; ++It)
            {
                FProperty* P = *It;
                if (P->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient | CPF_Deprecated)) continue;
                if (D == Changed && (P->GetFName() == TEXT("Code") || P->GetFName() == TEXT("Inputs"))) continue;
                if (Cast<UMaterialExpressionMaterialFunctionCall>(A)
                    && (P->GetFName() == TEXT("FunctionInputs") || P->GetFName() == TEXT("FunctionOutputs"))) continue;
                for (int32 I = 0; I < P->ArrayDim; ++I)
                {
                    FString AV, DV;
                    P->ExportText_InContainer(I, AV, A, nullptr, nullptr, PPF_None);
                    P->ExportText_InContainer(I, DV, D, nullptr, nullptr, PPF_None);
                    DV.ReplaceInline(*Copy->GetPathName(), *Source->GetPathName(), ESearchCase::CaseSensitive);
                    if (AV != DV) return Fail(A->GetPathName() + TEXT(" property ") + P->GetName());
                }
            }
            if (auto* AC = Cast<UMaterialExpressionMaterialFunctionCall>(A))
            {
                auto* DC = CastChecked<UMaterialExpressionMaterialFunctionCall>(D);
                if (AC->MaterialFunction != DC->MaterialFunction || AC->FunctionInputs.Num() != DC->FunctionInputs.Num()
                    || AC->FunctionOutputs.Num() != DC->FunctionOutputs.Num()) return Fail(TEXT("Function pin counts/reference"));
                for (int32 I = 0; I < AC->FunctionInputs.Num(); ++I)
                    if (AC->FunctionInputs[I].ExpressionInputId != DC->FunctionInputs[I].ExpressionInputId
                        || !SameInput(AC->FunctionInputs[I].Input, DC->FunctionInputs[I].Input, Source, Copy))
                        return Fail(AC->GetPathName() + TEXT(" function input GUID/edge"));
                for (int32 I = 0; I < AC->FunctionOutputs.Num(); ++I)
                {
                    const auto& AP = AC->FunctionOutputs[I]; const auto& DP = DC->FunctionOutputs[I];
                    if (AP.ExpressionOutputId != DP.ExpressionOutputId || AP.Output.OutputName != DP.Output.OutputName
                        || AP.Output.Mask != DP.Output.Mask || AP.Output.MaskR != DP.Output.MaskR
                        || AP.Output.MaskG != DP.Output.MaskG || AP.Output.MaskB != DP.Output.MaskB || AP.Output.MaskA != DP.Output.MaskA)
                        return Fail(AC->GetPathName() + TEXT(" function output GUID/mask"));
                }
            }
            if (D == Changed)
            {
                auto* AC = CastChecked<UMaterialExpressionCustom>(A);
                if (Changed->Code != ExpectedCode || Changed->Inputs.Num() != AC->Inputs.Num() + AddedInputs)
                    return Fail(TEXT("Density Code/Inputs"));
                for (int32 I = 0; I < AC->Inputs.Num(); ++I)
                    if (AC->Inputs[I].InputName != Changed->Inputs[I].InputName
                        || !SameInput(AC->Inputs[I].Input, Changed->Inputs[I].Input, Source, Copy))
                        return Fail(TEXT("Original density input changed"));
            }
        }
        for (int32 P = 0; P < MP_MAX; ++P)
        {
            FMaterialInputDescription A, D;
            if (Source->GetExpressionInputDescription(static_cast<EMaterialProperty>(P), A)
                != Copy->GetExpressionInputDescription(static_cast<EMaterialProperty>(P), D)
                || !!A.Input != !!D.Input || A.Type != D.Type || A.bHidden != D.bHidden
                || A.bUseConstant != D.bUseConstant || A.ConstantValue != D.ConstantValue
                || (A.Input && !SameInput(*A.Input, *D.Input, Source, Copy)))
                return Fail(FString::Printf(TEXT("Material output %d"), P));
        }
        for (const TCHAR* Name : {TEXT("BlendMode"), TEXT("ShadingModel"), TEXT("TwoSided"),
            TEXT("bDisableDepthTest"), TEXT("bUseMaterialAttributes"), TEXT("MaterialDomain")})
        {
            FProperty* P = FindFProperty<FProperty>(UMaterial::StaticClass(), Name);
            if (!P || !P->Identical_InContainer(Source, Copy)) return Fail(FString(TEXT("Render setting ")) + Name);
        }
        return true;
    }

    inline bool Build(IAssetTools& Tools, bool bProduction = false)
    {
        using namespace APSAtmosphereTailAssets;
        const TCHAR* TargetDestination = bProduction ? APSAtmosphereTailMaterial::Destination : Destination;
        const TCHAR* TargetPath = bProduction ? APSAtmosphereTailMaterial::MasterPath : MasterPath;
        const TCHAR* ParameterName = bProduction ? APSAtmosphereTailMaterial::TailParameter : TailParameter;
        const float TargetDefault = bProduction ? CandidateValue : ControlValue;
        const auto Refuse = [](const FString& Error)
        { UE_LOG(LogTemp, Error, TEXT("[APS.AtmosphereTail.Bake] Refused: %s"), *Error); return false; };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline commandlet only"));
        FString Error;
        if (!VerifySources(Error)) return Refuse(Error);
        if (IFileManager::Get().FileExists(*FPackageName::LongPackageNameToFilename(
            FString(TargetPath).Left(FString(TargetPath).Find(TEXT("."))), FPackageName::GetAssetPackageExtension())))
            return Refuse(TEXT("Immutable atmosphere destination already exists"));
        auto* Source = LoadObject<UMaterial>(nullptr, SourceMasterPath);
        if (!Source || Source->GetOutermost()->IsDirty() || !Source->bUseMaterialAttributes)
            return Refuse(TEXT("Saved exact material-attributes source required"));

        TArray<UObject*> Pending{Source};
        TSet<UObject*> Seen;
        TMap<UObject*, TArray<UMaterialExpression*>> Graphs;
        FString Problems;
        for (int32 I = 0; I < Pending.Num(); ++I)
        {
            UObject* Owner = Pending[I];
            if (Seen.Contains(Owner)) continue;
            if (!Owner || Seen.Num() >= 128) return Refuse(TEXT("Invalid/oversized function closure"));
            Seen.Add(Owner);
            const FString Package = Owner->GetOutermost()->GetName();
            bool bPinned = false;
            for (const FSource& Pin : Sources) bPinned |= Package == Pin.Package;
            if (!bPinned || Owner->GetOutermost()->IsDirty()) Problems += TEXT("Unpinned/dirty source: ") + Owner->GetPathName() + TEXT("\n");
            TArray<UMaterialExpression*> Nodes;
            if (!Graph(Owner, Nodes, Error)) return Refuse(Error);
            Graphs.Add(Owner, Nodes);
            const auto Serialized = FBuild::Expressions(Owner);
            int32 Missing = 0;
            for (UMaterialExpression* E : Nodes)
            {
                if (!Serialized.Contains(E))
                {
                    ++Missing;
                    UE_LOG(LogTemp, Display, TEXT("[APS.AtmosphereTail.Bake] missing owner=%s node=%s productionFunction=%d"),
                        *Owner->GetPathName(), *E->GetPathName(), Owner != Source);
                    if (Owner != Source) Problems += TEXT("Incomplete production function: ") + E->GetPathName() + TEXT("\n");
                }
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    auto* Function = Cast<UMaterialFunction>(Call->MaterialFunction);
                    if (!Function) { Problems += TEXT("Null/function-instance call: ") + E->GetPathName() + TEXT("\n"); continue; }
                    Pending.AddUnique(Function);
                    TArray<FFunctionExpressionInput> Inputs;
                    TArray<FFunctionExpressionOutput> Outputs;
                    Function->GetInputsAndOutputs(Inputs, Outputs);
                    bool bPinsMatch = Inputs.Num() == Call->FunctionInputs.Num() && Outputs.Num() == Call->FunctionOutputs.Num();
                    for (int32 P = 0; P < Inputs.Num() && bPinsMatch; ++P)
                        bPinsMatch = Inputs[P].ExpressionInput && Inputs[P].ExpressionInputId == Call->FunctionInputs[P].ExpressionInputId
                            && Inputs[P].Input.InputName == Call->FunctionInputs[P].Input.InputName;
                    for (int32 P = 0; P < Outputs.Num() && bPinsMatch; ++P)
                        bPinsMatch = Outputs[P].ExpressionOutput && Outputs[P].ExpressionOutputId == Call->FunctionOutputs[P].ExpressionOutputId
                            && Outputs[P].Output.OutputName == Call->FunctionOutputs[P].Output.OutputName;
                    if (!bPinsMatch) Problems += FString::Printf(TEXT("Function pin inventory mismatch: %s saved=%d/%d actual=%d/%d\n"),
                        *Call->GetPathName(), Call->FunctionInputs.Num(), Call->FunctionOutputs.Num(), Inputs.Num(), Outputs.Num());
                }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.AtmosphereTail.Bake] closure=%s serialized=%d linked=%d missing=%d dirty=%d"),
                *Owner->GetPathName(), Serialized.Num(), Nodes.Num(), Missing, Owner->GetOutermost()->IsDirty());
        }
        if (!Problems.IsEmpty()) return Refuse(Problems); // Report the entire audited closure; repair none of it.

        const auto* Root = Source->GetExpressionInputForProperty(MP_MaterialAttributes);
        TArray<UMaterialExpression*> Active;
        if (!Root || !Root->Expression || !Collect(Source, {Root->Expression}, Active, Error))
            return Refuse(TEXT("Active material attributes closure missing: ") + Error);
        UMaterialExpressionCustom* Density = nullptr;
        UMaterialExpressionScalarParameter* Radius = nullptr;
        int32 DensityCount = 0, RadiusCount = 0;
        for (UMaterialExpression* E : Active)
        {
            if (auto* C = Cast<UMaterialExpressionCustom>(E); C && C->GetName() == TEXT("MaterialExpressionCustom_0"))
            { Density = C; ++DensityCount; }
            if (E->HasAParameterName() && E->GetParameterName() == TEXT("AtmosRadius"))
            { Radius = Cast<UMaterialExpressionScalarParameter>(E); ++RadiusCount; }
        }
        if (DensityCount != 1 || !Density || Density->OutputType != CMOT_Float4
            || Density->MaterialExpressionGuid != FGuid(0x8BDCC041, 0x47E5D750, 0x66538A81, 0xB0CE6F7C)
            || RadiusCount != 1 || !Radius)
            return Refuse(TEXT("Ambiguous/changed active Custom_0 or AtmosRadius scalar"));
        for (UMaterialExpression* E : Graphs.FindChecked(Source))
            if (E->HasAParameterName() && E->GetParameterName() == TailParameter)
                return Refuse(TEXT("Diagnostic parameter already exists in source"));
        int32 EarthPins = 0, RadiusPins = 0;
        for (const FCustomInput& Input : Density->Inputs)
        {
            if (Input.InputName == TEXT("earth_radius"))
            {
                ++EarthPins;
                auto* Earth = Cast<UMaterialExpressionScalarParameter>(Input.Input.Expression);
                if (!Earth || Earth->ParameterName != TEXT("EarthRadius") || Input.Input.OutputIndex || Input.Input.Mask)
                    return Refuse(TEXT("Physical earth_radius binding changed"));
                if (!(Radius->DefaultValue > Earth->DefaultValue)) return Refuse(TEXT("Non-positive default physical shell height"));
            }
            if (Input.InputName == TEXT("AtmosRadius"))
            {
                ++RadiusPins;
                if (Input.Input.Expression != Radius || Input.Input.OutputIndex || Input.Input.Mask)
                    return Refuse(TEXT("Existing AtmosRadius custom input is ambiguous"));
            }
            if (Input.InputName == TailParameter) return Refuse(TEXT("Diagnostic custom input already exists"));
        }
        if (EarthPins != 1 || RadiusPins > 1) return Refuse(TEXT("Ambiguous physical radius inputs"));
        FString CandidateCode;
        if (!MakeCode(Density->Code, CandidateCode, Error)) return Refuse(Error);
        if (bProduction) CandidateCode.ReplaceInline(TailParameter, ParameterName, ESearchCase::CaseSensitive);

        FBuild B(Tools, TargetDestination);
        auto* Master = Cast<UMaterial>(B.Duplicate(Source, TEXT("M_APS_AtmosphereTail")));
        if (!Master || Master->GetPathName() != TargetPath || B.Outputs.Num() != 1) return Refuse(B.Error);
        TArray<UMaterialExpression*> Nodes;
        if (!Graph(Master, Nodes, Error)) return Refuse(Error);
        UMaterialExpressionCustom* CopyDensity = nullptr;
        UMaterialExpressionScalarParameter* CopyRadius = nullptr;
        int32 Registered = 0, Hydrated = 0;
        for (UMaterialExpression* E : Nodes)
        {
            Registered += !FBuild::Expressions(Master).Contains(E);
            Master->GetExpressionCollection().AddExpression(E);
            if (E->GetPathName(Master) == Density->GetPathName(Source)) CopyDensity = Cast<UMaterialExpressionCustom>(E);
            if (E->GetPathName(Master) == Radius->GetPathName(Source)) CopyRadius = Cast<UMaterialExpressionScalarParameter>(E);
            if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
            {
                // Explicit OWN-call hydration only. Never traverse/write function
                // nodes, update function resources, or change serialized pin data.
                TArray<FFunctionExpressionInput> Inputs;
                TArray<FFunctionExpressionOutput> Outputs;
                Call->MaterialFunction->GetInputsAndOutputs(Inputs, Outputs);
                if (Inputs.Num() != Call->FunctionInputs.Num() || Outputs.Num() != Call->FunctionOutputs.Num())
                    return Refuse(TEXT("Duplicate function inventory changed"));
                for (int32 P = 0; P < Inputs.Num(); ++P)
                {
                    if (Inputs[P].ExpressionInputId != Call->FunctionInputs[P].ExpressionInputId) return Refuse(TEXT("Duplicate input GUID changed"));
                    Call->FunctionInputs[P].ExpressionInput = Inputs[P].ExpressionInput;
                }
                for (int32 P = 0; P < Outputs.Num(); ++P)
                {
                    if (Outputs[P].ExpressionOutputId != Call->FunctionOutputs[P].ExpressionOutputId) return Refuse(TEXT("Duplicate output GUID changed"));
                    Call->FunctionOutputs[P].ExpressionOutput = Outputs[P].ExpressionOutput;
                }
                ++Hydrated;
            }
        }
        if (!CopyDensity || !CopyRadius || !VerifyCopy(Source, Master, Graphs.FindChecked(Source), nullptr, FString(), 0, Error))
            return Refuse(TEXT("Before density edit: ") + Error);
        auto* Switch = B.Add<UMaterialExpressionScalarParameter>(Master);
        if (!Switch) return Refuse(TEXT("Could not create diagnostic scalar"));
        Switch->ParameterName = ParameterName; Switch->DefaultValue = TargetDefault;
        Switch->Group = bProduction ? TEXT("APS Atmosphere") : TEXT("APS Isolated Diagnostics");
        Switch->UpdateParameterGuid(true, true);
        Master->GetExpressionCollection().AddExpression(Switch);
        const auto AddInput = [&](const TCHAR* Name, UMaterialExpression* Expression)
        { FCustomInput Input; Input.InputName = Name; Input.Input.Expression = Expression; CopyDensity->Inputs.Add(Input); };
        if (!RadiusPins) AddInput(TEXT("AtmosRadius"), CopyRadius);
        AddInput(ParameterName, Switch);
        CopyDensity->Code = CandidateCode;
        Master->PostEditChange();
        if (!VerifyCopy(Source, Master, Graphs.FindChecked(Source), CopyDensity, CandidateCode, RadiusPins ? 1 : 2, Error))
            return Refuse(Error);
        if (!Graph(Master, Nodes, Error)) return Refuse(Error);
        const auto Serialized = FBuild::Expressions(Master);
        for (UMaterialExpression* E : Nodes)
            if (!Serialized.Contains(E)) return Refuse(TEXT("Owned material still has an unregistered node: ") + E->GetPathName());

        // One master, compiled and completed before any save. Scalar 0/1 shares
        // the same shader map; no static permutation or production bake is used.
        Master->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        auto* Resource = Master->GetMaterialResource(GMaxRHIFeatureLevel);
        const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        if (Resource) for (const FString& CompileError : Resource->GetCompileErrors())
            UE_LOG(LogTemp, Error, TEXT("[APS.AtmosphereTail.Bake] compileError=%s"), *CompileError);
        UE_LOG(LogTemp, Display, TEXT("[APS.AtmosphereTail.Bake] compile resource=%d complete=%d errors=%d map=%d localVF=%d"),
            Resource != nullptr, Resource && Resource->IsGameThreadShaderMapComplete(), Resource ? Resource->GetCompileErrors().Num() : -1,
            Map != nullptr, Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType));
        if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
            || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            return Refuse(TEXT("Candidate shader incomplete/errors/missing LocalVF"));
        float Default = -1;
        if (!Master->GetScalarParameterValue(FMaterialParameterInfo(ParameterName), Default) || Default != TargetDefault)
            return Refuse(TEXT("Tail scalar default readback mismatch"));
        if (!VerifyCopy(Source, Master, Graphs.FindChecked(Source), CopyDensity, CandidateCode, RadiusPins ? 1 : 2, Error)
            || !VerifySources(Error)) return Refuse(Error);
        for (UObject* Owner : Seen)
            if (Owner->GetOutermost()->IsDirty()) return Refuse(TEXT("Source became dirty: ") + Owner->GetPathName());
        UPackage* Package = Master->GetOutermost();
        const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Master, *File, Args)) return Refuse(TEXT("Owned master save failed: ") + File);
        UE_LOG(LogTemp, Display, TEXT("[APS.AtmosphereTail.Bake] READY master=%s SHA1=%s outputs=1 registered=%d ownedCallHydrations=%d productionFunctionChanges=0 default=%g densityTerms=4; cold reload and rendered comparison still required"),
            *Master->GetPathName(), *Hash(Package->GetName()), Registered, Hydrated, TargetDefault);
        return true;
    }
}
#endif
