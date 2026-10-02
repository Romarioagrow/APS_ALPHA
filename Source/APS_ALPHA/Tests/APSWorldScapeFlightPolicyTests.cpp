#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeFlightPolicy.h"
#include <limits>
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeFlightPolicyTest,
    "APS.Gameplay.World.PlanetSurface.FlightResidencyPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSWorldScapeFlightPolicyTest::RunTest(const FString& Parameters)
{
    using namespace APSWorldScapeFlightPolicy;
    constexpr double R = 6750.e5;
    TestFalse(TEXT("Fast low flight preserves ground"), WantsTransit(FVector(R+100.e5,0,0), FVector(1.e10,0,0),R,false));
    TestTrue(TEXT("Outbound enters transit"), WantsTransit(FVector(R*1.5,0,0),FVector(1.e6,0,0),R,false));
    TestFalse(TEXT("Slow high approach keeps detailed root"), WantsTransit(FVector(R*1.5,0,0),FVector(-1.e6,0,0),R,false));
    TestTrue(TEXT("Transit survives braking above inner boundary"), WantsTransit(FVector(R*1.3,0,0),FVector::ZeroVector,R,true));
    TestFalse(TEXT("Return requests near before crossing"), WantsTransit(FVector(R*1.5,0,0),FVector(-R*.05,0,0),R,true));
    TestFalse(TEXT("Tangential fast low flight keeps near"), WantsTransit(FVector(R+50.e5,0,0),FVector(0,1.e10,0),R,true));
    TestTrue(TEXT("Stationary distant orbit can evict"), WantsTransit(FVector(R*2.1,0,0),FVector::ZeroVector,R,false));
    TestFalse(TEXT("Inside body fails safe"), WantsTransit(FVector(R*.9,0,0),FVector::ZeroVector,R,true));
    TestFalse(TEXT("Invalid radius fails safe"), WantsTransit(FVector(R*2,0,0),FVector::ZeroVector,std::numeric_limits<double>::quiet_NaN(),true));
    const FVector Centre(9.e11,-2.e11,3.e11), Shift(1.e9,2.e9,3.e9);
    const FVector P0=Centre+FVector(R*1.5,0,0), P1=P0+FVector(1.e6,0,0);
    TestTrue(TEXT("Common origin rebase preserves relative motion"),
        ((P1-Shift)-(Centre-Shift)-(P0-Centre)).Equals(FVector(1.e6,0,0),.01));
    return true;
}
#endif
