#pragma once

#if WITH_EDITOR
#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "UObject/UObjectHash.h"

// One in-memory pointer edit to the canonical Continuous master's LIGHTING path.
// No graph creation, function edits, shader preparation, load, compile or save.
// Caller must guard exact input SHA/backup/package state and verify compilation
// and cold reload. A matching parent/path alone is not that provenance proof.
namespace APSOrbitalReliefLightingUpdate
{
    namespace Private
    {
        inline bool Whole(const FExpressionInput& I)
        { return I.Expression && I.OutputIndex == 0 && !I.Mask && !I.MaskR && !I.MaskG && !I.MaskB && !I.MaskA; }

        inline const TCHAR* FilterCode()
        {
            return TEXT("float d=length(CameraDelta.xyz)*max(InverseScale.x,0.0);\n")
                TEXT("float t=saturate((d-StartCm)/max(EndCm-StartCm,1.0));\n")
                TEXT("if(t<=0.0) return NativeNormal.xyz;\n")
                TEXT("float r2=dot(Relative.xyz,Relative.xyz);\n")
                TEXT("if(r2<1.e-8) return NativeNormal.xyz;\n")
                TEXT("float3 radial=Relative.xyz*rsqrt(r2);\n")
                TEXT("if(t>=1.0) return radial;\n")
                TEXT("float w=t*t*(3.0-2.0*t);\n")
                TEXT("float3 n=lerp(NativeNormal.xyz,radial,w);\n")
                TEXT("return n*rsqrt(max(dot(n,n),1.e-8));");
        }

        inline FExpressionInput* VectorInput(UMaterialExpressionCustom* C)
        {
            if (!C || C->OutputType != CMOT_Float3
                || C->Description != TEXT("APS rendered vector to planet frame")
                || C->Code != TEXT("return float3(dot(V.xyz,X.xyz),dot(V.xyz,Y.xyz),dot(V.xyz,Z.xyz));")) return nullptr;
            FExpressionInput* Result = nullptr;
            for (FCustomInput& I : C->Inputs)
                if (I.InputName == TEXT("V"))
                { if (Result) return nullptr; Result = &I.Input; }
            return Result && Whole(*Result) ? Result : nullptr;
        }

