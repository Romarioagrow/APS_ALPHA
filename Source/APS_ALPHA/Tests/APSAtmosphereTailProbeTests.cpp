#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
// Rio 06.10 (packaged build): editor-only material/texture APIs inside; game targets skip this file, editor automation is unchanged.
#include "APSAtmosphereTailProbe.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmosphereTailParserTest,
    "APS.Contracts.FrozenDescent.AtmosphereTail.Parser",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSAtmosphereTailParserTest::RunTest(const FString& Parameters)
{
    const FString Required = TEXT("-APSProbeFrozenDescent=Jaim -APSProbeFrozenAtmosphereBoundary -APSProbeDefaultAtmosphere ");
    int32 Mode = 17; FString Error;
    TestTrue(TEXT("Absent succeeds"), APSAtmosphereTailProbe::Parse(TEXT(""), Mode, Error));
    TestEqual(TEXT("Absent is not a candidate"), Mode, -1);
    for (int32 Expected : {0, 1})
    {
        TestTrue(TEXT("Explicit mode succeeds"), APSAtmosphereTailProbe::Parse(
            *(Required + FString::Printf(TEXT("-APSProbeAtmosphereTail=%d"), Expected)), Mode, Error));
        TestEqual(TEXT("Mode retained"), Mode, Expected);
    }
    for (const TCHAR* Invalid : {TEXT("-APSProbeAtmosphereTail"), TEXT("-APSProbeAtmosphereTail=2"),
        TEXT("-APSProbeAtmosphereTail=-1"), TEXT("-APSProbeAtmosphereTail=1x"),
        TEXT("-APSProbeAtmosphereTail=0 -APSProbeAtmosphereTail=1"),
        TEXT("-APSProbeAtmosphereTail=1 -APSProbeContinuousWarpPixel")})
        TestFalse(TEXT("Malformed or competing candidate rejected"), APSAtmosphereTailProbe::Parse(*(Required + Invalid), Mode, Error));
    TestFalse(TEXT("No boundary rejected"), APSAtmosphereTailProbe::Parse(TEXT("-APSProbeAtmosphereTail=0"), Mode, Error));
    TestFalse(TEXT("No body rejected"), APSAtmosphereTailProbe::Parse(
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeDefaultAtmosphere -APSProbeAtmosphereTail=1"), Mode, Error));
    TestFalse(TEXT("No default atmosphere rejected"), APSAtmosphereTailProbe::Parse(
        TEXT("-APSProbeFrozenDescent=Jaim -APSProbeFrozenAtmosphereBoundary -APSProbeAtmosphereTail=1"), Mode, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmosphereTailGroundParserTest,
    "APS.Contracts.FrozenDescent.AtmosphereTail.GroundParser",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSAtmosphereTailGroundParserTest::RunTest(const FString& Parameters)
{
    int32 Mode = 17; FString Error; bool bGround = true;
    TestTrue(TEXT("Null command line succeeds"), APSAtmosphereTailProbe::Parse(nullptr, Mode, Error, &bGround));
    TestEqual(TEXT("Null keeps ordinary mode"), Mode, -1);
    TestFalse(TEXT("Null clears ground mode"), bGround);
    TestTrue(TEXT("Absent flag succeeds"), APSAtmosphereTailProbe::Parse(TEXT(""), Mode, Error, &bGround));
    TestFalse(TEXT("Absent flag keeps ordinary route"), bGround);
    for (const TCHAR* Body : {TEXT("Lidim"), TEXT("Jaim")})
    {
        const FString Required = FString::Printf(
            TEXT("-APSProbeFrozenDescent=%s -APSProbeFrozenAtmosphereBoundary -APSProbeDefaultAtmosphere "), Body);
        for (int32 Expected : {0, 1})
        {
            const FString Tail = Required + FString::Printf(TEXT("-APSProbeAtmosphereTail=%d"), Expected);
            TestTrue(TEXT("Existing boundary mode succeeds"), APSAtmosphereTailProbe::Parse(*Tail, Mode, Error, &bGround));
            TestFalse(TEXT("Existing boundary route unchanged"), bGround);
            TestTrue(TEXT("Explicit ground mode succeeds"), APSAtmosphereTailProbe::Parse(
                *(Tail + TEXT(" -APSProbeFrozenAtmosphereGround")), Mode, Error, &bGround));
            TestEqual(TEXT("Ground preserves explicit tail mode"), Mode, Expected);
            TestTrue(TEXT("Ground route requested"), bGround);
            TestTrue(TEXT("Ground flag order and case independent"), APSAtmosphereTailProbe::Parse(
                *(TEXT("-apsprobefrozenatmosphereground ") + Tail), Mode, Error, &bGround));
            TestTrue(TEXT("Leading ground flag retained"), bGround);
        }
    }
    const FString Ground = TEXT("-APSProbeFrozenAtmosphereGround ");
    const FString Required = TEXT("-APSProbeFrozenDescent=Lidim -APSProbeFrozenAtmosphereBoundary -APSProbeDefaultAtmosphere ");
    TestTrue(TEXT("Ground may observe ordinary production without a lease"),
        APSAtmosphereTailProbe::Parse(*(Ground + Required), Mode, Error, &bGround));
    TestEqual(TEXT("Production does not select a diagnostic lease"), Mode, -1);
    TestTrue(TEXT("Production ground route retained"), bGround);
    for (const FString& Invalid : {
        Ground,
        Ground + TEXT("-APSProbeFrozenDescent=Lidim -APSProbeDefaultAtmosphere -APSProbeAtmosphereTail=0"),
        Ground + TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeDefaultAtmosphere -APSProbeAtmosphereTail=0"),
        Ground + TEXT("-APSProbeFrozenDescent=Lidim -APSProbeFrozenAtmosphereBoundary -APSProbeAtmosphereTail=0"),
        Ground + Required + TEXT("-APSProbeAtmosphereTail=0 -APSProbeContinuousWarpPixel"),
        Ground + Required + TEXT("-APSProbeAtmosphereTail=0 -APSProbeFrozenAtmosphereGround"),
        Required + TEXT("-APSProbeAtmosphereTail=0 -APSProbeFrozenAtmosphereGround=1"),
        Ground + Required + TEXT("-APSProbeAtmosphereTail=0 -APSProbeAtmosphereTail=1"),
        Ground + Required + TEXT("-APSProbeAtmosphereTail=0 -APSProbeFrozenAtmosphereBoundary")})
    {
        bGround = true;
        TestFalse(TEXT("Malformed or incomplete ground mode rejected"), APSAtmosphereTailProbe::Parse(*Invalid, Mode, Error, &bGround));
        TestFalse(TEXT("Rejected mode cannot select ground route"), bGround);
        TestFalse(TEXT("Rejected mode explains failure"), Error.IsEmpty());
        TestFalse(TEXT("Lease parser also rejects without output pointer"), APSAtmosphereTailProbe::Parse(*Invalid, Mode, Error));
    }
    return true;
}
#endif
