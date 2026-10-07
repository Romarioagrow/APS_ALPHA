#pragma once

#if WITH_EDITOR
#include "CoreMinimal.h"
#include "APSPlanetReliefNormalCode.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPixelNormalWS.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "UObject/UObjectHash.h"

// In-memory transform only. No load, hydration, compile, save or runtime caller.
// Caller owns source SHA/backup/dirty-package gates and cold/compiled validation.
// A live graph that does not match the audited routing is refused, not repaired.
namespace APSPlanetReliefNormalUpdate
{
    inline constexpr const TCHAR* Description = TEXT("APS canonical relief: final rock lighting normal v1");
    enum class ERoute : uint8 { Surface, UnifiedLavaRock };

    struct FInputs
    {
        FString ExpectedMasterPath; // Explicit caller-owned object, never inferred.
        ERoute Route = ERoute::Surface;
        FExpressionInput ExpectedRockNormal; // Exact original final world-normal link.
        FExpressionInput BaselineWorldNormal; // Existing physical-normal filter output.
        FExpressionInput CanonicalWorldNormal; // Already sampled/decoded/world-rotated.
        FExpressionInput Availability;
        FExpressionInput Blend; // RequestedBlend; zero must preserve the near output.
    };

    struct FResult
    {
        UMaterial* ChangedOwner = nullptr;
        UMaterialExpressionCustom* Node = nullptr;
    };

    namespace Private
    {
        inline bool Same(const FExpressionInput& A, const FExpressionInput& B)
        {
            return A.Expression == B.Expression && A.OutputIndex == B.OutputIndex
                && A.InputName == B.InputName && A.Mask == B.Mask && A.MaskR == B.MaskR
                && A.MaskG == B.MaskG && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
        }

        inline bool Whole(const FExpressionInput& I)
        { return I.Expression && I.OutputIndex == 0 && !I.Mask && !I.MaskR && !I.MaskG && !I.MaskB && !I.MaskA; }

        inline const TCHAR* BaselineCode()
        {
            // Exact APSSharedTerrainNormalContinuity::MakeFilter code, not a new
            // normal/distance policy. Any future change requires a fresh audit.
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

        inline bool Tree(UMaterial* Master, UMaterialExpression* Root,
            TSet<UMaterialExpression*>& Seen, bool bNewDependency, FString& Error)
        {
            TArray<UMaterialExpression*> Pending{Root};
            for (int32 I = 0; I < Pending.Num(); ++I)
            {
                UMaterialExpression* E = Pending[I];
                if (!IsValid(E) || E->GetOuter() != Master)
                { Error = TEXT("Relief normal dependency is missing or not directly owned by the master"); return false; }
                if (Seen.Contains(E)) continue;
                if (Seen.Num() >= 4096)
                { Error = TEXT("Relief normal dependency bound exceeded"); return false; }
                Seen.Add(E);
                if (bNewDependency && (Cast<UMaterialExpressionPixelNormalWS>(E)
                    || Cast<UMaterialExpressionVertexInterpolator>(E)
                    || Cast<UMaterialExpressionMaterialFunctionCall>(E)))
                { Error = TEXT("Supplied relief input hides an unaudited function, pixel-normal feedback or vertex-stage evaluation"); return false; }
                for (FExpressionInput* Input : E->GetInputsView())
                    if (Input && Input->Expression)
                    {
                        if (Input->Expression == E)
                        { Error = TEXT("Self-referencing relief normal dependency"); return false; }
                        Pending.Add(Input->Expression);
                    }
            }
            return true;
        }

        inline bool Shore(UMaterial* Master, UMaterialExpressionLinearInterpolate* Normal)
        {
            // APSUnifiedLavaSurfaceBuilder: vertex radial height / tolerance,
            // SmoothStep(1,3), OneMinus shared by all seven final property Lerps.
            if (!Normal || !Whole(Normal->A) || !Whole(Normal->B) || !Whole(Normal->Alpha)
                || !Cast<UMaterialExpressionVertexNormalWS>(Normal->B.Expression)) return false;
            auto* Weight = Cast<UMaterialExpressionOneMinus>(Normal->Alpha.Expression);
            auto* Step = Weight && Whole(Weight->Input) ? Cast<UMaterialExpressionSmoothStep>(Weight->Input.Expression) : nullptr;
            auto* Divide = Step && Whole(Step->Value) ? Cast<UMaterialExpressionDivide>(Step->Value.Expression) : nullptr;
            auto* Height = Divide && Whole(Divide->A) ? Cast<UMaterialExpressionVertexInterpolator>(Divide->A.Expression) : nullptr;
            auto* Tolerance = Divide && Whole(Divide->B) ? Cast<UMaterialExpressionScalarParameter>(Divide->B.Expression) : nullptr;
            auto* Delta = Height && Whole(Height->Input) ? Cast<UMaterialExpressionSubtract>(Height->Input.Expression) : nullptr;
            auto* Radius = Delta && Whole(Delta->A) ? Cast<UMaterialExpressionDistance>(Delta->A.Expression) : nullptr;
            auto* Sea = Delta && Whole(Delta->B) ? Cast<UMaterialExpressionScalarParameter>(Delta->B.Expression) : nullptr;
            if (!Step || Step->Min.Expression || Step->Max.Expression || Step->ConstMin != 1.0f || Step->ConstMax != 3.0f
                || !Tolerance || Tolerance->ParameterName != TEXT("APS_UnifiedRadiusToleranceCm")
                || !Radius || !Radius->A.Expression || !Radius->B.Expression
                || !Sea || Sea->ParameterName != TEXT("APS_UnifiedSeaRadiusCm")) return false;
            for (EMaterialProperty P : {MP_BaseColor, MP_EmissiveColor, MP_Roughness,
                    MP_Metallic, MP_Specular, MP_AmbientOcclusion, MP_Normal})
            {
                const FExpressionInput* Input = Master->GetExpressionInputForProperty(P);
                auto* Lerp = Input && Whole(*Input) ? Cast<UMaterialExpressionLinearInterpolate>(Input->Expression) : nullptr;
                if (!Lerp || Lerp->GetOuter() != Master || !Same(Lerp->Alpha, Normal->Alpha)) return false;
            }
            return true;
        }
    }

