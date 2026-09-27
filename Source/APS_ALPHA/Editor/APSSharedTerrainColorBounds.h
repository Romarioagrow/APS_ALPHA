#pragma once
#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVectorParameter.h"

// A palette mix is a convex combination, not extrapolation beyond authored
// colors. Preserve all coverage, textures, coordinates and normal branches.
namespace APSSharedTerrainColorBounds
{
    inline constexpr const TCHAR* Marker = TEXT("APS bounded top-layer color variation");

    inline bool SameInput(const FExpressionInput& A, const FExpressionInput& B)
    {
        return A.Expression == B.Expression && A.OutputIndex == B.OutputIndex
            && A.InputName == B.InputName && A.Mask == B.Mask
            && A.MaskR == B.MaskR && A.MaskG == B.MaskG && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
    }

    template<class TBuild>
    bool Apply(TBuild& Build, UMaterial* Master, bool& Changed)
    {
        Changed = false;
        const auto Refuse = [&](const TCHAR* Why) { Build.Error = Why; return false; };
        if (!Master) return Refuse(TEXT("Missing shared terrain master"));
        UMaterialExpressionLinearInterpolate* Mix = nullptr;
        const auto Before = Build.Expressions(Master);
        TMap<FExpressionInput*, FExpressionInput> Edges;
        for (UMaterialExpression* E : Before)
        {
            if (!E) continue;
            for (FExpressionInput* Input : E->GetInputsView()) if (Input) Edges.Add(Input, *Input);
            auto* Lerp = Cast<UMaterialExpressionLinearInterpolate>(E);
            auto* A = Lerp ? Cast<UMaterialExpressionVectorParameter>(Lerp->A.Expression) : nullptr;
            auto* B = Lerp ? Cast<UMaterialExpressionVectorParameter>(Lerp->B.Expression) : nullptr;
            if (!A || !B || A->ParameterName != TEXT("Color1_3") || B->ParameterName != TEXT("Color2_3")) continue;
            if (Mix) return Refuse(TEXT("More than one top palette mix"));
            Mix = Lerp;
        }
        for (int32 P = 0; P < MP_MAX; ++P)
            if (FExpressionInput* Input = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P)))
                Edges.Add(Input, *Input);
        if (!Mix || Mix->Alpha.OutputIndex || Mix->Alpha.Mask)
            return Refuse(TEXT("Top palette blend/pin mapping changed"));
        auto* Existing = Cast<UMaterialExpressionClamp>(Mix->Alpha.Expression);
        FExpressionInput OriginalAlpha = Existing ? Existing->Input : Mix->Alpha;
        auto* Gain = Cast<UMaterialExpressionMultiply>(OriginalAlpha.Expression);
        if (OriginalAlpha.OutputIndex || OriginalAlpha.Mask || !Gain
            || Gain->GetName() != TEXT("MaterialExpressionMultiply_1")
            || !Gain->A.Expression || Gain->A.Expression->GetName() != TEXT("MaterialExpressionAdd_4")
            || Gain->B.Expression || Gain->ConstB != 1.2f)
            return Refuse(TEXT("Audited top palette variation changed"));
        if (Existing)
        {
            if (Existing->Desc != Marker || Existing->ClampMode != CMODE_Clamp
                || Existing->Min.Expression || Existing->Max.Expression
                || Existing->MinDefault != 0.0f || Existing->MaxDefault != 1.0f)
                return Refuse(TEXT("Unexpected existing top palette bound"));
            return true; // Already protected, do not add a second node.
        }
        auto* Bound = Build.template Add<UMaterialExpressionClamp>(Master);
        if (!Bound) return Refuse(TEXT("Cannot create top palette bound"));
        Bound->Desc = Marker;
        Bound->ClampMode = CMODE_Clamp;
        Bound->Input = OriginalAlpha;
        Bound->MinDefault = 0.0f; Bound->MaxDefault = 1.0f;
        Mix->Alpha.Connect(0, Bound);
        if (Build.Expressions(Master).Num() != Before.Num() + 1
            || !SameInput(Bound->Input, OriginalAlpha))
            return Refuse(TEXT("Unexpected color-bound graph mutation"));
        for (const auto& Edge : Edges)
            if (Edge.Key != &Mix->Alpha && !SameInput(*Edge.Key, Edge.Value))
                return Refuse(TEXT("Unrelated material edge changed"));
        Changed = true;
        return true;
    }
}
#endif
