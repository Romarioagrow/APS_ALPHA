#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Generation/APSAtmosphereGeneration.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmosphereAirGlowTest,
    "APS.Gameplay.Generation.AtmosphereAirGlow.SubordinateEmission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAtmosphereAirGlowTest::RunTest(const FString& Parameters)
{
    for (const float Pressure : {0.0f, 0.5f, 1.0f})
    for (const float Humidity : {0.0f, 0.5f, 1.0f})
    for (const float Dust : {0.0f, 0.5f, 1.0f})
    for (const float Seed : {0.0f, 0.5f, 1.0f})
    {
        const float Glow = APSAtmosphereGeneration::GeneratedAirGlow(Pressure, Humidity, Dust, Seed);
        TestTrue(TEXT("Generated night emission stays below the previous .018 floor"),
            FMath::IsFinite(Glow) && Glow >= 0.0018f && Glow <= 0.007f);
    }
    return true;
}
#endif