    inline bool Patch(UMaterial* Master, const FInputs& Inputs, FResult& Out, FString& Error)
    {
        if (!Error.IsEmpty()) return false;
        const auto Fail = [&](const TCHAR* Why) { Error = Why; return false; };
        if (!IsInGameThread() || !IsRunningCommandlet() || !IsValid(Master)
            || Inputs.ExpectedMasterPath.IsEmpty() || Master->GetPathName() != Inputs.ExpectedMasterPath
            || Out.ChangedOwner || Out.Node)
            return Fail(TEXT("Relief normal patch needs an explicit owned master in an offline commandlet and an empty result"));
        if (Master->MaterialDomain != MD_Surface || Master->bUseMaterialAttributes || Master->bTangentSpaceNormal
            || !Master->GetShadingModels().HasShadingModel(MSM_DefaultLit))
            return Fail(TEXT("Expected existing DefaultLit world-space normal property routing"));
        if (Inputs.Route != ERoute::Surface && Inputs.Route != ERoute::UnifiedLavaRock)
            return Fail(TEXT("Unknown relief normal route"));
        FExpressionInput* Normal = Master->GetExpressionInputForProperty(MP_Normal);
        if (!Normal || !Private::Whole(*Normal) || static_cast<FVectorMaterialInput*>(Normal)->UseConstant)
            return Fail(TEXT("Final normal is missing, swizzled or overridden by a constant"));
        FExpressionInput* Target = Normal;
        if (Inputs.Route == ERoute::UnifiedLavaRock)
        {
            auto* Lerp = Cast<UMaterialExpressionLinearInterpolate>(Normal->Expression);
            if (!Private::Shore(Master, Lerp)) return Fail(TEXT("UnifiedLava final rock/lava/shore routing differs from the audited builder"));
            Target = &Lerp->A;
        }
        if (!Private::Same(*Target, Inputs.ExpectedRockNormal) || !Private::Whole(*Target))
            return Fail(TEXT("Original rock normal link differs from the caller's exact preflight link"));
        auto* Rock = Cast<UMaterialExpressionCustom>(Target->Expression);
        if (!Rock || Rock->Description != TEXT("APS planet vector to rendered world")
            || Rock->Code != TEXT("return V.x*X.xyz + V.y*Y.xyz + V.z*Z.xyz;") || Rock->OutputType != CMOT_Float3)
            return Fail(TEXT("Original rock normal is not the audited final world-frame rotation"));
        auto* Baseline = Cast<UMaterialExpressionCustom>(Inputs.BaselineWorldNormal.Expression);
        if (!Private::Whole(Inputs.BaselineWorldNormal) || !Baseline
            || Baseline->Description != TEXT("APS physical-distance geometric normal continuity v1")
            || Baseline->Code != Private::BaselineCode() || Baseline->OutputType != CMOT_Float3 || Baseline->Inputs.Num() != 6)
            return Fail(TEXT("Caller baseline is not the exact existing physical-normal filter"));
        const TCHAR* BaselineNames[] = {TEXT("NativeNormal"), TEXT("Relative"), TEXT("CameraDelta"),
            TEXT("InverseScale"), TEXT("StartCm"), TEXT("EndCm")};
        for (int32 I = 0; I < 6; ++I)
            if (Baseline->Inputs[I].InputName != BaselineNames[I] || !Baseline->Inputs[I].Input.Expression)
                return Fail(TEXT("Existing baseline input contract changed"));
        if (!Cast<UMaterialExpressionVertexNormalWS>(Baseline->Inputs[0].Input.Expression))
            return Fail(TEXT("Existing baseline no longer starts with a native vertex normal"));

        TSet<UMaterialExpression*> RockTree;
        if (!Private::Tree(Master, Rock, RockTree, false, Error)) return false;
        if (!RockTree.Contains(Baseline)) return Fail(TEXT("Supplied baseline is not an actual dependency of the original rock normal"));
        for (const FExpressionInput* I : {&Inputs.CanonicalWorldNormal, &Inputs.Availability, &Inputs.Blend})
        {
            TSet<UMaterialExpression*> Dependencies;
            if (!Private::Tree(Master, I->Expression, Dependencies, true, Error)) return false;
        }

        // Snapshot every pre-existing root property link and owned expression
        // input, including disconnected/unregistered owned nodes. No graph edits
        // occur until all routing/uniqueness/ownership checks have succeeded.
        struct FSaved { FExpressionInput* Pointer; FExpressionInput Value; };
        TArray<FSaved> Saved;
        for (int32 P = 0; P < MP_MAX; ++P)
            if (FExpressionInput* I = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P))) Saved.Add({I, *I});
        TArray<UObject*> Children; GetObjectsWithOuter(Master, Children, false);
        int32 Baselines = 0;
        for (UObject* Child : Children)
            if (auto* E = Cast<UMaterialExpression>(Child); IsValid(E))
            {
                if (auto* C = Cast<UMaterialExpressionCustom>(E))
                {
                    if (C->Description == Description || C->Code == UTF8_TO_TCHAR(APSPlanetReliefNormalCode::RockNormalHLSL()))
                        return Fail(TEXT("Relief normal transform is already present; refusing a second patch"));
                    if (C->Description == TEXT("APS physical-distance geometric normal continuity v1")) ++Baselines;
                }
                for (FExpressionInput* I : E->GetInputsView()) if (I) Saved.Add({I, *I});
            }
        if (Baselines != 1) return Fail(TEXT("Expected exactly one master-owned physical-normal baseline"));

