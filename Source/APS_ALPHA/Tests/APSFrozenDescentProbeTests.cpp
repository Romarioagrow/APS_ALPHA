#if WITH_DEV_AUTOMATION_TESTS

#include "APSFrozenDescentProbe.h"
#include "APSContinuousWarpPixelProbe.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include <limits>

using namespace APSFrozenDescentProbe;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFrozenWarpPixelPreparationBudgetTest,
    "APS.Contracts.FrozenDescent.WarpPixelPreparationBudget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFrozenWarpPixelPreparationBudgetTest::RunTest(const FString& Parameters)
{
    using namespace APSContinuousWarpPixelProbe;
    FPreparationBudget Budget;
    TestFalse(TEXT("no preparation deadline before first Prepare"), Budget.Expired(1000.0));
    TestEqual(TEXT("no elapsed time before first Prepare"), Budget.Elapsed(1000.0), 0.0);
    Budget.Begin(1000.0);
    TestTrue(TEXT("first Prepare starts own clock"), Budget.Started());
    Budget.Begin(1040.0);
    TestEqual(TEXT("later polls cannot restart preparation"), Budget.Elapsed(1040.0), 40.0);
    TestFalse(TEXT("old flight-step 40s is not preparation timeout"), Budget.Expired(1040.0));
    TestFalse(TEXT("just before fixed cap remains pending"), Budget.Expired(1119.999));
    TestTrue(TEXT("120 seconds exactly expires"), Budget.Expired(1120.0));
    TestTrue(TEXT("later polls stay expired"), Budget.Expired(1200.0));
    TestEqual(TEXT("earlier timestamp cannot produce negative elapsed"), Budget.Elapsed(999.0), 0.0);
    TestEqual(TEXT("ordinary test gets no allowance"), PreparationAllowance(false), 0.0);
    TestEqual(TEXT("explicit WarpPixel gets fixed allowance"), PreparationAllowance(true), 120.0);
    TestEqual(TEXT("ordinary Frozen route budget unchanged"), 150.0 + DurationSeconds + PreparationAllowance(false), 150.0 + DurationSeconds);
    TestEqual(TEXT("WarpPixel whole-test increase is bounded and fixed"),
        (150.0 + DurationSeconds + PreparationAllowance(true)) - (150.0 + DurationSeconds), 120.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFrozenDescentParserTest,
    "APS.Contracts.FrozenDescent.ParserAndReferences", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFrozenDescentParserTest::RunTest(const FString& Parameters)
{
    FReference Reference;
    FString Error;
    bool bRequested = true;
    TestTrue(TEXT("absent flag succeeds"), ParseCommandLine(TEXT("-game -log"), Reference, bRequested, Error));
    TestFalse(TEXT("absent flag does not opt in"), bRequested);
    TestTrue(TEXT("null command line is absent"), ParseCommandLine(nullptr, Reference, bRequested, Error));
    TestFalse(TEXT("null command line does not opt in"), bRequested);
    TestTrue(TEXT("Lidim parses"), ParseCommandLine(TEXT("-game -APSProbeFrozenDescent=Lidim -log"), Reference, bRequested, Error));
    TestTrue(TEXT("Lidim opts in"), bRequested);
    TestEqual(TEXT("Lidim seed"), Reference.SurfaceSeed, 257455);
    TestEqual(TEXT("Lidim radius"), Reference.RadiusKm, 1280.896);
    TestEqual(TEXT("Lidim scale"), Reference.RoundedNoiseScale, 683);
    TestEqual(TEXT("Lidim intensity"), Reference.RoundedNoiseIntensity, 908291);
    TestTrue(TEXT("case-insensitive quoted option parses"), ParseCommandLine(TEXT("\"-apsprobefrozendescent=jAiM\""), Reference, bRequested, Error));
    TestEqual(TEXT("Jaim canonical name"), Reference.Name, FString(TEXT("Jaim")));
    TestEqual(TEXT("Jaim seed"), Reference.SurfaceSeed, 597932);
    TestEqual(TEXT("Jaim radius"), Reference.RadiusKm, 500.3573);
    TestEqual(TEXT("Jaim scale"), Reference.RoundedNoiseScale, 562);
    TestEqual(TEXT("Jaim intensity"), Reference.RoundedNoiseIntensity, 882510);
    for (const TCHAR* Invalid : {TEXT("-APSProbeFrozenDescent"), TEXT("-APSProbeFrozenDescent="),
        TEXT("-APSProbeFrozenDescent=Theon"), TEXT("-APSProbeFrozenDescent=Lidim -APSProbeFrozenDescent=Jaim"),
        TEXT("-APSProbeFrozenDescent=Lidim -APSProbeFrozenDescent=Lidim")})
    {
        TestFalse(FString::Printf(TEXT("reject %s"), Invalid), ParseCommandLine(Invalid, Reference, bRequested, Error));
        TestTrue(TEXT("invalid request remains explicitly requested"), bRequested);
        TestFalse(TEXT("invalid request reports cause"), Error.IsEmpty());
        TestTrue(TEXT("invalid request publishes no fallback"), Reference.Name.IsEmpty());
    }
    TestFalse(TEXT("reject unknown reference"), TryResolveReference(TEXT(""), Reference, Error));
    TestFalse(TEXT("reject default reference"), ValidateReference(Reference, Error));
    TryResolveReference(TEXT("Lidim"), Reference, Error);
    Reference.RadiusKm = std::numeric_limits<double>::quiet_NaN();
    TestFalse(TEXT("reject nonfinite reference"), ValidateReference(Reference, Error));
    Reference.RadiusKm = std::numeric_limits<double>::infinity();
    TestFalse(TEXT("reject positive infinite reference"), ValidateReference(Reference, Error));
    Reference.RadiusKm = -std::numeric_limits<double>::infinity();
    TestFalse(TEXT("reject negative infinite reference"), ValidateReference(Reference, Error));
    TryResolveReference(TEXT("Lidim"), Reference, Error);
    ++Reference.RoundedNoiseIntensity;
    TestFalse(TEXT("reject edited expected noise"), ValidateReference(Reference, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFrozenDescentFixtureTest,
    "APS.Contracts.FrozenDescent.FixtureAndRuntimeValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFrozenDescentFixtureTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGeneratedWorld> Model(NewObject<UGeneratedWorld>());
    Model->GenerationSeed = 777;
    Model->AtmosphereHeight = 23.0;
    Model->AtmosphereOpacity = 4.0;
    Model->PreviewDisplayNameOverrides.Add(TEXT("sentinel"), TEXT("keep"));
    FString Error;
    for (const TCHAR* Name : {TEXT("Lidim"), TEXT("Jaim")})
    {
        FReference Reference;
        TestTrue(TEXT("known fixture"), TryResolveReference(Name, Reference, Error));
        TestTrue(TEXT("configure ordinary model"), ConfigureModel(*Model, Reference, Error));
        TestEqual(TEXT("configured seed"), Model->PlanetSurfaceSeed, Reference.SurfaceSeed);
        TestEqual(TEXT("configured radius"), Model->PlanetRadius, Reference.RadiusKm);
        TestTrue(TEXT("configured type"), Model->PlanetType == EPlanetType::Frozen);
        TestEqual(TEXT("world seed untouched"), Model->GenerationSeed, 777);
        TestEqual(TEXT("atmosphere height untouched"), Model->AtmosphereHeight, 23.0);
        TestEqual(TEXT("atmosphere opacity untouched"), Model->AtmosphereOpacity, 4.0);
        TestEqual(TEXT("unrelated edit retained"), Model->PreviewDisplayNameOverrides.FindRef(TEXT("sentinel")), FString(TEXT("keep")));
        FRuntimeSnapshot Good;
        Good.BodyType = Good.ProfileType = EPlanetType::Frozen;
        Good.BodySeed = Good.RootSeed = Reference.SurfaceSeed;
        Good.BodyRadiusKm = static_cast<double>(static_cast<float>(Reference.RadiusKm));
        Good.PresentationScale = 1.0;
        Good.RootRadiusCm = Good.RootRadiusCodeCm = static_cast<double>(static_cast<float>(Good.BodyRadiusKm * 100000.0));
        Good.NoiseScale = Reference.RoundedNoiseScale;
        Good.NoiseIntensity = Reference.RoundedNoiseIntensity;
        Good.ProfileNoiseScale = Reference.RoundedNoiseScale + 0.49;
        Good.ProfileNoiseIntensity = Reference.RoundedNoiseIntensity - 0.49;
        Good.bRootOcean = false;
        Good.ProfileLiquidType = EAPSPlanetLiquidType::None;
        TestTrue(TEXT("expected snapshot passes"), ValidateSnapshot(Good, Reference, Error));
        TestTrue(TEXT("success clears error"), Error.IsEmpty());
        FRuntimeSnapshot Bad = Good;
        ++Bad.RootSeed;
        Bad.BodyType = EPlanetType::Volcanic;
        Bad.bRootOcean = true;
        Bad.NoiseIntensity += 1.0;
        Bad.RootRadiusCm += 1.0;
        TestFalse(TEXT("runtime mismatch is not repaired"), ValidateSnapshot(Bad, Reference, Error));
        for (const TCHAR* Field : {TEXT("root.seed"), TEXT("body.type"), TEXT("root.ocean"), TEXT("root.noiseIntensity"), TEXT("root.radiusCm")})
            TestTrue(FString::Printf(TEXT("mismatch identifies %s"), Field), Error.Contains(Field));
        Bad = Good;
        Bad.ProfileNoiseScale += 0.02;
        TestFalse(TEXT("profile rounding mismatch fails"), ValidateSnapshot(Bad, Reference, Error));
        Bad = Good;
        Bad.NoiseScale += 0.1;
        TestFalse(TEXT("fractional plugin noise fails despite same rounding"), ValidateSnapshot(Bad, Reference, Error));
        Bad = Good;
        Bad.ProfileNoiseIntensity = std::numeric_limits<double>::infinity();
        TestFalse(TEXT("nonfinite profile noise fails before conversion"), ValidateSnapshot(Bad, Reference, Error));
        Bad = Good;
        Bad.RootRadiusCodeCm += 0.01;
        TestFalse(TEXT("worker radius must exactly match plugin radius"), ValidateSnapshot(Bad, Reference, Error));
        Bad = Good;
        Bad.BodyRadiusKm = std::numeric_limits<double>::quiet_NaN();
        TestFalse(TEXT("nonfinite body radius fails"), ValidateSnapshot(Bad, Reference, Error));
        Bad = Good;
        Bad.BodyRadiusKm = Reference.RadiusKm;
        TestFalse(TEXT("unquantized body radius fails exact normal-model contract"), ValidateSnapshot(Bad, Reference, Error));
        TestFalse(TEXT("missing actors fail"), ValidateRuntime(nullptr, nullptr, nullptr, Reference, Error));
    }
    const int32 PreviousSeed = Model->PlanetSurfaceSeed;
    TestFalse(TEXT("invalid reference cannot configure model"), ConfigureModel(*Model, FReference{}, Error));
    TestEqual(TEXT("invalid reference leaves model intact"), Model->PlanetSurfaceSeed, PreviousSeed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFrozenDescentRouteTest,
    "APS.Contracts.FrozenDescent.ContinuousRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFrozenDescentRouteTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("long enough for descent and ascent"), DurationSeconds >= 70.0);
    TestTrue(TEXT("every near hold lasts at least four seconds"), HoldSeconds >= 4.0);
    TestEqual(TEXT("capture rate is eight Hz"), CaptureIntervalSeconds, 0.125);
    TestEqual(TEXT("start height"), Route(0.0).HeightKm, 30.5);
    TestEqual(TEXT("negative time clamps to start"), Route(-1.0).HeightKm, 30.5);
    TestEqual(TEXT("end height"), Route(DurationSeconds).HeightKm, 30.5);
    TestTrue(TEXT("end is complete"), Route(DurationSeconds).bFinished);
    TestFalse(TEXT("penultimate sample not complete"), Route(DurationSeconds - 0.001).bFinished);
    TestEqual(TEXT("late time stays at endpoint"), Route(DurationSeconds + 100.0).HeightKm, 30.5);
    for (double Invalid : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
        TestFalse(TEXT("nonfinite route time is explicitly invalid"), Route(Invalid).bValid);
    constexpr double Epsilon = 1.0e-5;
    for (int32 Index = 0; Index < WaypointCount; ++Index)
    {
        const double Start = Index * (HoldSeconds + LegSeconds);
        const FRouteSample Hold = Route(Start + HoldSeconds * 0.5);
        TestTrue(TEXT("waypoint is a hold"), Hold.bHold);
        TestEqual(TEXT("hold waypoint index"), Hold.WaypointIndex, Index);
        TestEqual(TEXT("hold preserves exact reference height"), Hold.HeightKm, WaypointHeightsKm[Index]);
        for (double Boundary : {Start, FMath::Min(Start + HoldSeconds, DurationSeconds)})
            TestTrue(TEXT("height continuous around boundary"),
                FMath::Abs(Route(Boundary - Epsilon).HeightKm - Route(Boundary + Epsilon).HeightKm) < 1.0e-6);
        if (Index == WaypointCount - 1) continue;
        double Previous = WaypointHeightsKm[Index];
        for (int32 Step = 1; Step <= 100; ++Step)
        {
            const FRouteSample Sample = Route(Start + HoldSeconds + LegSeconds * Step / 100.0);
            const bool bDescending = WaypointHeightsKm[Index + 1] < WaypointHeightsKm[Index];
            TestTrue(TEXT("each leg is monotonic"), bDescending ? Sample.HeightKm <= Previous + 1.0e-12 : Sample.HeightKm >= Previous - 1.0e-12);
            TestTrue(TEXT("each leg stays within endpoints"),
                Sample.HeightKm >= FMath::Min(WaypointHeightsKm[Index], WaypointHeightsKm[Index + 1]) - 1.0e-12
                && Sample.HeightKm <= FMath::Max(WaypointHeightsKm[Index], WaypointHeightsKm[Index + 1]) + 1.0e-12);
            Previous = Sample.HeightKm;
        }
    }
    for (double T = 0.0; T <= DurationSeconds; T += CaptureIntervalSeconds)
    {
        const FRouteSample Sample = Route(T);
        TestTrue(TEXT("capture samples finite and valid"), Sample.bValid && FMath::IsFinite(Sample.HeightKm));
        TestTrue(TEXT("capture samples in physical height bounds"), Sample.HeightKm >= 0.002 - 1.0e-12 && Sample.HeightKm <= 30.5 + 1.0e-12);
    }
    TestEqual(TEXT("bottom hold is labelled"), FString(Route(5.0 * (HoldSeconds + LegSeconds) + 2.0).Phase), FString(TEXT("near_hold")));
    TestEqual(TEXT("ascent leg is labelled"), FString(Route(5.0 * (HoldSeconds + LegSeconds) + HoldSeconds + 1.0).Phase), FString(TEXT("ascent")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFrozenAtmosphereBoundaryParserTest,
    "APS.Contracts.FrozenDescent.AtmosphereBoundary.Parser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFrozenAtmosphereBoundaryParserTest::RunTest(const FString& Parameters)
{
    FString Error;
    bool bRequested = true;
    for (const TCHAR* Absent : {TEXT(""), TEXT("-APSProbeFrozenDescent=Lidim -APSProbeDefaultAtmosphere"),
        TEXT("-APSProbeFrozenAtmosphereBoundaryExtra")})
    {
        TestTrue(TEXT("absent boundary flag leaves existing modes alone"), ParseAtmosphereBoundaryCommandLine(Absent, bRequested, Error));
        TestFalse(TEXT("no implicit boundary opt-in"), bRequested);
        TestTrue(TEXT("absence clears error"), Error.IsEmpty());
    }
    TestTrue(TEXT("null command line remains absent"), ParseAtmosphereBoundaryCommandLine(nullptr, bRequested, Error));
    TestFalse(TEXT("null cannot opt in"), bRequested);
    for (const TCHAR* Valid : {
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeFrozenDescent=Lidim -APSProbeDefaultAtmosphere"),
        TEXT("-APSProbeFrozenDescent=Jaim -APSProbeDefaultAtmosphere -APSProbeFrozenAtmosphereBoundary"),
        TEXT("\"-apsprobefrozenatmosphereboundary\" -apsprobefrozendescent=jAiM -apsprobedefaultatmosphere")})
    {
        TestTrue(TEXT("explicit valid boundary mode"), ParseAtmosphereBoundaryCommandLine(Valid, bRequested, Error));
        TestTrue(TEXT("boundary requested"), bRequested);
        TestTrue(TEXT("valid request clears error"), Error.IsEmpty());
    }
    for (const TCHAR* Invalid : {
        TEXT("-APSProbeFrozenAtmosphereBoundary"),
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeDefaultAtmosphere"),
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeFrozenDescent=Lidim"),
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeFrozenDescent=Theon -APSProbeDefaultAtmosphere"),
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeFrozenDescent=Lidim -APSProbeDefaultAtmosphere=false"),
        TEXT("-APSProbeFrozenAtmosphereBoundary=true -APSProbeFrozenDescent=Lidim -APSProbeDefaultAtmosphere"),
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeFrozenAtmosphereBoundary -APSProbeFrozenDescent=Lidim -APSProbeDefaultAtmosphere"),
        TEXT("-APSProbeFrozenAtmosphereBoundary -APSProbeFrozenDescent=Lidim -APSProbeFrozenDescent=Jaim -APSProbeDefaultAtmosphere")})
    {
        TestFalse(TEXT("malformed boundary request fails closed"), ParseAtmosphereBoundaryCommandLine(Invalid, bRequested, Error));
        TestTrue(TEXT("malformed request never silently falls back"), bRequested);
        TestFalse(TEXT("malformed request has a reason"), Error.IsEmpty());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFrozenAtmosphereBoundaryModelTest,
    "APS.Contracts.FrozenDescent.AtmosphereBoundary.HeightOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFrozenAtmosphereBoundaryModelTest::RunTest(const FString& Parameters)
{
    struct FCase { const TCHAR* Name; double ExpectedHeightKm; };
    const FCase Cases[] = {{TEXT("Jaim"), 16.678577423095703}, {TEXT("Lidim"), 42.696533203125}};
    FString Error;
    for (const auto& C : Cases)
    {
        FReference Reference;
        TestTrue(TEXT("known reference"), TryResolveReference(C.Name, Reference, Error));
        TStrongObjectPtr<UGeneratedWorld> Model(NewObject<UGeneratedWorld>(GetTransientPackage()));
        Model->GenerationSeed = 777;
        Model->AtmosphereHeight = 23.0;
        Model->AtmosphereOpacity = 4.0;
        Model->AtmosphereMultiScattering = 2.5;
        Model->AtmosphereRayleighScattering = 6.75;
        Model->AtmosphereColor = FLinearColor(.2f, .4f, .7f, .9f);
        Model->PreviewDisplayNameOverrides.Add(TEXT("sentinel"), TEXT("keep"));
        TestTrue(TEXT("configure existing baseline"), ConfigureModel(*Model, Reference, Error));
        TestEqual(TEXT("baseline still does not change atmosphere"), Model->AtmosphereHeight, 23.0);
        TStrongObjectPtr<UGeneratedWorld> Before(DuplicateObject<UGeneratedWorld>(Model.Get(), GetTransientPackage()));
        if (!TestNotNull(TEXT("transient property snapshot"), Before.Get())) return false;
        TestTrue(TEXT("explicit production-height configuration"), ConfigureAtmosphereBoundaryModel(*Model, Reference, Error));
        TestEqual(TEXT("production float RadiusKM/30 height"), Model->AtmosphereHeight, C.ExpectedHeightKm);
        for (TFieldIterator<FProperty> It(UGeneratedWorld::StaticClass()); It; ++It)
        {
            if (It->GetFName() == TEXT("AtmosphereHeight")) continue;
            for (int32 Index = 0; Index < It->ArrayDim; ++Index)
                TestTrue(FString(TEXT("height-only; unchanged property ")) + It->GetName(),
                    It->Identical_InContainer(Before.Get(), Model.Get(), Index));
        }
        TestTrue(TEXT("repeat height configuration succeeds"), ConfigureAtmosphereBoundaryModel(*Model, Reference, Error));
        TestEqual(TEXT("height configuration idempotent"), Model->AtmosphereHeight, C.ExpectedHeightKm);
        TestFalse(TEXT("invalid reference cannot configure atmosphere"), ConfigureAtmosphereBoundaryModel(*Model, FReference{}, Error));
        TestEqual(TEXT("invalid reference changes no height"), Model->AtmosphereHeight, C.ExpectedHeightKm);
        ++Model->PlanetSurfaceSeed;
        TestFalse(TEXT("mismatched model is not repaired"), ConfigureAtmosphereBoundaryModel(*Model, Reference, Error));
        TestEqual(TEXT("mismatched model keeps its seed"), Model->PlanetSurfaceSeed, Reference.SurfaceSeed + 1);
        TestEqual(TEXT("mismatched model keeps its height"), Model->AtmosphereHeight, C.ExpectedHeightKm);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFrozenAtmosphereBoundaryRouteTest,
    "APS.Contracts.FrozenDescent.AtmosphereBoundary.Route", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFrozenAtmosphereBoundaryRouteTest::RunTest(const FString& Parameters)
{
    struct FCase { double PlanetRadiusKm, AtmosphereHeightKm, SurfaceAnchorRadiusKm, ShellHeightKm; };
    // Nonzero ground offsets ensure the planner cannot mistake sea-level height
    // for the route's above-ground coordinate. Last case checks adaptive >60 km.
    const FCase Cases[] = {
        {499.3572998046875, 16.678577423095703, 502.35728, 13.6785972277832},
        {1279.89599609375, 42.696533203125, 1282.896, 39.69652929687504},
        {499.0, 100.0, 502.0, 97.0}
    };
    FString Error;
    TestEqual(TEXT("baseline duration unchanged"), DurationSeconds, 94.0);
    TestEqual(TEXT("baseline start unchanged"), Route(0.0).HeightKm, 30.5);
    TestEqual(TEXT("boundary uses same eight Hz budget"), CaptureIntervalSeconds, .125);
    for (const auto& C : Cases)
    {
        FAtmosphereBoundaryRoute Plan;
        if (!TestTrue(TEXT("build from actual shell and anchor"), BuildAtmosphereBoundaryRoute(
            C.PlanetRadiusKm, C.AtmosphereHeightKm, C.SurfaceAnchorRadiusKm, Plan, Error))) continue;
        TestTrue(TEXT("correct above-ground shell height"), FMath::Abs(Plan.ShellHeightKm - C.ShellHeightKm) < 1.0e-9);
        TestEqual(TEXT("eleven mirrored anchors"), Plan.WaypointHeightsKm.Num(), 11);
        TestTrue(TEXT("includes reported 34.5 km"), Plan.WaypointHeightsKm.Contains(34.5));
        const double OuterHeight = FMath::Max(60.0, C.ShellHeightKm + 10.0);
        TestEqual(TEXT("outer hold clears either moon shell"), Plan.WaypointHeightsKm[0], OuterHeight);
        TestTrue(TEXT("explicit just-outside hold"), Plan.WaypointHeightsKm.Contains(Plan.ShellHeightKm + .05));
        TestTrue(TEXT("explicit just-inside hold"), Plan.WaypointHeightsKm.Contains(Plan.ShellHeightKm - .05));
        TestEqual(TEXT("negative time clamps to outer hold"), AtmosphereBoundaryRoute(-1.0, Plan).HeightKm, OuterHeight);
        TestTrue(TEXT("final sample complete"), AtmosphereBoundaryRoute(DurationSeconds, Plan).bFinished);
        TestFalse(TEXT("penultimate sample not complete"), AtmosphereBoundaryRoute(DurationSeconds - .001, Plan).bFinished);
        TestEqual(TEXT("late time clamps to outer hold"), AtmosphereBoundaryRoute(DurationSeconds + 100.0, Plan).HeightKm, OuterHeight);
        for (int32 Index = 0; Index < WaypointCount; ++Index)
        {
            const double Start = Index * (HoldSeconds + LegSeconds);
            const auto Hold = AtmosphereBoundaryRoute(Start + HoldSeconds * .5, Plan);
            TestTrue(TEXT("every anchor is a valid hold"), Hold.bValid && Hold.bHold);
            TestEqual(TEXT("hold exact height"), Hold.HeightKm, Plan.WaypointHeightsKm[Index]);
            TestEqual(TEXT("mirror height"), Plan.WaypointHeightsKm[Index], Plan.WaypointHeightsKm[WaypointCount - 1 - Index]);
            for (double Boundary : {Start, FMath::Min(Start + HoldSeconds, DurationSeconds)})
                TestTrue(TEXT("continuous at hold/leg boundaries"), FMath::Abs(
                    AtmosphereBoundaryRoute(Boundary - 1.0e-5, Plan).HeightKm
                    - AtmosphereBoundaryRoute(Boundary + 1.0e-5, Plan).HeightKm) < 1.0e-6);
            if (Index == WaypointCount - 1) continue;
            double Previous = Plan.WaypointHeightsKm[Index];
            for (int32 Step = 1; Step <= 100; ++Step)
            {
                const auto Sample = AtmosphereBoundaryRoute(Start + HoldSeconds + LegSeconds * Step / 100.0, Plan);
                TestTrue(TEXT("leg finite and monotonic"), Sample.bValid && FMath::IsFinite(Sample.HeightKm)
                    && (Index < 5 ? Sample.HeightKm <= Previous + 1.0e-12 : Sample.HeightKm >= Previous - 1.0e-12));
                TestTrue(TEXT("leg stays within endpoints"), Sample.HeightKm >= FMath::Min(
                    Plan.WaypointHeightsKm[Index], Plan.WaypointHeightsKm[Index + 1]) - 1.0e-12
                    && Sample.HeightKm <= FMath::Max(Plan.WaypointHeightsKm[Index], Plan.WaypointHeightsKm[Index + 1]) + 1.0e-12);
                Previous = Sample.HeightKm;
            }
        }
        for (double Invalid : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
            TestFalse(TEXT("nonfinite time fails closed"), AtmosphereBoundaryRoute(Invalid, Plan).bValid);
        auto Broken = Plan;
        Broken.WaypointHeightsKm[0] += 1.0;
        TestFalse(TEXT("edited asymmetric route rejected"), AtmosphereBoundaryRoute(0.0, Broken).bValid);
        Broken = Plan; Broken.ShellHeightKm += .1;
        TestFalse(TEXT("route cannot reuse a different shell"), AtmosphereBoundaryRoute(0.0, Broken).bValid);
    }
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    const double Infinity = std::numeric_limits<double>::infinity();
    const FCase InvalidCases[] = {{NaN, 10, 500, 0}, {499, Infinity, 500, 0}, {499, 10, NaN, 0},
        {0, 10, 500, 0}, {499, 0, 500, 0}, {499, 10, -1, 0}, {499, 3, 500, 0},
        {499, 33.5, 500, 0}, {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 500, 0}};
    for (const auto& C : InvalidCases)
    {
        FAtmosphereBoundaryRoute Plan;
        BuildAtmosphereBoundaryRoute(499, 17, 502, Plan, Error);
        TestFalse(TEXT("invalid radii/low shell/duplicate anchors fail"), BuildAtmosphereBoundaryRoute(
            C.PlanetRadiusKm, C.AtmosphereHeightKm, C.SurfaceAnchorRadiusKm, Plan, Error));
        TestTrue(TEXT("failed plan publishes no partial or stale waypoints"), Plan.WaypointHeightsKm.IsEmpty());
        TestFalse(TEXT("failed plan explains cause"), Error.IsEmpty());
        TestFalse(TEXT("empty plan cannot sample fallback route"), AtmosphereBoundaryRoute(0.0, Plan).bValid);
    }
    AddInfo(TEXT("Production shell-height / route contracts only; no exact moon climate, saved-scene, GPU or visual acceptance claim."));
    return true;
}

#endif
