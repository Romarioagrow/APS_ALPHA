#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSOrbitalWaterAppearance.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSOrbitalWaterAppearanceTest,
    "APS.Gameplay.World.PlanetSurface.OrbitalWaterAppearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSOrbitalWaterAppearanceTest::RunTest(const FString& Parameters)
{
    using namespace APSOrbitalWaterAppearance;
    TestTrue(TEXT("Owned full-scale water body eligible (planet or moon)"),
        IsEligible(true, false, EAPSPlanetLiquidType::Water, 1.0, FVector::OneVector));
    TestFalse(TEXT("Borrowed root preserved"), IsEligible(false, false, EAPSPlanetLiquidType::Water, 1.0, FVector::OneVector));
    TestFalse(TEXT("Manual planet preserved"), IsEligible(true, true, EAPSPlanetLiquidType::Water, 1.0, FVector::OneVector));
    TestFalse(TEXT("Preview preserved"), IsEligible(true, false, EAPSPlanetLiquidType::Water, 0.01, FVector::OneVector));
    TestFalse(TEXT("Scaled root preserved"), IsEligible(true, false, EAPSPlanetLiquidType::Water, 1.0, FVector(2.0)));
    for (const EAPSPlanetLiquidType Liquid : {EAPSPlanetLiquidType::None, EAPSPlanetLiquidType::Lava, EAPSPlanetLiquidType::Ammonia})
        TestFalse(TEXT("Other liquids untouched"), IsEligible(true, false, Liquid, 1.0, FVector::OneVector));
    TestEqual(TEXT("Underwater keeps saved optics"), Weight(-100.0, 1.0f), 0.0f);
    TestEqual(TEXT("Near flight keeps saved optics"), Weight(2000000.0, 1.0f), 0.0f);
    TestEqual(TEXT("Midpoint blends continuously"), Weight(11000000.0, 1.0f), 0.5f);
    TestEqual(TEXT("Orbit reaches target"), Weight(20000000.0, 1.0f), 1.0f);
    TestEqual(TEXT("Disabled restores saved optics"), Weight(30000000.0, 0.0f), 0.0f);
    TestEqual(TEXT("Negative strength clamps"), Weight(30000000.0, -1.0f), 0.0f);
    TestEqual(TEXT("Excess strength clamps"), Weight(30000000.0, 2.0f), 1.0f);
    TestEqual(TEXT("Invalid height fails closed"), Weight(std::numeric_limits<double>::quiet_NaN(), 1.0f), 0.0f);
    TestEqual(TEXT("Infinite height fails closed"), Weight(std::numeric_limits<double>::infinity(), 1.0f), 0.0f);
    TestEqual(TEXT("Invalid strength fails closed"), Weight(30000000.0, std::numeric_limits<float>::quiet_NaN()), 0.0f);
    TestEqual(TEXT("Orbital specular target"), Specular(0.65f, 1.0f), 0.25f);
    TestEqual(TEXT("Orbital roughness target"), Roughness(0.18f, 1.0f), 0.26f);
    for (const float Saved : {0.0f, 0.1f, 0.18f, 0.25f, 0.26f, 0.65f, 1.0f})
    {
        TestEqual(TEXT("Near specular is exactly authored"), Specular(Saved, 0.0f), Saved);
        TestEqual(TEXT("Near roughness is exactly authored"), Roughness(Saved, 0.0f), Saved);
        TestTrue(TEXT("Refinement never increases orbital reflectance"), Specular(Saved, 1.0f) <= Saved);
        TestTrue(TEXT("Refinement never polishes an authored rough ocean"), Roughness(Saved, 1.0f) >= Saved);
    }
    float Previous = 0.0f;
    for (int32 Km = 0; Km <= 300; ++Km)
    {
        const float Current = Weight(Km * 100000.0, 1.0f);
        TestTrue(TEXT("Altitude response is bounded, monotonic and smooth"),
            Current >= Previous && Current <= 1.0f && Current - Previous < 0.01f);
        Previous = Current;
    }
    return true;
}
#endif