        auto* Node = NewObject<UMaterialExpressionCustom>(Master);
        Node->Material = Master; Node->MaterialExpressionGuid = FGuid::NewGuid();
        Node->Description = Description; Node->OutputType = CMOT_Float3;
        Node->Code = UTF8_TO_TCHAR(APSPlanetReliefNormalCode::RockNormalHLSL());
        Node->Inputs.Reset(); Node->Outputs.Reset(); Node->Outputs.Add(FExpressionOutput());
        const auto Add = [&](const TCHAR* Name, const FExpressionInput& Input)
        { FCustomInput I; I.InputName = Name; I.Input = Input; Node->Inputs.Add(I); };
        Add(TEXT("AuthoredNormal"), *Target);
        Add(TEXT("BaselineNormal"), Inputs.BaselineWorldNormal);
        Add(TEXT("CanonicalNormal"), Inputs.CanonicalWorldNormal);
        Add(TEXT("Availability"), Inputs.Availability);
        Add(TEXT("RequestedBlend"), Inputs.Blend);
        const FExpressionInput Original = *Target;
        Master->GetExpressionCollection().AddExpression(Node);
        Target->Expression = Node; // The ONLY pre-existing graph-field write.
        for (const FSaved& S : Saved)
        {
            FExpressionInput Expected = S.Value;
            if (S.Pointer == Target) Expected.Expression = Node;
            if (!Private::Same(*S.Pointer, Expected))
            {
                *Target = Original;
                Master->GetExpressionCollection().RemoveExpression(Node);
                Node->MarkAsGarbage();
                return Fail(TEXT("Unexpected existing graph-link mutation; relief target restored, publication forbidden"));
            }
        }
        Out.ChangedOwner = Master; Out.Node = Node;
        return true;
    }
}
#endif
