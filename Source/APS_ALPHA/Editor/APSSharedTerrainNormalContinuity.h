#pragma once

#if WITH_EDITOR
#include "Materials/Material.h"
#include "MaterialEditingLibrary.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDoubleVectorParameter.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"

// This transform is shared by a protected existing-asset update and future new
// shared-terrain bakes. It has no dependency on the diagnostic AB2/AB3 assets.
// TBuild supplies Add<T>(), Expressions(), Error and DestinationRoot only.
namespace APSSharedTerrainNormalContinuity
{
    inline constexpr const TCHAR* FilterDescription = TEXT("APS physical-distance geometric normal continuity v1");
    inline constexpr const TCHAR* SlopeDescription = TEXT("APS preserve native near slope; smooth far pixel slope v1");

    template<class TBuild> struct TTransform
    {
        TBuild& B;
        struct FOwner
        {
            UObject* Object = nullptr;
            TArray<UMaterialExpression*> Graph;
            TArray<UMaterialExpressionCustom*> Adapters;
        };
        struct FFilter
        {
            UMaterialExpression* Normal = nullptr;
            UMaterialExpression* CameraDelta = nullptr;
            UMaterialExpression* Scale = nullptr;
            UMaterialExpression* Start = nullptr;
            UMaterialExpression* End = nullptr;
        };
        TArray<FOwner> Owners;
        TSet<UObject*> Inspected;
        TArray<UObject*> Changed;
        TMap<UMaterialExpression*, bool> DependsCache;
        TSet<UMaterialExpression*> DependsActive;
        TMap<UMaterialExpression*, UMaterialExpression*> SlopeCopies;
        TArray<TPair<UMaterialExpressionVertexInterpolator*, FExpressionInput>> WarpInputs;
        int32 RewiredNormals = 0, MasterWarpVIs = 0, SlopePixelCopies = 0;
        explicit TTransform(TBuild& InB) : B(InB) {}

        bool Fail(const FString& Why) { B.Error = Why; return false; }

        TArray<UMaterialExpression*> Graph(UObject* Owner)
        {
            // Compilation follows input links, including saved coordinate nodes
            // missing from a duplicate function's flat ExpressionCollection.
            TArray<UMaterialExpression*> Pending = B.Expressions(Owner), Result;
            TSet<UMaterialExpression*> Seen;
            for (int32 I = 0; I < Pending.Num(); ++I)
            {
                UMaterialExpression* E = Pending[I];
                if (!E || Seen.Contains(E)) continue;
                if (Seen.Num() >= 4096 || !E->IsIn(Owner))
                { Fail(TEXT("Expression closure exceeded bound or owner: ") + Owner->GetPathName()); return {}; }
                Seen.Add(E); Result.Add(E);
                for (FExpressionInput* Input : E->GetInputsView())
                    if (Input && Input->Expression && !Seen.Contains(Input->Expression)) Pending.Add(Input->Expression);
            }
            return Result;
        }

        static FCustomInput* NormalInput(UMaterialExpression* E)
        {
            auto* C = Cast<UMaterialExpressionCustom>(E);
            if (!C || C->Description != TEXT("APS rendered vector to planet frame")
                || C->Code != TEXT("return float3(dot(V.xyz,X.xyz),dot(V.xyz,Y.xyz),dot(V.xyz,Z.xyz));")) return nullptr;
            for (FCustomInput& I : C->Inputs)
                if (I.InputName == TEXT("V") && Cast<UMaterialExpressionVertexNormalWS>(I.Input.Expression)) return &I;
            return nullptr;
        }

        static void Register(UObject* Owner, UMaterialExpression* E)
        {
            if (auto* M = Cast<UMaterial>(Owner)) M->GetExpressionCollection().AddExpression(E);
            else CastChecked<UMaterialFunction>(Owner)->GetExpressionCollection().AddExpression(E);
        }

        template<class T> T* Add(UObject* Owner)
        {
            T* E = B.template Add<T>(Owner);
            if (E) Register(Owner, E); // Explicitly register only our new nodes.
            return E;
        }

        bool Scan(UObject* Owner)
        {
            if (Inspected.Contains(Owner)) return true;
            if (Inspected.Num() >= 128) return Fail(TEXT("Function graph exceeded bound"));
            Inspected.Add(Owner);
            FOwner State; State.Object = Owner; State.Graph = Graph(Owner);
            if (!B.Error.IsEmpty()) return false;
            for (UMaterialExpression* E : State.Graph)
            {
                if (auto* C = Cast<UMaterialExpressionCustom>(E))
                    if (C->Description == FilterDescription || C->Description == SlopeDescription)
                        return Fail(TEXT("Continuity transform already present; refuse double patch"));
                if (FCustomInput* N = NormalInput(E))
                {
                    if (N->Input.OutputIndex || N->Input.Mask) return Fail(TEXT("Normal input swizzle changed"));
                    State.Adapters.Add(CastChecked<UMaterialExpressionCustom>(E));
                }
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    if (auto* F = Cast<UMaterialFunction>(Call->MaterialFunction)) { if (!Scan(F)) return false; }
                    else if (Call->MaterialFunction) return Fail(TEXT("Function instance requires audit"));
                }
            }
            if (State.Adapters.Num())
            {
                if (!Owner->GetOutermost()->GetName().StartsWith(B.DestinationRoot + TEXT("/")))
                    return Fail(TEXT("Refusing normal edit outside owned shared root: ") + Owner->GetPathName());
                Owners.Add(MoveTemp(State));
            }
            return true;
        }

