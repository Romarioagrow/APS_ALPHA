#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudLayers.h"
#include <limits>

namespace APSPlanetCloudLayersTests
{
inline APSPlanetCloudWeather::FWeather Water()
{
    APSPlanetCloudWeather::FWeather Base;
    Base.Enabled = true;
    Base.Condensate = APSPlanetCloudWeather::ECondensate::Water;
    Base.BottomKm = 3.2f;
    Base.ThicknessKm = 2.2f;
    Base.Coverage = .552f;
    Base.Density = 1.f;
    return Base;
}
inline bool Same(const APSPlanetCloudLayers::FDeck& A, const APSPlanetCloudLayers::FDeck& B)
{
    return A.BottomKm == B.BottomKm && A.ThicknessKm == B.ThicknessKm
        && A.Coverage == B.Coverage && A.Density == B.Density;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudLayersPolicyTest, "APS.Gameplay.World.PlanetSurface.Clouds.Layers.Policy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCloudLayersPolicyTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetCloudLayers;
    using namespace APSPlanetCloudLayersTests;
    const auto Base = Water();
    const auto Stack = Resolve(Base, 100.);
    TestTrue(TEXT("Water enables the isolated three-deck policy"), Stack.Enabled);
    TestEqual(TEXT("Low deck retains climate sea-level altitude"), Stack.Low.BottomKm, Base.BottomKm);
    TestTrue(TEXT("Middle deck is tallest, high deck is thin"),
        Stack.Middle.ThicknessKm > Stack.Low.ThicknessKm && Stack.Low.ThicknessKm > Stack.High.ThicknessKm);
    TestTrue(TEXT("Decks have strictly positive gaps"),
        Stack.Low.BottomKm + Stack.Low.ThicknessKm < Stack.Middle.BottomKm
        && Stack.Middle.BottomKm + Stack.Middle.ThicknessKm < Stack.High.BottomKm);
    TestEqual(TEXT("Bounds start at low deck"), Stack.BoundsBottomKm(), Stack.Low.BottomKm);
    TestEqual(TEXT("Bounds include high deck"), Stack.BoundsTopKm(), Stack.High.BottomKm + Stack.High.ThicknessKm);
    const float Column = Stack.Low.Density * Stack.Low.ThicknessKm
        + Stack.Middle.Density * Stack.Middle.ThicknessKm + Stack.High.Density * Stack.High.ThicknessKm;
    TestTrue(TEXT("Nominal optical column is repartitioned rather than tripled"),
        FMath::IsNearlyEqual(Column, Base.Density * Base.ThicknessKm, 1.e-5f));
    // No radius enters Resolve: the same physical climate applies to a small
    // moon, an Earth-size body, or a 70000 km planet without inflating clouds.
    for (double RadiusKm : { 50., 6370., 10000., 70000. })
    {
        const auto Repeat = Resolve(Base, 100.);
        TestTrue(FString::Printf(TEXT("Fixed physical decks at radius %.0f km"), RadiusKm),
            Repeat.Enabled == Stack.Enabled && Same(Repeat.Low, Stack.Low)
            && Same(Repeat.Middle, Stack.Middle) && Same(Repeat.High, Stack.High));
    }
    for (double Height : { 4., 10., 16., 100., 2000. })
    {
        const auto Candidate = Resolve(Base, Height);
        if (Candidate.Enabled)
        {
            TestTrue(TEXT("All layers lie below 85 percent of atmosphere"), Candidate.BoundsTopKm() <= Height * .85);
            for (const auto* Deck : { &Candidate.Low, &Candidate.Middle, &Candidate.High })
                TestTrue(TEXT("Finite positive bounded deck properties"),
                    FMath::IsFinite(Deck->BottomKm) && Deck->BottomKm > 0.f
                    && FMath::IsFinite(Deck->ThicknessKm) && Deck->ThicknessKm > 0.f
                    && FMath::IsFinite(Deck->Coverage) && Deck->Coverage > 0.f && Deck->Coverage <= 1.f
                    && FMath::IsFinite(Deck->Density) && Deck->Density > 0.f && Deck->Density <= Base.Density);
        }
    }
    const auto Thin = Resolve(Base, 4.);
    TestFalse(TEXT("Insufficient vertical room retains original single-layer fallback"), Thin.Enabled);
    TestEqual(TEXT("Disabled stack has no bounds bottom"), Thin.BoundsBottomKm(), 0.f);
    TestEqual(TEXT("Disabled stack has no bounds top"), Thin.BoundsTopKm(), 0.f);
    TestEqual(TEXT("Input climate bottom unchanged"), Base.BottomKm, 3.2f);
    TestEqual(TEXT("Input climate thickness unchanged"), Base.ThicknessKm, 2.2f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudLayersControlsTest, "APS.Gameplay.World.PlanetSurface.Clouds.Layers.Controls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCloudLayersControlsTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetCloudLayers;
    using namespace APSPlanetCloudLayersTests;
    FAPSResolvedPlanetSurfaceProfile Profile;
    Profile.PlanetType = EPlanetType::Terrestrial;
    Profile.Humidity = .6f;
    Profile.Temperature = .45f;
    Profile.AtmosphericPressure = 1.f;
    Profile.BiomeSeed = 424242;
    FAPSPlanetCloudSettings Settings;
    Settings.CoverageScale = 1.;
    const auto Base = APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings);
    const auto Stack = Resolve(Base, 100.);
    TestTrue(TEXT("Real climate resolver supplies the layered candidate"), Stack.Enabled);
    Settings.CoverageScale = .5;
    const auto HalfCoverage = Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings), 100.);
    TestTrue(TEXT("Coverage control scales every deck without height changes"), HalfCoverage.Enabled
        && FMath::IsNearlyEqual(HalfCoverage.Low.Coverage, Stack.Low.Coverage * .5f)
        && FMath::IsNearlyEqual(HalfCoverage.Middle.Coverage, Stack.Middle.Coverage * .5f)
        && FMath::IsNearlyEqual(HalfCoverage.High.Coverage, Stack.High.Coverage * .5f)
        && HalfCoverage.BoundsBottomKm() == Stack.BoundsBottomKm() && HalfCoverage.BoundsTopKm() == Stack.BoundsTopKm());
    Settings.CoverageScale = 0.;
    TestFalse(TEXT("Coverage zero cannot leave wisps behind"),
        Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings), 100.).Enabled);
    Settings.CoverageScale = 1.;
    Settings.DensityScale = .5;
    const auto HalfDensity = Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings), 100.);
    TestTrue(TEXT("Density control scales all three optical masses"), HalfDensity.Enabled
        && FMath::IsNearlyEqual(HalfDensity.Low.Density, Stack.Low.Density * .5f)
        && FMath::IsNearlyEqual(HalfDensity.Middle.Density, Stack.Middle.Density * .5f)
        && FMath::IsNearlyEqual(HalfDensity.High.Density, Stack.High.Density * .5f));
    Settings.DensityScale = 0.;
    TestFalse(TEXT("Density zero disables all decks"),
        Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings), 100.).Enabled);
    Settings.DensityScale = 1.;
    Settings.AltitudeScale = 1.5;
    const auto Raised = Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings), 100.);
    TestTrue(TEXT("Altitude raises entire stack without shrinking layer gaps"), Raised.Enabled
        && Raised.Low.BottomKm > Stack.Low.BottomKm
        && FMath::IsNearlyEqual(Raised.Middle.BottomKm - Stack.Middle.BottomKm, Raised.Low.BottomKm - Stack.Low.BottomKm, 1.e-5f)
        && FMath::IsNearlyEqual(Raised.High.BottomKm - Stack.High.BottomKm, Raised.Low.BottomKm - Stack.Low.BottomKm, 1.e-5f));
    Settings = FAPSPlanetCloudSettings();
    Settings.CoverageScale = 1.;
    Settings.FeatureScale = 2.; Settings.WindScale = 0.; Settings.SeedOffset = 77;
    const auto Repatterned = Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings), 100.);
    TestTrue(TEXT("Pattern controls do not move or inflate cloud shells"), Repatterned.Enabled
        && Same(Repatterned.Low, Stack.Low) && Same(Repatterned.Middle, Stack.Middle) && Same(Repatterned.High, Stack.High));
    Profile.AtmosphericPressure = 0.f;
    TestFalse(TEXT("Vacuum climate never creates a stack"),
        Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., false, 30.f, Settings), 100.).Enabled);
    Profile.AtmosphericPressure = 1.f;
    TestFalse(TEXT("Authored worlds are not opted in"),
        Resolve(APSPlanetCloudWeather::Resolve(Profile, 100., true, 30.f, Settings), 100.).Enabled);
    TestEqual(TEXT("Layer policy never rerolls terrain seed"), Profile.BiomeSeed, 424242);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudLayersFallbackTest, "APS.Gameplay.World.PlanetSurface.Clouds.Layers.Fallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCloudLayersFallbackTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetCloudLayers;
    using APSPlanetCloudWeather::ECondensate;
    auto Base = APSPlanetCloudLayersTests::Water();
    Base.Condensate = ECondensate::Ice;
    TestTrue(TEXT("Ice crystals may use the three-deck candidate"), Resolve(Base, 100.).Enabled);
    for (const auto Species : { ECondensate::None, ECondensate::Ammonia, ECondensate::AcidAerosol,
        ECondensate::Dust, ECondensate::Ash, ECondensate::Hydrocarbon })
    {
        Base.Condensate = Species;
        TestFalse(TEXT("Foreign condensates retain original single integral"), Resolve(Base, 100.).Enabled);
    }
    Base.Condensate = ECondensate::Water;
    Base.Enabled = false;
    TestFalse(TEXT("Disabled climate stays disabled"), Resolve(Base, 100.).Enabled);
    Base.Enabled = true;
    for (double Height : { 0., -1., std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity() })
        TestFalse(TEXT("Invalid atmosphere is rejected"), Resolve(Base, Height).Enabled);
    for (float* Value : { &Base.BottomKm, &Base.ThicknessKm, &Base.Coverage, &Base.Density })
    {
        const float Original = *Value;
        for (float Invalid : { 0.f, -1.f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() })
        {
            *Value = Invalid;
            TestFalse(TEXT("Invalid resolved deck properties are rejected"), Resolve(Base, 100.).Enabled);
        }
        *Value = Original;
    }
    Base.ThicknessKm = std::numeric_limits<float>::max();
    TestFalse(TEXT("Overflowing shell arithmetic is rejected"), Resolve(Base, 1.e100).Enabled);
    Base = APSPlanetCloudLayersTests::Water();
    Base.BottomKm = 1.e20f;
    TestFalse(TEXT("Float precision cannot silently collapse separate shells"), Resolve(Base, 1.e30).Enabled);
    Base = APSPlanetCloudLayersTests::Water();
    Base.ThicknessKm = .3f;
    TestFalse(TEXT("Thin climate cannot invert the high-wisp and low-cumulus widths"), Resolve(Base, 100.).Enabled);
    Base = APSPlanetCloudLayersTests::Water();
    Base.Coverage = 20.f; Base.Density = 1000.f;
    const auto Bounded = Resolve(Base, 100.);
    TestTrue(TEXT("Finite out-of-range optics remain bounded"), Bounded.Enabled);
    for (const auto* Deck : { &Bounded.Low, &Bounded.Middle, &Bounded.High })
        TestTrue(TEXT("Coverage and density clamps apply to each deck"), Deck->Coverage <= 1.f && Deck->Density <= 3.f);
    return true;
}
#endif
