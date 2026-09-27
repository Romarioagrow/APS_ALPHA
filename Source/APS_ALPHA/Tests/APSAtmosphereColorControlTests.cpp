#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/UI/MainMenu/APSAtmosphereColorControl.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmosphereColorControlTest,
    "APS.UI.Generation.AtmosphereColor.LinearCoefficientControls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAtmosphereColorControlTest::RunTest(const FString& Parameters)
{
    using namespace APSAtmosphereColorControl;
    for (const FLinearColor Original : {
        FLinearColor(3.8f, 13.5f, 33.0f, 0.0f),
        FLinearColor(4.0f, 9.0f, 128.0f, 0.375f),
        FLinearColor(7.0f, 7.0f, 7.0f, -2.0f),
        FLinearColor(-0.0f, 0.0f, 0.0f, 0.25f)})
    {
        FLinearColor Coefficients = Original;
        FState State;
        State.Read(Coefficients);
        State.Swatch();
        State.Read(Coefficients);
        TestTrue(TEXT("Read and swatch preserve coefficients bitwise"), SameBits(Coefficients, Original));
        TestFalse(TEXT("Same hue is a bitwise no-op"), State.SetHue(Coefficients, State.Hue));
        TestFalse(TEXT("Same saturation is a bitwise no-op"), State.SetSaturation(Coefficients, State.Saturation));
        TestFalse(TEXT("Same strength is a bitwise no-op even above 64"), State.SetStrength(Coefficients, State.Strength));
        TestTrue(TEXT("No edit remains bitwise identical"), SameBits(Coefficients, Original));

        const float Peak = FMath::Max3(Original.R, Original.G, Original.B);
        State.SetHue(Coefficients, 120.0f);
        State.SetSaturation(Coefficients, 0.75f);
        TestEqual(TEXT("Hue/saturation preserve peak coefficient including >64"), FMath::Max3(Coefficients.R, Coefficients.G, Coefficients.B), Peak);
        TestEqual(TEXT("Hue/saturation preserve alpha"), Coefficients.A, Original.A);
    }

    FLinearColor Zero(-0.0f, 0, 0, 0.375f);
    const FLinearColor OriginalZero = Zero;
    FState ZeroState;
    ZeroState.Read(Zero);
    TestFalse(TEXT("Hue at zero does not brighten or write"), ZeroState.SetHue(Zero, 240.0f));
    TestFalse(TEXT("Saturation at zero does not brighten or write"), ZeroState.SetSaturation(Zero, 1.0f));
    ZeroState.Read(Zero);
    TestEqual(TEXT("Pending achromatic hue is visible in UI"), ZeroState.Hue, 240.0f);
    TestTrue(TEXT("Zero coefficients remain bitwise untouched"), SameBits(Zero, OriginalZero));
    TestEqual(TEXT("Zero swatch stays black"), ZeroState.Swatch(), FLinearColor(0, 0, 0, 1));
    TestTrue(TEXT("Explicit strength activates the selected color"), ZeroState.SetStrength(Zero, 10.0f));
    TestEqual(TEXT("Pending hue/saturation produce blue coefficient"), Zero, FLinearColor(0, 0, 10, 0.375f));

    FLinearColor Gray(5, 5, 5, 0.5f);
    FState GrayState;
    GrayState.SetHue(Gray, 120);
    TestEqual(TEXT("Hue alone does not color achromatic coefficients"), Gray, FLinearColor(5, 5, 5, 0.5f));
    GrayState.SetSaturation(Gray, 1);
    TestEqual(TEXT("Hue is remembered until saturation changes"), Gray, FLinearColor(0, 5, 0, 0.5f));

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    for (const FLinearColor Original : {FLinearColor(-1, 2, 3, 0), FLinearColor(NaN, 2, 3, 0)})
    {
        FLinearColor Invalid = Original;
        FState State;
        State.Read(Invalid);
        TestFalse(TEXT("Invalid coefficients disable mapped controls"), State.bValid);
        TestTrue(TEXT("Invalid read yields finite UI values"), FMath::IsFinite(State.Hue) && FMath::IsFinite(State.Saturation) && FMath::IsFinite(State.Strength));
        TestFalse(TEXT("Invalid hue edit rejected"), State.SetHue(Invalid, 180));
        TestFalse(TEXT("Invalid saturation edit rejected"), State.SetSaturation(Invalid, 0.5f));
        TestFalse(TEXT("Invalid strength edit rejected"), State.SetStrength(Invalid, 20));
        TestTrue(TEXT("Invalid legacy data is never silently repaired"), SameBits(Invalid, Original));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmosphereColorSaveContractTest,
    "APS.UI.Generation.AtmosphereColor.ExistingSaveContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAtmosphereColorSaveContractTest::RunTest(const FString& Parameters)
{
    using namespace APSAtmosphereColorControl;
    UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
    Model->GenerationSeed = 883716;
    Model->PlanetSurfaceSeed = 31791;
    Model->AtmosphereRayleighScattering = 8.0; // Density scale height, not HSV strength.
    Model->AtmosphereOpacity = 12.0;
    Model->AtmosphereColor = FLinearColor(4, 9, 128, 0.375f);
    FAPSPreviewBodyEditOverride Body;
    Body.AtmosphereColor = FLinearColor(5, 7, 90, 0.625f);
    Model->SetPreviewBodyEditOverride(TEXT("SYS0/S0/P0"), Body);

    const FLinearColor Original = Model->AtmosphereColor;
    FState State;
    State.Read(Model->AtmosphereColor);
    State.Swatch();
    TestTrue(TEXT("UI read does not alter saved RGB"), SameBits(Model->SaveWorldData().AtmosphereColor, Original));
    State.SetHue(Model->AtmosphereColor, 120);
    State.SetSaturation(Model->AtmosphereColor, 0.8f);
    TestEqual(TEXT("UI edit does not change generation seed"), Model->GenerationSeed, 883716);
    TestEqual(TEXT("UI edit does not change terrain seed"), Model->PlanetSurfaceSeed, 31791);
    TestEqual(TEXT("UI edit does not reinterpret density height"), Model->AtmosphereRayleighScattering, 8.0);
    TestEqual(TEXT("UI edit does not change opacity"), Model->AtmosphereOpacity, 12.0);
    TestEqual(TEXT("Legacy magnitude survives hue edit"), FMath::Max3(Model->AtmosphereColor.R, Model->AtmosphereColor.G, Model->AtmosphereColor.B), 128.0f);

    UGameSave* Save = NewObject<UGameSave>();
    Save->GeneratedWorldsDataArray.Add(Model->SaveWorldData());
    TestTrue(TEXT("Existing snapshot captures without new fields"), APSWorldSaveSnapshot::Capture(Model, Save->GeneratedWorldModelData));
    UGeneratedWorld* Restored = APSWorldSaveSnapshot::Restore(Save, GetTransientPackage(), TEXT("APS_ATMOSPHERE_COLOR_UI_TEST"));
    if (!TestNotNull(TEXT("Existing snapshot restores"), Restored)) return false;
    TestTrue(TEXT("Edited scattering coefficients restore bitwise"), SameBits(Restored->AtmosphereColor, Model->AtmosphereColor));
    TestEqual(TEXT("Generation seed restores unchanged"), Restored->GenerationSeed, Model->GenerationSeed);
    const FAPSPreviewBodyEditOverride* RestoredBody = Restored->FindPreviewBodyEditOverride(TEXT("SYS0/S0/P0"));
    if (TestNotNull(TEXT("Per-body legacy atmosphere override restores"), RestoredBody))
        TestTrue(TEXT("Unedited body coefficients remain bitwise identical"), SameBits(RestoredBody->AtmosphereColor, Body.AtmosphereColor));
    return true;
}
#endif
