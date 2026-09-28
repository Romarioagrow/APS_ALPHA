#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSLivingBiomeTransfer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSLivingBiomeTransferTest,
    "APS.Materials.SharedTerrain.LivingBiomeTransfer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSLivingBiomeTransferTest::RunTest(const FString& Parameters)
{
    using namespace APSLivingBiomeTransfer;
    TestEqual(TEXT("Sea datum is unchanged"), Height(0.0, 0.08), 0.08);
    TestTrue(TEXT("Full authored land span reaches highland layers"),
        FMath::IsNearlyEqual(Height(0.08, 0.08), 0.86, 1.e-9));
    double Previous = 0.08;
    for (int32 I = 0; I <= 100; ++I)
    {
        const double H = Height(I * 0.001, 0.08);
        TestTrue(TEXT("Height transfer remains bounded and monotone"),
            H >= Previous && H >= 0.08 && H <= 0.86 + 1.e-12);
        Previous = H;
    }
    TestTrue(TEXT("Dry and wet regions no longer collapse to the same grass band"),
        Humidity(0.6, 0.1, 0.4, 0.0) < 0.2
        && Humidity(0.6, 0.9, 0.4, 0.0) > 0.7);
    TestTrue(TEXT("Coast is not forced into the inland dry belt"),
        Humidity(0.6, 0.5, 0.4, 1.0) > Humidity(0.6, 0.5, 0.4, 0.0));
    // Algebraic coverage contract, not a shader/render test. Native variation
    // shifts the shoreline slightly; both endpoints must remain distinct.
    for (const float Variation : {0.0f, 0.015f, 0.03f})
    {
        auto InlandMask = [Variation](float R)
        {
            return FMath::Square(FMath::Clamp((1.0f + 2.0f * CoastContrast)
                * (R - CoastShift - Variation) - CoastContrast, 0.0f, 1.0f));
        };
        TestTrue(TEXT("Sand is not already suppressed at sea level"), InlandMask(0.08f) < 0.001f);
        TestTrue(TEXT("The beach does not cover inland elevations"), InlandMask(0.20f) > 0.999f);
    }
    FAPSResolvedPlanetSurfaceProfile Profile;
    Profile.Archetype = EAPSPlanetSurfaceArchetype::Magmatic;
    Profile.LiquidType = EAPSPlanetLiquidType::Lava;
    TestFalse(TEXT("Lava transfer excluded"), APSLivingTerrainPalette::Allows(Profile));
    Profile.Archetype = EAPSPlanetSurfaceArchetype::Cryogenic;
    Profile.LiquidType = EAPSPlanetLiquidType::None;
    TestFalse(TEXT("Dry frozen transfer excluded"), APSLivingTerrainPalette::Allows(Profile));
    return true;
}
#endif
