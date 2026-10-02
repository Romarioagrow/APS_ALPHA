#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Rendering/APSPlanetSurfaceFill.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetSurfaceFillTest,
    "APS.Gameplay.World.PlanetSurface.SurfaceFillContinuity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceFillTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetSurfaceFill;
    for (double Altitude : {-100.0, 0.0, 200.0, 100000.0, FadeStartCm})
        TestEqual(TEXT("Accepted ground intensity unchanged"), Weight(Altitude), 1.0f);
    TestEqual(TEXT("Fade midpoint"), Weight(3000000.0), 0.5f);
    for (double Altitude : {MaximumAltitudeCm, 10000000.0, std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()})
        TestEqual(TEXT("Absent outside finite local surface band"), Weight(Altitude), 0.0f);
    float Previous = 1.0f;
    for (int32 I = 0; I <= 1000; ++I)
    {
        const float W = Weight(FadeStartCm + I * (MaximumAltitudeCm - FadeStartCm) / 1000.0);
        TestTrue(TEXT("Bounded monotonic attenuation"), W >= 0.0f && W <= Previous);
        Previous = W;
    }
    TestTrue(TEXT("No finite intensity jump at visibility cutoff"), Weight(MaximumAltitudeCm - 100.0) < 0.00001f);
    TestTrue(TEXT("No intensity jump leaving accepted low-altitude band"), Weight(FadeStartCm + 100.0) > 0.99999f);
    return true;
}
#endif
