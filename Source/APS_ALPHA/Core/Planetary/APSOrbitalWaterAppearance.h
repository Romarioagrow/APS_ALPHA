#pragma once

#include "CoreMinimal.h"
#include "APSPlanetSurfaceProfile.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace APSOrbitalWaterAppearance
{
    // Body-class independent: a generated water moon uses the same optical
    // contract as a planet. Borrowed/authored/scaled/custom surfaces stay intact.
    inline bool IsEligible(bool bOwnedRoot, bool bManual, EAPSPlanetLiquidType Liquid,
        double PresentationScale, const FVector& RootScale)
    {
        return bOwnedRoot && !bManual && Liquid == EAPSPlanetLiquidType::Water
            && FMath::IsFinite(PresentationScale) && FMath::IsNearlyEqual(PresentationScale, 1.0)
            && !RootScale.ContainsNaN() && RootScale.Equals(FVector::OneVector, KINDA_SMALL_NUMBER);
    }

    // The caller owns Water-family/physical-body eligibility. Restore from the
    // selected parent, never from the previous MID override or a camera height.
    // Validate both authored values before writing either one.
    inline bool RestoreAuthoredResponse(UMaterialInstanceDynamic* Material)
    {
        if (!IsValid(Material) || !IsValid(Material->Parent.Get())) return false;
        const FHashedMaterialParameterInfo SpecularInfo(TEXT("Specular"));
        const FHashedMaterialParameterInfo RoughnessInfo(TEXT("Roughness"));
        float SavedSpecular = 0, SavedRoughness = 0, ActualSpecular = 0, ActualRoughness = 0;
        if (!Material->Parent->GetScalarParameterValue(SpecularInfo, SavedSpecular)
            || !Material->Parent->GetScalarParameterValue(RoughnessInfo, SavedRoughness)
            || !Material->GetScalarParameterValue(SpecularInfo, ActualSpecular)
            || !Material->GetScalarParameterValue(RoughnessInfo, ActualRoughness)
            || !FMath::IsFinite(SavedSpecular) || !FMath::IsFinite(SavedRoughness)
            || SavedSpecular < 0 || SavedSpecular > 1 || SavedRoughness < 0 || SavedRoughness > 1)
            return false;
        // Exact restoration, including a non-finite old override. No steady-state
        // parameter updates once the MID already matches its authored parent.
        if (!FMath::IsFinite(ActualSpecular) || ActualSpecular != SavedSpecular)
            Material->SetScalarParameterValue(TEXT("Specular"), SavedSpecular);
        if (!FMath::IsFinite(ActualRoughness) || ActualRoughness != SavedRoughness)
            Material->SetScalarParameterValue(TEXT("Roughness"), SavedRoughness);
        return true;
    }
}
