#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSOrbitalWaterAppearance.h"
#include "APS_ALPHA/Core/Planetary/APSSharedWaterMaterial.h"
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSOrbitalWaterAppearanceTest,
    "APS.Gameplay.World.PlanetSurface.OrbitalWaterAppearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSOrbitalWaterAppearanceTest::RunTest(const FString& Parameters)
{
    using namespace APSOrbitalWaterAppearance;
    TestTrue(TEXT("Owned full-scale water body eligible (planet or moon)"),
        IsEligible(true, false, EAPSPlanetLiquidType::Water, 1.0, FVector::OneVector));
    TestFalse(TEXT("Borrowed root preserved"), IsEligible(false, false, EAPSPlanetLiquidType::Water, 1.0, FVector::OneVector));
    TestFalse(TEXT("Manual planet preserved"), IsEligible(true, true, EAPSPlanetLiquidType::Water, 1.0, FVector::OneVector));
    TestFalse(TEXT("Preview preserved"), IsEligible(true, false, EAPSPlanetLiquidType::Water, 0.01, FVector::OneVector));
    TestFalse(TEXT("Scaled root preserved"), IsEligible(true, false, EAPSPlanetLiquidType::Water, 1.0, FVector(2.0)));
    for (const EAPSPlanetLiquidType Liquid : {EAPSPlanetLiquidType::None, EAPSPlanetLiquidType::Lava, EAPSPlanetLiquidType::Ammonia})
        TestFalse(TEXT("Other liquids untouched"), IsEligible(true, false, Liquid, 1.0, FVector::OneVector));
    TestFalse(TEXT("Invalid presentation scale rejected"),
        IsEligible(true, false, EAPSPlanetLiquidType::Water, std::numeric_limits<double>::infinity(), FVector::OneVector));
    TestFalse(TEXT("Missing material is not repaired"), RestoreAuthoredResponse(nullptr));

#if WITH_EDITOR
    TStrongObjectPtr<UMaterialInstance> SavedTemplate(LoadObject<UMaterialInstance>(nullptr, APSSharedWaterMaterial::TemplatePath()));
    if (!TestNotNull(TEXT("Saved Water template exists"), SavedTemplate.Get())) return false;
    // Synthetic parent overrides test guard boundaries without editing the asset.
    // Actual saved-parent optics are tested by SharedGeneratedLiquidSelection.
    TStrongObjectPtr<UMaterialInstanceConstant> Parent(NewObject<UMaterialInstanceConstant>(GetTransientPackage()));
    if (!TestNotNull(TEXT("Transient parent created"), Parent.Get())) return false;
    Parent->SetParentEditorOnly(SavedTemplate.Get());
    TStrongObjectPtr<UMaterialInstanceDynamic> Child(UMaterialInstanceDynamic::Create(Parent.Get(), GetTransientPackage()));
    if (!TestNotNull(TEXT("Transient parent created"), Parent.Get())
        || !TestNotNull(TEXT("Transient child created"), Child.Get())) return false;
    for (const float Saved : {0.0f, 0.18f, 0.25f, 0.26f, 0.65f, 1.0f})
    {
        Parent->SetScalarParameterValueEditorOnly(TEXT("Specular"), Saved);
        Parent->SetScalarParameterValueEditorOnly(TEXT("Roughness"), Saved);
        Child->SetScalarParameterValue(TEXT("Specular"), 1.0f - Saved);
        Child->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
        // Inject invalid CPU fixture data directly; the setter's floating-point
        // equality gate is not a reliable way to prepare NaN with /fp:fast.
        auto* OldRoughness = Child->ScalarParameterValues.FindByPredicate([](const FScalarParameterValue& Value)
            { return Value.ParameterInfo.Name == TEXT("Roughness"); });
        if (!TestNotNull(TEXT("Old roughness override exists"), OldRoughness)) return false;
        OldRoughness->ParameterValue = std::numeric_limits<float>::quiet_NaN();
        float InvalidOldRoughness = 0;
        Child->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Roughness")), InvalidOldRoughness);
        TestFalse(TEXT("Old override readback is actually nonfinite"), FMath::IsFinite(InvalidOldRoughness));
        TestTrue(TEXT("Previous finite/non-finite overrides restored"), RestoreAuthoredResponse(Child.Get()));
        float Specular = -1, Roughness = -1;
        Child->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Specular")), Specular);
        Child->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Roughness")), Roughness);
        TestEqual(TEXT("Specular exactly matches parent"), Specular, Saved);
        TestEqual(TEXT("Roughness exactly matches parent"), Roughness, Saved);
        const int32 OverrideCount = Child->ScalarParameterValues.Num();
        TestTrue(TEXT("Repeat restoration remains valid"), RestoreAuthoredResponse(Child.Get()));
        TestEqual(TEXT("Repeat restoration adds no override"), Child->ScalarParameterValues.Num(), OverrideCount);
    }
    for (const float Invalid : {-0.01f, 1.01f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
    for (const bool bInvalidSpecular : {false, true})
    {
        Parent->SetScalarParameterValueEditorOnly(TEXT("Specular"), 0.3f);
        Parent->SetScalarParameterValueEditorOnly(TEXT("Roughness"), 0.7f);
        const FName InvalidName = bInvalidSpecular ? TEXT("Specular") : TEXT("Roughness");
        auto* InvalidParameter = Parent->ScalarParameterValues.FindByPredicate([InvalidName](const FScalarParameterValue& Value)
            { return Value.ParameterInfo.Name == InvalidName; });
        if (!TestNotNull(TEXT("Authored fixture override exists"), InvalidParameter)) return false;
        InvalidParameter->ParameterValue = Invalid;
        float ParentReadback = 0;
        Parent->GetScalarParameterValue(FHashedMaterialParameterInfo(InvalidName), ParentReadback);
        TestTrue(InvalidName.ToString() + TEXT(" parent readback is actually invalid"),
            !FMath::IsFinite(ParentReadback) || ParentReadback < 0 || ParentReadback > 1);
        Child->SetScalarParameterValue(TEXT("Specular"), 0.4f);
        Child->SetScalarParameterValue(TEXT("Roughness"), 0.6f);
        TestFalse(TEXT("Invalid authored response rejected before either write"), RestoreAuthoredResponse(Child.Get()));
        float Specular = -1, Roughness = -1;
        Child->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Specular")), Specular);
        Child->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Roughness")), Roughness);
        TestEqual(TEXT("Rejected response leaves specular untouched"), Specular, 0.4f);
        TestEqual(TEXT("Rejected response leaves roughness untouched"), Roughness, 0.6f);
    }
#endif
    AddInfo(TEXT("Authored response has no camera/altitude/CVar input; transient parameter contracts only, not rendered acceptance."));
    return true;
}
#endif
