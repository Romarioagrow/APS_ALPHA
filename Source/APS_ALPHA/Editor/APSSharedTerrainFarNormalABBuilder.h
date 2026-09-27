#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "LocalVertexFactory.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"

// Diagnostic AB3, derived from the already baked AB2. No existing assets,
// selectors, texture-normal branches, palette or climate parameters are changed.
namespace APSSharedTerrainFarNormalABBuilder
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodFarNormalAB");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodFarNormalAB/MI_APS_LodFarNormalTerra.MI_APS_LodFarNormalTerra");

    struct FBuilder
    {
        APSSharedTerrainMaterialBuilder::FBuild B;
        TMap<UMaterialFunction*, bool> Needed;
        TSet<UMaterialFunction*> Visiting;
        TMap<UMaterialFunction*, UMaterialFunction*> Copies;
        int32 MasterInputs = 0, FunctionInputs = 0, NormalOwners = 0;
        explicit FBuilder(IAssetTools& Tools) : B(Tools, Destination) {}

        TArray<UMaterialExpression*> Graph(UObject* Owner)
        {
            // Saved duplicate functions can have reachable spatial adapters that
            // are absent from ExpressionCollection (two EditorOnlyData exports).
            // Follow exactly their serialized input graph, as compilation does;
            // do not enumerate arbitrary orphan subobjects or cross an owner.
            TArray<UMaterialExpression*> Pending = B.Expressions(Owner), Result;
            TSet<UMaterialExpression*> Seen;
            for (int32 Index = 0; Index < Pending.Num(); ++Index)
            {
                UMaterialExpression* E = Pending[Index];
                if (!E || Seen.Contains(E)) continue;
                if (Seen.Num() >= 4096 || !E->IsIn(Owner))
                { B.Error = TEXT("Expression closure exceeded bound or escaped owner: ") + Owner->GetPathName(); return {}; }
                Seen.Add(E); Result.Add(E);
                for (FExpressionInput* Input : E->GetInputsView())
                    if (Input && Input->Expression && !Seen.Contains(Input->Expression)) Pending.Add(Input->Expression);
            }
            return Result;
        }

        static FCustomInput* NormalInput(UMaterialExpression* E)
        {
            auto* C = Cast<UMaterialExpressionCustom>(E);
            if (!C || C->Description != TEXT("APS rendered vector to planet frame")) return nullptr;
            for (FCustomInput& I : C->Inputs)
                if (I.InputName == TEXT("V") && Cast<UMaterialExpressionVertexNormalWS>(I.Input.Expression)) return &I;
            return nullptr;
        }

        bool NeedsCopy(UMaterialFunction* F)
        {
            if (const bool* Found = Needed.Find(F)) return *Found;
            if (Visiting.Contains(F)) { B.Error = TEXT("Recursive material function"); return false; }
            Visiting.Add(F);
            const auto Expressions = Graph(F);
            if (!B.Error.IsEmpty()) return false;
            bool Result = false;
            int32 RawNormals = 0, Adapters = 0;
            for (UMaterialExpression* E : Expressions)
            {
                RawNormals += Cast<UMaterialExpressionVertexNormalWS>(E) ? 1 : 0;
                Adapters += NormalInput(E) ? 1 : 0;
                Result |= NormalInput(E) != nullptr;
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    if (auto* Child = Cast<UMaterialFunction>(Call->MaterialFunction)) Result |= NeedsCopy(Child);
                    else if (Call->MaterialFunction) B.Error = TEXT("Function instance requires audit");
                }
            }
            Visiting.Remove(F);
            Needed.Add(F, Result);
            UE_LOG(LogTemp, Display, TEXT("[APS.FarNormalAB] Dependency path=%s registered=%d closure=%d rawNormals=%d exactAdapters=%d needsCopy=%d"),
                *F->GetPathName(), B.Expressions(F).Num(), Expressions.Num(), RawNormals, Adapters, Result ? 1 : 0);
            return Result;
        }

        UMaterialExpression* MakeFilter(UObject* Owner, UMaterialExpression* NativeNormal)
        {
            // Reuse the existing DoubleVector parameters, including their GUIDs.
            UMaterialExpressionDoubleVectorParameter* Center = nullptr;
            UMaterialExpressionDoubleVectorParameter* Scale = nullptr;
            for (UMaterialExpression* E : Graph(Owner))
                if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E))
                {
                    if (P->ParameterName == TEXT("APS_SharedPlanetCenter")) Center = P;
                    if (P->ParameterName == TEXT("APS_SharedInverseScale")) Scale = P;
                }
            if (!Center || !Scale)
            { B.Error = TEXT("Existing physical frame missing: ") + Owner->GetPathName(); return nullptr; }
            auto* World = B.Add<UMaterialExpressionWorldPosition>(Owner);
            World->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
            auto* Camera = B.Add<UMaterialExpressionCameraPositionWS>(Owner);
            auto* Relative = B.Add<UMaterialExpressionSubtract>(Owner);
            Relative->A.Expression = World; Relative->B.Expression = Center;
            Relative->B.Mask = 1; Relative->B.MaskR = Relative->B.MaskG = Relative->B.MaskB = 1;
            auto* CameraDelta = B.Add<UMaterialExpressionSubtract>(Owner);
            CameraDelta->A.Expression = Camera; CameraDelta->B.Expression = World;
            auto Scalar = [&](const TCHAR* Name, float Default)
            {
                auto* P = B.Add<UMaterialExpressionScalarParameter>(Owner);
                P->ParameterName = Name; P->DefaultValue = Default;
                P->Group = TEXT("APS Diagnostic Far Normal"); P->UpdateParameterGuid(true, true);
                return P;
            };
            // Experimental 200..700 km, NOT an accepted production tuning.
            // cm are physical: all presentation scales share the same interval.
            auto* Start = Scalar(TEXT("APS_FarNormalStartCm"), 20000000.0f);
            auto* End = Scalar(TEXT("APS_FarNormalEndCm"), 70000000.0f);
            auto* Filter = B.Add<UMaterialExpressionCustom>(Owner);
            Filter->Description = TEXT("APS AB3 physical-distance geometric normal only");
            Filter->OutputType = CMOT_Float3;
            // LWC subtraction happens before Custom's float demotion. Never pass
            // absolute world/camera coordinates into Custom. Texture coordinates
            // retain the original LWC graph, this node processes directions only.
            Filter->Code = TEXT(
                "float d=length(CameraDelta.xyz)*max(InverseScale.x,0.0);\n"
                "float t=saturate((d-StartCm)/max(EndCm-StartCm,1.0));\n"
                "if(t<=0.0) return NativeNormal.xyz;\n"
                "float r2=dot(Relative.xyz,Relative.xyz);\n"
                "if(r2<1.e-8) return NativeNormal.xyz;\n"
                "float3 radial=Relative.xyz*rsqrt(r2);\n"
                "if(t>=1.0) return radial;\n"
                "float w=t*t*(3.0-2.0*t);\n"
                "float3 n=lerp(NativeNormal.xyz,radial,w);\n"
                "return n*rsqrt(max(dot(n,n),1.e-8));");
            Filter->Inputs.Reset();
            const auto Input = [&](const TCHAR* Name, UMaterialExpression* E)
            { FCustomInput I; I.InputName = Name; I.Input.Expression = E; Filter->Inputs.Add(I); };
            Input(TEXT("NativeNormal"), NativeNormal); Input(TEXT("Relative"), Relative);
            Input(TEXT("CameraDelta"), CameraDelta); Input(TEXT("InverseScale"), Scale);
            Input(TEXT("StartCm"), Start); Input(TEXT("EndCm"), End);
            return Filter;
        }

        bool Patch(UObject* Owner)
        {
            const auto Original = Graph(Owner);
            if (!B.Error.IsEmpty()) return false;
            UMaterialExpression* Filter = nullptr;
            int32 Patched = 0;
            for (UMaterialExpression* E : Original)
            {
                if (FCustomInput* Input = NormalInput(E))
                {
                    if (Input->Input.OutputIndex != 0 || Input->Input.Mask)
                    { B.Error = TEXT("Unexpected normal adapter swizzle"); return false; }
                    if (!Filter) Filter = MakeFilter(Owner, Input->Input.Expression);
                    if (!Filter) return false;
                    // Only replace the geometric input. The original direction
                    // rotation, RNM texture normal, slope thresholds and palette
                    // remain wired exactly as in AB2.
                    Input->Input.Expression = Filter; ++Patched;
                }
                else if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    auto* Source = Cast<UMaterialFunction>(Call->MaterialFunction);
                    if (!Source || !NeedsCopy(Source)) continue;
                    if (!B.Error.IsEmpty()) return false;
                    UMaterialFunction* Copy = Copies.FindRef(Source);
                    if (!Copy)
                    {
                        const FString Name = FString::Printf(TEXT("MF_FarNormal_%08x"), FCrc::StrCrc32(*Source->GetPathName()));
                        Copy = Cast<UMaterialFunction>(B.Duplicate(Source, Name));
                        if (!Copy) return false;
                        Copies.Add(Source, Copy);
                        if (!Patch(Copy)) return false;
                        UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
                    }
                    if (!B.ReconnectFunctionById(Call, Source, Copy)) return false;
                }
            }
            if (Patched) ++NormalOwners;
            if (Cast<UMaterial>(Owner)) MasterInputs += Patched; else FunctionInputs += Patched;
            UE_LOG(LogTemp, Display, TEXT("[APS.FarNormalAB] owner=%s geometricInputs=%d"), *Owner->GetPathName(), Patched);
            return B.Error.IsEmpty();
        }

        bool Run()
        {
            auto* Source = LoadObject<UMaterial>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain"));
            auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra"));
            if (!Source || !Template || Template->Parent != Source || Source->GetOutermost()->IsDirty() || Template->GetOutermost()->IsDirty())
            { B.Error = TEXT("Saved AB2 master and exact Terra template required"); return false; }
            auto* Master = Cast<UMaterial>(B.Duplicate(Source, TEXT("M_APS_LodFarNormalTerrain")));
            if (!Master || !Patch(Master)) return false;
            if (MasterInputs != 3 || FunctionInputs != 3 || NormalOwners != 3)
            { B.Error = FString::Printf(TEXT("Normal graph drift: expected 3 master + 3 function inputs in 3 owners, got %d/%d/%d"), MasterInputs, FunctionInputs, NormalOwners); return false; }
            auto* Instance = Cast<UMaterialInstanceConstant>(B.Duplicate(Template, TEXT("MI_APS_LodFarNormalTerra")));
            if (!Instance) return false;
            Instance->SetParentEditorOnly(Master, false);
            Instance->CopyMaterialUniformParametersEditorOnly(Template, true);
            Instance->PostEditChange(); Master->PostEditChange();
            for (UObject* Output : B.Outputs)
                if (auto* M = Cast<UMaterialInterface>(Output)) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
            for (UObject* Output : B.Outputs)
                if (auto* M = Cast<UMaterialInterface>(Output))
                {
                    FMaterialResource* R = M->GetMaterialResource(GMaxRHIFeatureLevel);
                    const FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
                    if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                    { B.Error = TEXT("AB3 shader not complete/LocalVF ready: ") + M->GetPathName(); return false; }
                }
            for (UObject* Output : B.Outputs)
            {
                UPackage* P = Output->GetOutermost(); P->MarkPackageDirty();
                FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
                if (!UPackage::SavePackage(P, Output, *FPackageName::LongPackageNameToFilename(P->GetName(), FPackageName::GetAssetPackageExtension()), Args))
                { B.Error = TEXT("AB3 candidate save failed"); return false; }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.FarNormalAB] Candidate baked only: geometricInputs=6 distance=200..700 physical km outputs=%d; visual acceptance pending"), B.Outputs.Num());
            return true;
        }
    };

    inline bool Build(IAssetTools& Tools)
    {
        FBuilder Builder(Tools);
        const bool Result = Builder.Run();
        if (!Result) UE_LOG(LogTemp, Error, TEXT("[APS.FarNormalAB] Refused: %s"), *Builder.B.Error);
        return Result;
    }
}
#endif