        inline bool ExclusiveLighting(UMaterial* Master, UMaterialExpressionCustom* Adapter,
            UMaterialExpressionCustom* FinalNormal,
            const TMap<UMaterialExpression*, TArray<UMaterialExpression*>>& Consumers,
            FString& Why)
        {
            // Audited topology, not an expression/object-name assumption:
            // base-normal adapter -> ONE Normalize -> arithmetic -> final normal.
            const TArray<UMaterialExpression*>* First = Consumers.Find(Adapter);
            auto* Normalize = First && First->Num() == 1 ? Cast<UMaterialExpressionNormalize>((*First)[0]) : nullptr;
            if (!Normalize || !Whole(Normalize->VectorInput) || Normalize->VectorInput.Expression != Adapter)
            { Why = TEXT("not a unique Normalize consumer"); return false; }
            TArray<UMaterialExpression*> Pending{Adapter};
            TSet<UMaterialExpression*> Reached;
            for (int32 I = 0; I < Pending.Num(); ++I)
            {
                UMaterialExpression* E = Pending[I];
                if (Reached.Contains(E)) continue;
                if (Reached.Num() >= 4096) { Why = TEXT("consumer closure exceeds bound"); return false; }
                Reached.Add(E);
                if (E != Adapter && E != FinalNormal && (Cast<UMaterialExpressionCustom>(E)
                    || Cast<UMaterialExpressionMaterialFunctionCall>(E)
                    || Cast<UMaterialExpressionTextureSample>(E)
                    || Cast<UMaterialExpressionVertexInterpolator>(E)))
                { Why = TEXT("reaches a function, texture sample, vertex stage or unaudited Custom"); return false; }
                if (const auto* Next = Consumers.Find(E)) Pending.Append(*Next);
            }
            if (!Reached.Contains(FinalNormal)) { Why = TEXT("does not reach final lighting normal"); return false; }
            if (const auto* Next = Consumers.Find(FinalNormal); Next && !Next->IsEmpty())
            { Why = TEXT("final normal is reused by an expression"); return false; }
            for (int32 P = 0; P < MP_MAX; ++P)
            {
                if (P == MP_Normal) continue;
                const FExpressionInput* Input = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P));
                if (Input && Input->Expression && Reached.Contains(Input->Expression))
                { Why = FString::Printf(TEXT("reaches non-normal material property %d"), P); return false; }
            }
            return true;
        }
    }

    inline bool Patch(UMaterial* Master, FString& Error, bool bApply = true)
    {
        if (!Error.IsEmpty()) return false;
        const auto Fail = [&](const FString& Why) { Error = Why; return false; };
        constexpr const TCHAR* CanonicalPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.M_APS_ContinuousTerrain");
        if (!IsInGameThread() || !IsRunningCommandlet() || !IsValid(Master) || Master->GetPathName() != CanonicalPath)
            return Fail(TEXT("Orbital lighting edit requires the exact canonical Continuous master in an offline commandlet"));
        if (Master->MaterialDomain != MD_Surface || Master->bTangentSpaceNormal || Master->bUseMaterialAttributes
            || !Master->GetShadingModels().HasShadingModel(MSM_DefaultLit))
            return Fail(TEXT("Expected DefaultLit world-space normal property routing"));
        FExpressionInput* Normal = Master->GetExpressionInputForProperty(MP_Normal);
        if (!Normal || !Private::Whole(*Normal) || static_cast<FVectorMaterialInput*>(Normal)->UseConstant)
            return Fail(TEXT("Final normal is missing, swizzled or constant-overridden"));
        auto* Final = Cast<UMaterialExpressionCustom>(Normal->Expression);
        if (!Final || Final->GetOuter() != Master || Final->OutputType != CMOT_Float3
            || Final->Description != TEXT("APS planet vector to rendered world")
            || Final->Code != TEXT("return V.x*X.xyz + V.y*Y.xyz + V.z*Z.xyz;"))
            return Fail(TEXT("Final normal no longer matches the audited world-frame rotation; UnifiedLava is out of scope"));

        TArray<UObject*> Children; GetObjectsWithOuter(Master, Children, false);
        TArray<UMaterialExpression*> Graph;
        TMap<UMaterialExpression*, TArray<UMaterialExpression*>> Consumers;
        UMaterialExpressionCustom* Filter = nullptr;
        for (UObject* Child : Children)
            if (auto* E = Cast<UMaterialExpression>(Child); IsValid(E))
            {
                if (Graph.Num() >= 4096) return Fail(TEXT("Master-owned expression inventory exceeds the audited bound"));
                Graph.Add(E);
                if (auto* C = Cast<UMaterialExpressionCustom>(E);
                    C && C->Description == TEXT("APS physical-distance geometric normal continuity v1"))
                { if (Filter) return Fail(TEXT("More than one master-owned physical-normal filter")); Filter = C; }
                for (FExpressionInput* I : E->GetInputsView())
                    if (I && I->Expression)
                    {
                        if (!IsValid(I->Expression) || I->Expression->GetOuter() != Master)
                            return Fail(TEXT("Master input references an unexpected expression owner"));
                        Consumers.FindOrAdd(I->Expression).AddUnique(E);
                    }
            }
        if (!Graph.Contains(Final) || !Filter || Filter->OutputType != CMOT_Float3
            || Filter->Code != Private::FilterCode() || Filter->Inputs.Num() != 6)
            return Fail(TEXT("Missing or changed exact physical-normal filter/final normal"));
        const TCHAR* Names[] = {TEXT("NativeNormal"), TEXT("Relative"), TEXT("CameraDelta"),
            TEXT("InverseScale"), TEXT("StartCm"), TEXT("EndCm")};
        for (int32 I = 0; I < 6; ++I)
            if (Filter->Inputs[I].InputName != Names[I] || !Filter->Inputs[I].Input.Expression)
                return Fail(TEXT("Physical-normal filter input contract changed"));
        const FExpressionInput& Native = Filter->Inputs[0].Input;
        auto* VertexNormal = Cast<UMaterialExpressionVertexNormalWS>(Native.Expression);
        if (!Private::Whole(Native) || !VertexNormal || VertexNormal->GetOuter() != Master)
            return Fail(TEXT("Physical filter's native input is not an unmodified vertex-normal output"));

        FExpressionInput* Selected = nullptr;
        UMaterialExpressionCustom* SelectedAdapter = nullptr;
        int32 Matches = 0, AlreadyNative = 0;
        FString Rejections;
        for (UMaterialExpression* E : Graph)
        {
            auto* Adapter = Cast<UMaterialExpressionCustom>(E);
            FExpressionInput* V = Private::VectorInput(Adapter);
            if (!V || (V->Expression != Filter && V->Expression != VertexNormal)) continue;
            FString Why;
            if (!Private::ExclusiveLighting(Master, Adapter, Final, Consumers, Why))
            { Rejections += GetNameSafe(Adapter) + TEXT(": ") + Why + TEXT("; "); continue; }
            if (V->Expression == VertexNormal) { ++AlreadyNative; continue; }
            ++Matches; Selected = V; SelectedAdapter = Adapter;
        }
        if (AlreadyNative) return Fail(TEXT("An exclusive final-lighting adapter already uses the native normal; refusing repeat patch"));
        if (Matches != 1 || !Selected)
            return Fail(FString::Printf(TEXT("Expected exactly one exclusive lighting adapter, found %d. %s"), Matches, *Rejections));

        if (!bApply)
        {
            UE_LOG(LogTemp, Display, TEXT("[APS.OrbitalReliefLighting] INSPECT master=%s adapter=%s input=V filter=%s native=%s wouldChangeLinks=1 actualWrites=0; all routing guards passed"),
                *Master->GetPathName(), *SelectedAdapter->GetName(), *Filter->GetName(), *VertexNormal->GetName());
            return true;
        }

        // All guards precede the single graph-field write. Below the existing
        // filter's near threshold it already returns this exact NativeNormal.
        // Other adapters, filter inputs, slope/colour/UV/function graphs and all
        // material property links remain byte-for-byte untouched by this helper.
        Selected->Expression = VertexNormal;
        UE_LOG(LogTemp, Display, TEXT("[APS.OrbitalReliefLighting] master=%s adapter=%s input=V filter=%s native=%s changedLinks=1; lighting-only source edit, not rendered acceptance"),
            *Master->GetPathName(), *SelectedAdapter->GetName(), *Filter->GetName(), *VertexNormal->GetName());
        return true;
    }
}
#endif
