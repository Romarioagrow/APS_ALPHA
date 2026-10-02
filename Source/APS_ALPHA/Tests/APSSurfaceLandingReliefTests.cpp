#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSSurfaceLandingRelief.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSSurfaceLandingReliefTest,
    "APS.Gameplay.World.PlanetSurface.LandingReliefRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSSurfaceLandingReliefTest::RunTest(const FString& Parameters)
{
    using namespace APSSurfaceLandingRelief;
    for (bool Found : {false, true})
        for (bool Oceanic : {false, true})
            for (bool NewStart : {false, true})
                TestEqual(TEXT("Only failed new non-oceanic starts recover; accepted starts/saved replay preserved"),
                    CanRecover(Found, Oceanic, NewStart), !Found && !Oceanic && NewStart);
    const auto Hero = Preferred(false);
    const auto Floor = Gentle();
    TestEqual(TEXT("Existing hero near range unchanged"), Hero.NearCm, 300.0);
    TestEqual(TEXT("Existing hero middle range unchanged"), Hero.MidCm, 1500.0);
    TestEqual(TEXT("Existing hero far range unchanged"), Hero.FarCm, 2500.0);
    TestEqual(TEXT("Recovery reuses oceanic physical floor"), Preferred(true).NearCm, Floor.NearCm);
    TestFalse(TEXT("Flat plane remains rejected"), HasRelief(Floor, 0, 0, 0));
    TestFalse(TEXT("Insufficient near relief rejected"), HasRelief(Floor, 14.9, 2000, 4000));
    TestFalse(TEXT("Insufficient middle relief rejected"), HasRelief(Floor, 250, 149.9, 4000));
    TestFalse(TEXT("Insufficient far relief rejected"), HasRelief(Floor, 250, 2000, 349.9));
    TestTrue(TEXT("Non-flat bounded physical floor accepted"), HasRelief(Floor, 15, 150, 350));
    TestFalse(TEXT("Old hero gate rejects gentle non-flat terrain"), HasRelief(Hero, 250, 2000, 4000));
    TestTrue(TEXT("Recovery can admit same gentle terrain without modifying it"), HasRelief(Floor, 250, 2000, 4000));
    TestFalse(TEXT("Nonfinite range rejected"), HasRelief(Floor, std::numeric_limits<double>::infinity(), 2000, 4000));
    TestFalse(TEXT("NaN range rejected"), HasRelief(Floor, 250, std::numeric_limits<double>::quiet_NaN(), 4000));
    return true;
}
#endif
