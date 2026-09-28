#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeReadinessPolicy.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeReadinessPolicyTest,
    "APS.Gameplay.World.PlanetSurface.InitialPayloadReadiness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeReadinessPolicyTest::RunTest(const FString& Parameters)
{
    using namespace APSWorldScapeReadinessPolicy;
    TestTrue(TEXT("First worker-free frame is validated"), NeedsInitialPayloadValidation(false, false));
    TestFalse(TEXT("Do not read worker-owned payload"), NeedsInitialPayloadValidation(false, true));
    TestFalse(TEXT("Published idle frame does not rescan vertices"), NeedsInitialPayloadValidation(true, false));
    TestFalse(TEXT("Published worker frame preserves latch"), NeedsInitialPayloadValidation(true, true));
    TestFalse(TEXT("No samples cannot publish"), FHeightAgreement().IsReady());
    FHeightAgreement Incomplete;
    Incomplete.Add(0, 0); Incomplete.Add(0, 0);
    TestFalse(TEXT("Incomplete sample set cannot publish"), Incomplete.IsReady());
    for (const double Datum : {0.0, -50000.0, 120000.0})
    {
        FHeightAgreement Flat, LowRelief, Displaced, MissingRelief, WrongDatum;
        for (int32 Index = 0; Index < 8; ++Index)
        {
            Flat.Add(Datum, Datum);
            LowRelief.Add(Datum + Index * 5, Datum + Index * 5);
            Displaced.Add(Datum + Index * 100, Datum + Index * 100);
            MissingRelief.Add(Datum, Datum + Index * 30); // <= 2.5 m error alone is insufficient
            WrongDatum.Add(Datum + 251, Datum);
        }
        TestTrue(TEXT("Analytically flat patch is legitimate"), Flat.IsReady());
        TestTrue(TEXT("Sub-metre relief is legitimate"), LowRelief.IsReady());
        TestTrue(TEXT("Matching relief is ready"), Displaced.IsReady());
        TestFalse(TEXT("Smooth sphere cannot stand in for relief"), MissingRelief.IsReady());
        TestFalse(TEXT("Wrong displacement remains rejected"), WrongDatum.IsReady());
        Flat.Add(std::numeric_limits<double>::quiet_NaN(), Datum);
        TestFalse(TEXT("One invalid vertex poisons otherwise valid samples"), Flat.IsReady());
    }
    return true;
}
#endif