        bool DependsOnNormal(UMaterialExpression* E)
        {
            if (!E) return false;
            if (const bool* Found = DependsCache.Find(E)) return *Found;
            if (DependsActive.Contains(E) || DependsCache.Num() + DependsActive.Num() >= 512)
            { Fail(TEXT("Slope dependency graph is cyclic or too large")); return false; }
            DependsActive.Add(E);
            bool Result = NormalInput(E) != nullptr;
            for (FExpressionInput* I : E->GetInputsView()) if (I && I->Expression) Result |= DependsOnNormal(I->Expression);
            DependsActive.Remove(E); DependsCache.Add(E, Result);
            return Result;
        }

        FFilter MakeFilter(const FOwner& Owner)
        {
            FFilter Out;
            UMaterialExpressionDoubleVectorParameter *Center = nullptr, *Scale = nullptr;
            int32 Centers = 0, Scales = 0;
            for (UMaterialExpression* E : Owner.Graph)
                if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E))
                {
                    if (P->ParameterName == TEXT("APS_SharedPlanetCenter")) { Center = P; ++Centers; }
                    if (P->ParameterName == TEXT("APS_SharedInverseScale")) { Scale = P; ++Scales; }
                }
            if (Centers != 1 || Scales != 1)
            { Fail(TEXT("Expected unique existing physical frame: ") + Owner.Object->GetPathName()); return Out; }
            UObject* O = Owner.Object;
            auto* World = Add<UMaterialExpressionWorldPosition>(O);
            World->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
            auto* Camera = Add<UMaterialExpressionCameraPositionWS>(O);
            auto* Relative = Add<UMaterialExpressionSubtract>(O);
            Relative->A.Expression = World; Relative->B.Expression = Center;
            Relative->B.Mask = 1; Relative->B.MaskR = Relative->B.MaskG = Relative->B.MaskB = 1;
            auto* Delta = Add<UMaterialExpressionSubtract>(O);
            Delta->A.Expression = Camera; Delta->B.Expression = World;
            const auto Scalar = [&](const TCHAR* Name, float Value)
            {
                auto* P = Add<UMaterialExpressionScalarParameter>(O);
                P->ParameterName = Name; P->DefaultValue = Value;
                P->Group = TEXT("APS Physical Normal Continuity"); P->UpdateParameterGuid(true, true); return P;
            };
            Out.CameraDelta = Delta; Out.Scale = Scale;
            Out.Start = Scalar(TEXT("APS_FarNormalStartCm"), 20000000.0f);
            Out.End = Scalar(TEXT("APS_FarNormalEndCm"), 70000000.0f);
            auto* Filter = Add<UMaterialExpressionCustom>(O);
            Filter->Description = FilterDescription; Filter->OutputType = CMOT_Float3;
            // Same AB3 geometric filter; subtraction precedes Custom's LWC
            // demotion. No texture-normal/output-normal blending or UV edits.
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
            Input(TEXT("NativeNormal"), NormalInput(Owner.Adapters[0])->Input.Expression);
            Input(TEXT("Relative"), Relative); Input(TEXT("CameraDelta"), Delta);
            Input(TEXT("InverseScale"), Scale); Input(TEXT("StartCm"), Out.Start); Input(TEXT("EndCm"), Out.End);
            Out.Normal = Filter;
            return Out;
        }

        UMaterialExpression* CloneSlope(UObject* Owner, UMaterialExpression* Source, UMaterialExpression* Filter)
        {
            if (!Source || !DependsOnNormal(Source)) return Source;
            if (UMaterialExpression** Found = SlopeCopies.Find(Source)) return *Found;
            if (SlopeCopies.Num() >= 64 || !Source->IsIn(Owner)
                || Cast<UMaterialExpressionVertexInterpolator>(Source))
            { Fail(TEXT("Unexpected slope dependent graph/vertex stage")); return nullptr; }
            auto* Copy = DuplicateObject<UMaterialExpression>(Source, Owner,
                MakeUniqueObjectName(Owner, Source->GetClass(), TEXT("APS_FarSlopePixel")));
            if (!Copy) { Fail(TEXT("Slope expression clone failed")); return nullptr; }
            Copy->MaterialExpressionGuid = FGuid::NewGuid(); Copy->GraphNode = nullptr;
            Register(Owner, Copy); SlopeCopies.Add(Source, Copy);
            if (auto* SourceCall = Cast<UMaterialExpressionMaterialFunctionCall>(Source))
            {
                auto* CopyCall = CastChecked<UMaterialExpressionMaterialFunctionCall>(Copy);
                // DuplicateObject intentionally drops transient private function
                // pin pointers. The function itself is unchanged, so restore
                // its exact GUID/pointer binding before copying input wires.
                CopyCall->MaterialFunction = SourceCall->MaterialFunction;
                CopyCall->FunctionInputs = SourceCall->FunctionInputs;
                CopyCall->FunctionOutputs = SourceCall->FunctionOutputs;
            }
            const auto OriginalInputs = Source->GetInputsView();
            const auto CopiedInputs = Copy->GetInputsView();
            if (OriginalInputs.Num() != CopiedInputs.Num())
            { Fail(TEXT("Slope clone input count changed")); return nullptr; }
            for (int32 I = 0; I < OriginalInputs.Num(); ++I)
            {
                if (!OriginalInputs[I] || !CopiedInputs[I]) { Fail(TEXT("Null slope pin")); return nullptr; }
                *CopiedInputs[I] = *OriginalInputs[I]; // Preserve pin names/masks/output indices exactly.
                CopiedInputs[I]->Expression = CloneSlope(Owner, OriginalInputs[I]->Expression, Filter);
                if (!B.Error.IsEmpty()) return nullptr;
            }
            if (FCustomInput* N = NormalInput(Copy)) { N->Input.Expression = Filter; ++RewiredNormals; }
            ++SlopePixelCopies;
            return Copy;
        }

        bool PatchSlope(const FOwner& Owner, const FFilter& Filter)
        {
            UMaterialExpressionVertexInterpolator* VI = nullptr;
            for (UMaterialExpression* E : Owner.Graph)
                if (auto* V = Cast<UMaterialExpressionVertexInterpolator>(E))
                { if (VI) return Fail(TEXT("Multiple slope interpolators")); VI = V; }
            if (!VI || !VI->Input.Expression || VI->Input.OutputIndex || VI->Input.Mask)
                return Fail(TEXT("Slope VI contract changed"));
            UMaterialExpression* OriginalSlope = VI->Input.Expression;
            UMaterialExpression* PixelSlope = CloneSlope(Owner.Object, VI->Input.Expression, Filter.Normal);
            if (!PixelSlope || !B.Error.IsEmpty()) return false;
            auto* Blend = Add<UMaterialExpressionCustom>(Owner.Object);
            Blend->Description = SlopeDescription; Blend->OutputType = CMOT_Float1;
            Blend->Code = TEXT(
                "float d=length(CameraDelta.xyz)*max(InverseScale.x,0.0);\n"
                "float t=saturate((d-StartCm)/max(EndCm-StartCm,1.0));\n"
                "if(t<=0.0) return NativeSlope;\n"
                "if(t>=1.0) return PixelSlope;\n"
                "float w=t*t*(3.0-2.0*t);\n"
                "return lerp(NativeSlope,PixelSlope,w);");
            Blend->Inputs.Reset();
            const auto Input = [&](const TCHAR* Name, UMaterialExpression* E)
            { FCustomInput I; I.InputName = Name; I.Input.Expression = E; Blend->Inputs.Add(I); };
            Input(TEXT("NativeSlope"), VI); Input(TEXT("PixelSlope"), PixelSlope);
            Input(TEXT("CameraDelta"), Filter.CameraDelta); Input(TEXT("InverseScale"), Filter.Scale);
            Input(TEXT("StartCm"), Filter.Start); Input(TEXT("EndCm"), Filter.End);
            int32 Consumers = 0;
            // Only the pre-existing consumers are replaced; neither the original
            // vertex DAG nor Blend.NativeSlope can accidentally become cyclic.
            for (UMaterialExpression* E : Owner.Graph)
                for (FExpressionInput* I : E->GetInputsView())
                    if (I && I->Expression == VI)
                    {
                        if (I->OutputIndex) return Fail(TEXT("Slope output index changed"));
                        I->Expression = Blend; ++Consumers;
                    }
            if (Consumers != 1) return Fail(FString::Printf(TEXT("Expected one slope VI consumer, got %d"), Consumers));
            // The original VI must still reach only its two original unfiltered
            // adapters, never a camera-dependent new expression.
            if (SlopeCopies.Num() != 9 || VI->Input.Expression != OriginalSlope)
                return Fail(TEXT("Expected exactly nine cloned slope nodes and unchanged native VI input"));
            for (auto* Adapter : Owner.Adapters)
                if (!NormalInput(Adapter)) return Fail(TEXT("Original near slope normal adapter was modified"));
            return true;
        }

        bool Apply(UMaterial* Master)
        {
            if (!Master || !Scan(Master)) return false;
            int32 MasterNormals = 0, FunctionNormals = 0, SlopeOwners = 0;
            for (const FOwner& O : Owners)
            {
                if (O.Object == Master) MasterNormals += O.Adapters.Num(); else FunctionNormals += O.Adapters.Num();
                int32 VIs = 0;
                for (UMaterialExpression* E : O.Graph) VIs += Cast<UMaterialExpressionVertexInterpolator>(E) ? 1 : 0;
                if (O.Object == Master)
                {
                    MasterWarpVIs = VIs;
                    for (UMaterialExpression* E : O.Graph)
                        if (auto* VI = Cast<UMaterialExpressionVertexInterpolator>(E))
                        {
                            // The inspected warp DAGs have no function calls or
                            // normal adapters. Reject drift into vertex-unsafe
                            // filtered normals rather than moving them to pixel.
                            TArray<UMaterialExpression*> Pending{VI}; TSet<UMaterialExpression*> Seen;
                            for (int32 I = 0; I < Pending.Num(); ++I)
                            {
                                auto* N = Pending[I]; if (!N || Seen.Contains(N)) continue;
                                if (Seen.Num() >= 512 || !N->IsIn(Master) || NormalInput(N)
                                    || Cast<UMaterialExpressionMaterialFunctionCall>(N))
                                    return Fail(TEXT("Warp VI dependency contract changed"));
                                Seen.Add(N);
                                for (auto* P : N->GetInputsView()) if (P && P->Expression) Pending.Add(P->Expression);
                            }
                            WarpInputs.Emplace(VI, VI->Input);
                        }
                }
                else if (O.Object->GetName() == TEXT("MF_APS_MF_SlopeBlock_f6be5407"))
                {
                    ++SlopeOwners;
                    if (O.Adapters.Num() != 2 || VIs != 1) return Fail(TEXT("Slope two-normal/one-VI contract changed"));
                    for (UMaterialExpression* E : O.Graph)
                        if (auto* VI = Cast<UMaterialExpressionVertexInterpolator>(E))
                        {
                            if (!DependsOnNormal(VI->Input.Expression) || !B.Error.IsEmpty()) return Fail(TEXT("Slope VI no longer normal-dependent"));
                            for (auto* N : O.Adapters)
                                if (!DependsCache.Contains(N)) return Fail(TEXT("Slope adapter is outside VI dependency path"));
                        }
                }
                else if (O.Object->GetName() != TEXT("MF_APS_WorldAlignedTexture_a83aa78c") || O.Adapters.Num() != 1 || VIs)
                    return Fail(TEXT("Unexpected normal-dependent function: ") + O.Object->GetPathName());
            }
            if (MasterNormals != 3 || FunctionNormals != 3 || Owners.Num() != 3 || MasterWarpVIs != 5 || SlopeOwners != 1)
                return Fail(FString::Printf(TEXT("Expected six adapters/three owners/five warp VIs/one slope, got %d/%d/%d/%d/%d"),
                    MasterNormals, FunctionNormals, Owners.Num(), MasterWarpVIs, SlopeOwners));
            for (const FOwner& O : Owners)
            {
                const FFilter Filter = MakeFilter(O);
                if (!Filter.Normal) return false;
                if (O.Object->GetName() == TEXT("MF_APS_MF_SlopeBlock_f6be5407"))
                { if (!PatchSlope(O, Filter)) return false; }
                else for (auto* E : O.Adapters) { NormalInput(E)->Input.Expression = Filter.Normal; ++RewiredNormals; }
                Changed.Add(O.Object);
            }
            if (RewiredNormals != 6 || Changed.Num() != 3 || SlopePixelCopies != 9)
                return Fail(TEXT("Final six-normal/three-owner/nine-node-slope guard failed"));
            for (const auto& Pair : WarpInputs)
            {
                const FExpressionInput& A = Pair.Key->Input; const FExpressionInput& C = Pair.Value;
                if (A.Expression != C.Expression || A.OutputIndex != C.OutputIndex || A.Mask != C.Mask
                    || A.MaskR != C.MaskR || A.MaskG != C.MaskG || A.MaskB != C.MaskB || A.MaskA != C.MaskA)
                    return Fail(TEXT("A master warp VI input changed"));
            }
            for (UObject* O : Changed)
                if (auto* F = Cast<UMaterialFunction>(O)) UMaterialEditingLibrary::UpdateMaterialFunction(F);
            UE_LOG(LogTemp, Display, TEXT("[APS.TerrainNormalContinuity] Applied to owned graph only: normalAdapters=6 owners=3 retainedMasterWarpVIs=5 retainedNativeSlopeVI=1 pixelSlopeNodes=%d physicalKm=200..700; near branch preserves native slope interpolation"), SlopePixelCopies);
            return true;
        }
    };
}
#endif
