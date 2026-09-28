#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeStreamingPolicy.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeStreamingPolicyTest,
    "APS.Gameplay.World.PlanetSurface.StreamingSelectionPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeStreamingPolicyTest::RunTest(const FString& Parameters)
{
    using namespace APSWorldScapeStreamingPolicy;
    const double Rejected = TNumericLimits<double>::Max();
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    const double Infinity = std::numeric_limits<double>::infinity();
    for (double Invalid : {-1.0, NaN, Infinity})
        TestEqual(TEXT("Invalid distance rejected"), Score(Invalid, 100, 500, false, 0.72), Rejected);
    TestEqual(TEXT("Outside preload rejected"), Score(501, 100, 500, false, 0.72), Rejected);
    TestEqual(TEXT("Invalid radius rejected"), Score(200, NaN, 500, false, 0.72), Rejected);
    TestEqual(TEXT("Invalid limit rejected"), Score(200, 100, NaN, false, 0.72), Rejected);
    TestEqual(TEXT("Invalid bias rejected"), Score(200, 100, 500, true, NaN), Rejected);
    TestEqual(TEXT("Inside radius clamps to zero"), Score(50, 100, 500, false, 0.72), 0.0);
    TestFalse(TEXT("Invalid candidate cannot win empty selection"), Prefer(Rejected, false, TEXT("A"), Rejected, false, TEXT("")));
    TestFalse(TEXT("NaN cannot win"), Prefer(NaN, false, TEXT("A"), Rejected, false, TEXT("")));

    // Evaluate all candidates in every ordering, including an ineligible entry.
    const double Scores[] = {90, 10, Rejected};
    const int32 Orders[][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    for (const auto& Order : Orders)
    {
        double Best = Rejected;
        FString BestKey;
        int32 Winner = INDEX_NONE;
        for (int32 Index : Order)
        {
            const FString Key = FString::FromInt(Index);
            if (Prefer(Scores[Index], false, Key, Best, false, BestKey))
            { Best = Scores[Index]; BestKey = Key; Winner = Index; }
        }
        TestEqual(TEXT("Nearest wins independently of iteration order"), Winner, 1);
    }
    const double Resident = Score(200, 100, 500, true, 0.72);
    TestTrue(TEXT("Hysteresis holds near tie"), Resident < Score(190, 100, 500, false, 0.72));
    TestTrue(TEXT("Hysteresis releases to clearly closer family"), Score(120, 100, 500, false, 0.72) < Resident);
    TestTrue(TEXT("Current wins exact tie"), Prefer(10, true, TEXT("Z"), 10, false, TEXT("A")));
    TestFalse(TEXT("Tie never removes current"), Prefer(10, false, TEXT("A"), 10, true, TEXT("Z")));
    TestTrue(TEXT("Noncurrent tie is deterministic"), Prefer(10, false, TEXT("A"), 10, false, TEXT("Z")));
    TestFalse(TEXT("Tie comparison is strict"), Prefer(10, false, TEXT("A"), 10, false, TEXT("A")));
    return true;
}
#endif
