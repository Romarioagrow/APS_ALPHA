#pragma once

#if WITH_EDITOR
#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "UObject/UObjectHash.h"

// Offline, one-input diagnostic on the SAME original Continuous master.
// Caller owns exact disk SHA/clean-package guards, backup, compile/cold verification
// and save/restore. This helper does not load, dirty, compile or save any package.
namespace APSOriginalDistanceColorUpdate
{
    inline constexpr const TCHAR* ExpectedSourceSHA1 = TEXT("9541C8FB506D3F95E277D97C2FB1E7D626A420B0");

    namespace Private
    {
        inline bool Whole(const FExpressionInput& I)
        { return I.Expression && I.OutputIndex == 0 && !I.Mask && !I.MaskR && !I.MaskG && !I.MaskB && !I.MaskA; }

        struct FUse { UMaterialExpression* Owner; FExpressionInput* Input; };
    }

    inline bool Patch(UMaterial* Master, FString& Error, bool bApply = true)
    {
        if (!Error.IsEmpty()) return false;
        const auto Fail = [&](const FString& Why) { Error = Why; return false; };
        constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.M_APS_ContinuousTerrain");
        if (!IsInGameThread() || !IsRunningCommandlet() || !IsValid(Master) || Master->GetPathName() != MasterPath)
            return Fail(TEXT("Distance-colour edit requires the exact Continuous master in an offline GT commandlet"));
        if (Master->MaterialDomain != MD_Surface || Master->bUseMaterialAttributes || Master->bTangentSpaceNormal
            || Master->BlendMode != BLEND_Masked || !Master->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit))
            return Fail(TEXT("Expected original DefaultLit masked, world-normal property routing"));

        FExpressionInput* Base = Master->GetExpressionInputForProperty(MP_BaseColor);
        if (!Base || !Private::Whole(*Base) || static_cast<FColorMaterialInput*>(Base)->UseConstant)
            return Fail(TEXT("BaseColor is missing, swizzled or constant-overridden"));
        auto* Final = Cast<UMaterialExpressionLinearInterpolate>(Base->Expression);
        if (!Final || !Private::Whole(Final->A) || !Cast<UMaterialExpressionMultiply>(Final->A.Expression)
            || !Private::Whole(Final->B) || !Cast<UMaterialExpressionStaticSwitch>(Final->B.Expression)
            || !Private::Whole(Final->Alpha))
            return Fail(TEXT("BaseColor does not have the audited final Multiply/StaticSwitch/Clamp lerp topology"));
        auto* Clamp = Cast<UMaterialExpressionClamp>(Final->Alpha.Expression);
        if (!Clamp || Clamp->ClampMode != CMODE_Clamp || Clamp->Min.Expression || Clamp->Max.Expression
            || Clamp->MinDefault != 0.0f || Clamp->MaxDefault != 1.0f || !Private::Whole(Clamp->Input))
            return Fail(TEXT("Final colour alpha is not an unchanged unconnected-bounds Clamp[0,1]"));
        if (Cast<UMaterialExpressionMultiply>(Clamp->Input.Expression))
            return Fail(TEXT("Final colour Clamp already bypasses its distance lerp; refusing repeat patch"));
        auto* Distance = Cast<UMaterialExpressionLinearInterpolate>(Clamp->Input.Expression);
        if (!Distance || Distance->A.Expression || Distance->ConstA != 2.0f
            || !Private::Whole(Distance->B) || !Private::Whole(Distance->Alpha))
            return Fail(TEXT("Expected distance lerp with disconnected A=2 and whole B/Alpha inputs"));
        // Disconnected A has a serialized OutputIndex=1 in 9541. It is unused;
        // do not normalize or rewrite that pin while guarding its constant value.
        auto* OriginalMask = Cast<UMaterialExpressionMultiply>(Distance->B.Expression);
        auto* Sphere = Cast<UMaterialExpressionSphereMask>(Distance->Alpha.Expression);
        if (!OriginalMask || !Private::Whole(OriginalMask->A)
            || !Cast<UMaterialExpressionMultiply>(OriginalMask->A.Expression)
            || OriginalMask->B.Expression || OriginalMask->ConstB != 2.0f)
            return Fail(TEXT("Distance lerp B is not the original connected mask Multiply with constant B=2"));
        if (!Sphere || !Private::Whole(Sphere->A) || !Private::Whole(Sphere->B)
            || !Cast<UMaterialExpressionAppendVector>(Sphere->A.Expression)
            || !Cast<UMaterialExpressionAppendVector>(Sphere->B.Expression)
            || !Private::Whole(Sphere->Radius) || !Private::Whole(Sphere->Hardness))
            return Fail(TEXT("Distance alpha no longer has the audited sphere-mask coordinate/constant inputs"));
        auto* Radius = Cast<UMaterialExpressionConstant>(Sphere->Radius.Expression);
        auto* Hardness = Cast<UMaterialExpressionConstant>(Sphere->Hardness.Expression);
        if (!Radius || Radius->R != 70962136.0f || !Hardness || Hardness->R != 0.0f)
            return Fail(TEXT("Distance sphere radius/hardness differ from 70962136 cm / 0"));

