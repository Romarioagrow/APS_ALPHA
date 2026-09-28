#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSWaterDepthPalette.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWaterDepthPaletteBounds,
    "APS.Gameplay.World.PlanetSurface.WaterDepth.PaletteBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSWaterDepthPaletteBounds::RunTest(const FString& Parameters)
{
    const FLinearColor Colours[] = {
        FLinearColor(0.005f, 0.018f, 0.060f), FLinearColor(0.04f, 0.25f, 0.32f),
        FLinearColor(0.3f, 0.12f, 0.07f), FLinearColor::Black, FLinearColor::White};
    int32 Cases = 0;
    for (const auto& Deep : Colours)
        for (const auto& Shallow : Colours)
            for (float Budget : {0.0f, 0.15f, 0.35f, 1.0f})
            {
                float Strength = -1.0f;
                TestTrue(TEXT("Finite nonnegative palettes resolve"),
                    APSWaterDepthPalette::ResolveStrength(Deep, Shallow, Budget, Strength));
                TestTrue(TEXT("Strength is bounded"), Strength >= 0.0f && Strength <= 1.0f);
                const double A = APSWaterDepthPalette::Luminance(Deep);
                const double B = APSWaterDepthPalette::Luminance(Shallow);
                if (Deep == Shallow || A == 0.0 || B == 0.0 || Budget == 0.0f)
                    TestTrue(TEXT("No hue-only or black-endpoint fallback"), Strength == 0.0f);
                for (int32 L = 0; L <= 10; ++L)
                    for (int32 D = 0; D <= 10; ++D)
                    {
                        const double Legacy = L / 10.0, ShallowWeight = D / 10.0;
                        const double Alpha = Legacy + (1.0 - Legacy) * Strength * ShallowWeight;
                        const double Baseline = A + (B - A) * Legacy;
                        const double Changed = A + (B - A) * Alpha;
                        TestTrue(TEXT("Palette luminance stays inside relative budget"),
                            FMath::Abs(Changed - Baseline) <= double(Budget) * Baseline + 1.e-14);
                        TestTrue(TEXT("Zero depth weight preserves the baseline"),
                            D != 0 || Changed == Baseline);
                        ++Cases;
                    }
            }
    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Inf = std::numeric_limits<float>::infinity();
    for (float Invalid : {-1.0f, NaN, Inf})
    {
        float Strength = 1.0f;
        TestFalse(TEXT("Invalid endpoint rejected"), APSWaterDepthPalette::ResolveStrength(
            FLinearColor(Invalid, 0.2f, 0.3f), Colours[0], 0.35f, Strength));
        TestTrue(TEXT("Invalid endpoint resets strength"), Strength == 0.0f);
    }
    for (float InvalidBudget : {-0.01f, 1.01f, NaN, Inf})
    {
        float Strength = 1.0f;
        TestFalse(TEXT("Invalid budget rejected"), APSWaterDepthPalette::ResolveStrength(
            Colours[0], Colours[1], InvalidBudget, Strength));
        TestTrue(TEXT("Invalid budget resets strength"), Strength == 0.0f);
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthPalette] cases=%d; uniform arithmetic only, no visual or GPU acceptance"), Cases);
    return true;
}
#endif
