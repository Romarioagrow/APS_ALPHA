#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Offline one-time precision and bounded-detiling update: save only the
// existing terrain WAT function. APS_SharedDetailWarpStrength=0 retains the
// compensated precision mapping with no warp; the shader clamps strength 0..1.
// The master, both MICs, slope and macro/map functions retain their bytes.
namespace APSSharedTerrainDetailPrecisionUpdate
{
    struct FExpected { const TCHAR* Name; const TCHAR* SHA1; };
    inline constexpr FExpected Expected[] = {
        {TEXT("M_APS_SharedWorldScapeTerrain"), TEXT("CB0E7DF1BB20A94964EF2AFCA560E0870AC28646")},
        {TEXT("MF_APS_MF_MacroVariationBlock_50169517"), TEXT("3FD8F36F4DC8A117BFFD9A8E1978CC8126137BBC")},
        {TEXT("MF_APS_MF_PlanetMap_80d6e0bd"), TEXT("2957DE0FCC842614B088FCBBD30A7F3F887B2B3B")},
        {TEXT("MF_APS_MF_SlopeBlock_f6be5407"), TEXT("3FF327494FFB83C3521A5BA1318FA913C3303AC2")},
        {TEXT("MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("690852F8A2C69B7CC618F161AC7D8672EF7CABDD")},
        {TEXT("MI_APS_SharedMagma"), TEXT("A198333D905DA52AA96FFFC3EBF988690062AAB5")},
        {TEXT("MI_APS_SharedTerra"), TEXT("D05DBA6349ED39245B7BE1A0937F8225DBC84598")}
    };
    inline FString Filename(const FExpected& E)
    {
        return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
            FString(APSSharedTerrainMaterialBuilder::OutputRoot) / E.Name, FPackageName::GetAssetPackageExtension()));
    }
    inline FString Hash(const FString& File)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *File)) return FString();
        return FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().ToUpper();
    }

    // Compare only the arithmetic used by the existing physical-frame builder.
    // Never accept an arbitrary authored position override merely by node name.
    // Depth bounds also reject cyclic/unsupported graphs without mutating them.
    inline bool SameFrame(const FExpressionInput& A, const FExpressionInput& B, int32 Depth = 0)
    {
        if (Depth > 32 || A.OutputIndex != B.OutputIndex || A.Mask != B.Mask
            || (A.Mask && (A.MaskR != B.MaskR || A.MaskG != B.MaskG || A.MaskB != B.MaskB || A.MaskA != B.MaskA)))
            return false;
        const UMaterialExpression* X = A.Expression;
        const UMaterialExpression* Y = B.Expression;
        if (!X || !Y) return X == Y;
        if (X->GetClass() != Y->GetClass()) return false;
        const auto Pair = [Depth](const FExpressionInput& P, const FExpressionInput& Q)
        { return SameFrame(P, Q, Depth + 1); };
        if (const auto* P = Cast<UMaterialExpressionAppendVector>(X))
        { const auto* Q = CastChecked<UMaterialExpressionAppendVector>(Y); return Pair(P->A, Q->A) && Pair(P->B, Q->B); }
        if (const auto* P = Cast<UMaterialExpressionAdd>(X))
        { const auto* Q = CastChecked<UMaterialExpressionAdd>(Y); return P->ConstA == Q->ConstA && P->ConstB == Q->ConstB && Pair(P->A, Q->A) && Pair(P->B, Q->B); }
        if (const auto* P = Cast<UMaterialExpressionSubtract>(X))
        { const auto* Q = CastChecked<UMaterialExpressionSubtract>(Y); return P->ConstA == Q->ConstA && P->ConstB == Q->ConstB && Pair(P->A, Q->A) && Pair(P->B, Q->B); }
        if (const auto* P = Cast<UMaterialExpressionMultiply>(X))
        { const auto* Q = CastChecked<UMaterialExpressionMultiply>(Y); return P->ConstA == Q->ConstA && P->ConstB == Q->ConstB && Pair(P->A, Q->A) && Pair(P->B, Q->B); }
        if (const auto* P = Cast<UMaterialExpressionComponentMask>(X))
        { const auto* Q = CastChecked<UMaterialExpressionComponentMask>(Y); return P->R == Q->R && P->G == Q->G && P->B == Q->B && P->A == Q->A && Pair(P->Input, Q->Input); }
        if (const auto* P = Cast<UMaterialExpressionWorldPosition>(X))
            return P->WorldPositionShaderOffset == CastChecked<UMaterialExpressionWorldPosition>(Y)->WorldPositionShaderOffset;
        if (const auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(X))
        { const auto* Q = CastChecked<UMaterialExpressionDoubleVectorParameter>(Y); return P->ParameterName == Q->ParameterName && P->DefaultValue == Q->DefaultValue; }
        return false;
    }

    inline bool IsEquivalentMacroPosition(const FExpressionInput& Binding,
        const FExpressionInput& Default, UObject* Macro)
    {
        if (Binding.OutputIndex != 0 || Binding.Mask) return false;
        const auto* Subtract = Cast<UMaterialExpressionSubtract>(Binding.Expression);
        const auto* Zero = Subtract ? Cast<UMaterialExpressionConstant3Vector>(Subtract->B.Expression) : nullptr;
        if (!Subtract || Subtract->GetOuter() != Macro || !Zero || Zero->GetOuter() != Macro
            || Zero->Constant.R != 0 || Zero->Constant.G != 0 || Zero->Constant.B != 0
            || Subtract->B.OutputIndex != 0
            || (Subtract->B.Mask && (!Subtract->B.MaskR || !Subtract->B.MaskG || !Subtract->B.MaskB || Subtract->B.MaskA)))
            return false;
        // The native macro explicitly subtracts the virtual planet origin (0).
        // RGB on a verified append(xy,z) is an identity, not a changed frame.
        FExpressionInput Position = Subtract->A;
        if (!Cast<UMaterialExpressionAppendVector>(Position.Expression)
            || Position.OutputIndex != 0
            || (Position.Mask && (!Position.MaskR || !Position.MaskG || !Position.MaskB || Position.MaskA)))
            return false;
        Position.Mask = Position.MaskR = Position.MaskG = Position.MaskB = Position.MaskA = 0;
        return SameFrame(Position, Default);
    }

    inline bool Run(IAssetTools& Tools)
    {
        using FBuild = APSSharedTerrainMaterialBuilder::FBuild;
        FBuild B(Tools);
        const auto Refuse = [](const FString& Error)
        { UE_LOG(LogTemp, Error, TEXT("[APS.TerrainDetailPrecisionUpdate] Refused: %s"), *Error); return false; };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline protected commandlet only"));
        TArray<UObject*> Objects;
        for (const FExpected& E : Expected)
        {
            const FString File = Filename(E);
            if (Hash(File) != E.SHA1) return Refuse(TEXT("Accepted baseline changed: ") + File);
            if (IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("uexp")))
                || IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("ubulk"))))
                return Refuse(TEXT("Unexpected package sidecar: ") + File);
            const FString Path = FString(APSSharedTerrainMaterialBuilder::OutputRoot) / E.Name + TEXT(".") + E.Name;
            UObject* Object = LoadObject<UObject>(nullptr, *Path);
            if (!Object || Object->GetOutermost()->IsDirty() || Object->GetOutermost()->bIsCookedForEditor)
                return Refuse(TEXT("Missing, dirty or cooked shared object: ") + Path);
            Objects.Add(Object);
        }
        auto* Master = Cast<UMaterial>(Objects[0]);
        auto* Detail = Cast<UMaterialFunction>(Objects[4]);
        auto* Magma = Cast<UMaterialInstanceConstant>(Objects[5]);
        auto* Terra = Cast<UMaterialInstanceConstant>(Objects[6]);
        if (!Master || !Detail || !Magma || !Terra || Magma->Parent != Master || Terra->Parent != Master
            || !FBuild::IsPreciseDetailFunction(Detail) || Master->bTangentSpaceNormal)
            return Refuse(TEXT("Shared parent/function contract changed"));

        // Follow input closure because saved copied functions can contain
        // connected physical-frame nodes outside their flat expression array.
        APSSharedTerrainNormalContinuity::TTransform<FBuild> Reader(B);
        const TArray<UMaterialExpression*> DetailGraph = Reader.Graph(Detail);
        if (!B.Error.IsEmpty()) return Refuse(B.Error);
        UMaterialExpressionDoubleVectorParameter* Center = nullptr;
        UMaterialExpressionFunctionInput* PositionInput = nullptr;
        int32 Centers = 0, PositionInputs = 0;
        for (UMaterialExpression* E : DetailGraph)
        {
            if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E);
                P && P->ParameterName == TEXT("APS_SharedPlanetCenter")) { Center = P; ++Centers; }
            if (auto* P = Cast<UMaterialExpressionFunctionInput>(E);
                P && P->InputName == TEXT("WorldPosition")) { PositionInput = P; ++PositionInputs; }
        }
        if (Centers != 1 || PositionInputs != 1)
            return Refuse(TEXT("Expected one existing physical center and WorldPosition input"));

        TArray<UObject*> Pending{Master};
        TSet<UObject*> Visited;
        int32 DetailCalls = 0, EquivalentMacroCalls = 0;
        for (int32 Index = 0; Index < Pending.Num(); ++Index)
        {
            UObject* Owner = Pending[Index];
            if (Visited.Contains(Owner)) continue;
            if (Visited.Num() >= 128) return Refuse(TEXT("Function graph exceeds preflight bound"));
            Visited.Add(Owner);
            for (UMaterialExpression* E : Reader.Graph(Owner))
            {
                auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E);
                if (!Call || !Call->MaterialFunction) continue;
                auto* Function = Cast<UMaterialFunction>(Call->MaterialFunction);
                if (!Function) return Refuse(TEXT("Function instance requires explicit audit"));
                // Graph() includes connected nodes absent from the legacy flat
                // expression list. UE's UpdateTransientExpressionData only walks
                // that list, leaving these calls' transient pin pointers null.
                // Rehydrate by serialized GUID; do NOT reconnect/edit any edge.
                TArray<FFunctionExpressionInput> Inputs;
                TArray<FFunctionExpressionOutput> Outputs;
                Function->GetInputsAndOutputs(Inputs, Outputs);
                if (Inputs.Num() != Call->FunctionInputs.Num() || Outputs.Num() != Call->FunctionOutputs.Num())
                    return Refuse(TEXT("Function pin inventory changed: ") + Call->GetPathName());
                for (FFunctionExpressionInput& Pin : Call->FunctionInputs)
                {
                    const auto* Source = Inputs.FindByPredicate([&](const FFunctionExpressionInput& P)
                    { return P.ExpressionInputId == Pin.ExpressionInputId; });
                    if (!Source || !Source->ExpressionInput || Source->Input.InputName != Pin.Input.InputName)
                        return Refuse(TEXT("Function input GUID/name changed: ") + Call->GetPathName());
                    Pin.ExpressionInput = Source->ExpressionInput;
                }
                for (FFunctionExpressionOutput& Pin : Call->FunctionOutputs)
                {
                    const auto* Source = Outputs.FindByPredicate([&](const FFunctionExpressionOutput& P)
                    { return P.ExpressionOutputId == Pin.ExpressionOutputId; });
                    if (!Source || !Source->ExpressionOutput || Source->Output.OutputName != Pin.Output.OutputName)
                        return Refuse(TEXT("Function output GUID/name changed: ") + Call->GetPathName());
                    Pin.ExpressionOutput = Source->ExpressionOutput;
                }
                Pending.AddUnique(Function);
                if (Function != Detail) continue;
                const FFunctionExpressionInput* Binding = Call->FunctionInputs.FindByPredicate(
                    [&](const FFunctionExpressionInput& Input) { return Input.ExpressionInputId == PositionInput->Id; });
                if (!Binding) return Refuse(TEXT("Missing WAT position binding: ") + Call->GetPathName());
                if (!Binding->Input.Expression) { ++DetailCalls; continue; }
                const FString Name = Call->GetName();
                if (Owner != Objects[1] || (Name != TEXT("MaterialExpressionMaterialFunctionCall_16")
                    && Name != TEXT("MaterialExpressionMaterialFunctionCall_17")
                    && Name != TEXT("MaterialExpressionMaterialFunctionCall_18"))
                    || !IsEquivalentMacroPosition(Binding->Input, PositionInput->Preview, Owner))
                    return Refuse(TEXT("Non-equivalent WAT position override: ") + Call->GetPathName());
                ++EquivalentMacroCalls;
            }
            if (!B.Error.IsEmpty()) return Refuse(B.Error);
        }
        if (DetailCalls != 18) return Refuse(TEXT("Expected the audited sixteen master plus two slope WAT calls"));
        if (EquivalentMacroCalls != 3) return Refuse(TEXT("Expected three equivalent macro physical-frame calls"));
        UE_LOG(LogTemp, Display, TEXT("[APS.TerrainDetailPrecisionUpdate] Position preflight: defaultCalls=18 structurallyEquivalentMacroCalls=3; arbitrary overrides remain refused"));

        const FString Backup = FPaths::ConvertRelativePathToFull(
            FPaths::ProjectSavedDir() / TEXT("SharedTerrainDetailPrecisionBackup"));
        const bool bReuseBackup = IFileManager::Get().DirectoryExists(*Backup);
        if (bReuseBackup && !FParse::Param(FCommandLine::Get(), TEXT("ResumeVerifiedTerrainPrecisionBackup")))
            return Refuse(TEXT("Backup exists; inspect previous attempt: ") + Backup);
        if (!bReuseBackup && !IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
        for (const FExpected& E : Expected)
        {
            const FString Source = Filename(E), Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
            if (Hash(Source) != E.SHA1 || (!bReuseBackup && IFileManager::Get().Copy(*Dest, *Source, false, false) != COPY_OK)
                || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup failed before graph mutation: ") + Source);
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.TerrainDetailPrecisionUpdate] All seven source and backup hashes verified; reuseImmutableBackup=%d"), bReuseBackup ? 1 : 0);

        // Reuse the existing center; never create a duplicate physical frame.
        if (!B.PatchDetailCoordinates(Detail, Center)) return Refuse(B.Error);
        UMaterialEditingLibrary::UpdateMaterialFunction(Detail);
        Master->PostEditChange();
        UMaterialInterface* Materials[] = {Master, Magma, Terra};
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
                return Refuse(TEXT("Shader/LocalVF incomplete; no package saved: ") + Material->GetPathName());
        }
        for (const FExpected& E : Expected)
            if (Hash(Filename(E)) != E.SHA1) return Refuse(TEXT("Concurrent package edit: ") + Filename(E));
        if (Magma->Parent != Master || Terra->Parent != Master) return Refuse(TEXT("MIC parents changed"));

        UPackage* Package = Detail->GetOutermost();
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Detail, *Filename(Expected[4]), Args))
            return Refuse(TEXT("WAT save failed; original available in ") + Backup);
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected); ++Index)
            if (Index != 4 && Hash(Filename(Expected[Index])) != Expected[Index].SHA1)
                return Refuse(TEXT("Protected package changed: ") + Filename(Expected[Index]));
        UE_LOG(LogTemp, Display, TEXT("[APS.TerrainDetailPrecisionUpdate] Saved WAT precision + bounded detiling only; six protected packages unchanged. WarpStrength defaults to 1 (0 disables warp). Backup=%s. No visual acceptance is implied."), *Backup);
        return true;
    }
}
#endif