        // Root selection is anchored at MP_BaseColor, never names or repeated GUIDs.
        // Count INPUT SLOTS, not just consumer nodes: reuse in another pin fails.
        TArray<UObject*> Children; GetObjectsWithOuter(Master, Children, false);
        TSet<UMaterialExpression*> Graph;
        TMap<UMaterialExpression*, TArray<Private::FUse>> Uses;
        for (UObject* Child : Children)
            if (auto* E = Cast<UMaterialExpression>(Child); IsValid(E))
            {
                if (Graph.Num() >= 4096) return Fail(TEXT("Master expression inventory exceeds the audited bound"));
                Graph.Add(E);
                for (FExpressionInput* I : E->GetInputsView())
                    if (I && I->Expression)
                    {
                        if (!IsValid(I->Expression) || I->Expression->GetOuter() != Master)
                            return Fail(TEXT("Master input references an invalid or non-master-owned expression"));
                        Uses.FindOrAdd(I->Expression).Add({E, I});
                    }
            }
        UMaterialExpression* const Selected[] = {Final, Clamp, Distance, OriginalMask, Sphere, Radius, Hardness};
        for (UMaterialExpression* E : Selected)
            if (!Graph.Contains(E)) return Fail(TEXT("Selected colour-chain expression is absent from the owned graph"));
        const auto OnlyUse = [&](UMaterialExpression* E, UMaterialExpression* Owner, FExpressionInput* Input)
        {
            const auto* Found = Uses.Find(E);
            return Found && Found->Num() == 1 && (*Found)[0].Owner == Owner && (*Found)[0].Input == Input;
        };
        if (!OnlyUse(Sphere, Distance, &Distance->Alpha) || !OnlyUse(Distance, Clamp, &Clamp->Input)
            || !OnlyUse(Clamp, Final, &Final->Alpha) || Uses.Contains(Final))
            return Fail(TEXT("Expected exclusive Sphere->Lerp->Clamp->BaseColor lerp chain; other input consumers found"));
        for (int32 P = 0; P < MP_MAX; ++P)
        {
            if (P == MP_BaseColor) continue;
            const FExpressionInput* I = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P));
            if (I && (I->Expression == Sphere || I->Expression == Distance || I->Expression == Clamp || I->Expression == Final))
                return Fail(FString::Printf(TEXT("Selected colour chain also reaches material property %d"), P));
        }

        if (!bApply)
        {
            UE_LOG(LogTemp, Display, TEXT("[APS.OriginalDistanceColor] INSPECT master=%s clamp=%s distanceLerp=%s originalMask=%s wouldChangeInputs=1 actualWrites=0 exclusiveBaseColor=1; source SHA must be guarded by caller"),
                *Master->GetPathName(), *Clamp->GetName(), *Distance->GetName(), *OriginalMask->GetName());
            return true;
        }

        // The sole write: preserve the original B pin's entire input record.
        // Near is NOT bit-identical whenever the old sphere alpha is below one
        // and the Clamp is unsaturated. Upstream masks/radialization remain;
        // this does not make all colour camera-independent or prove a visual fix.
        Clamp->Input = Distance->B;
        UE_LOG(LogTemp, Display, TEXT("[APS.OriginalDistanceColor] master=%s clamp=%s bypassedDistanceLerp=%s originalMask=%s changedInputs=1 exclusiveBaseColor=1; near may change, other masks/normal/UV/functions untouched; NOT rendered acceptance"),
            *Master->GetPathName(), *Clamp->GetName(), *Distance->GetName(), *OriginalMask->GetName());
        return true;
    }
}
#endif
