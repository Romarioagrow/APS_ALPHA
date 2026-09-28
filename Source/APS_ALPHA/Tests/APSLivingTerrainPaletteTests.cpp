#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSLivingTerrainPalette.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSLivingTerrainPaletteTest,
    "APS.Gameplay.World.PlanetSurface.LivingPaletteDetail",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSLivingTerrainPaletteTest::RunTest(const FString& Parameters)
{
    using namespace APSLivingTerrainPalette;
    FAPSResolvedPlanetSurfaceProfile Profile;
    Profile.Palette.Dryland = FLinearColor(0.3f, 0.2f, 0.08f, 0.7f);
    Profile.Palette.Highland = FLinearColor(0.1f, 0.24f, 0.16f, 1.0f);
    Profile.Palette.Slope = FLinearColor(0.12f, 0.11f, 0.09f, 1.0f);
    const FLinearColor Original = Profile.Palette.Dryland;
    for (const auto Archetype : {EAPSPlanetSurfaceArchetype::Rocky, EAPSPlanetSurfaceArchetype::Temperate,
        EAPSPlanetSurfaceArchetype::Oceanic, EAPSPlanetSurfaceArchetype::Biosphere,
        EAPSPlanetSurfaceArchetype::Desert, EAPSPlanetSurfaceArchetype::Cryogenic,
        EAPSPlanetSurfaceArchetype::Magmatic, EAPSPlanetSurfaceArchetype::Metallic,
        EAPSPlanetSurfaceArchetype::ExoticChemical})
    {
        Profile.Archetype = Archetype;
        for (const auto Liquid : {EAPSPlanetLiquidType::None, EAPSPlanetLiquidType::Water,
            EAPSPlanetLiquidType::Lava, EAPSPlanetLiquidType::Ammonia})
        {
            Profile.LiquidType = Liquid;
            const bool Expected = Liquid == EAPSPlanetLiquidType::Water
                && (Archetype == EAPSPlanetSurfaceArchetype::Temperate
                    || Archetype == EAPSPlanetSurfaceArchetype::Oceanic
                    || Archetype == EAPSPlanetSurfaceArchetype::Biosphere);
            TestEqual(TEXT("Only living/temperate/ocean water palettes opt in"), Allows(Profile), Expected);
            FLinearColor ExpectedRock = Expected
                ? FMath::Lerp(Profile.Palette.Highland, Profile.Palette.Slope, 0.7f) : Profile.Palette.Highland;
            ExpectedRock.A = Profile.Palette.Highland.A;
            TestEqual(TEXT("Only exposed living rock receives the authored stone tint"),
                ExposedRock(Profile), ExpectedRock);
            TestEqual(TEXT("Zero strength restores exact accepted endpoint"), CoolDryland(Profile, 0.0f), Original);
            TestEqual(TEXT("Negative strength restores exact accepted endpoint"), CoolDryland(Profile, -1.0f), Original);
            TestEqual(TEXT("Invalid strength restores exact accepted endpoint"),
                CoolDryland(Profile, std::numeric_limits<float>::quiet_NaN()), Original);
            const auto Actual = CoolDryland(Profile, 1.0f);
            if (!Expected) TestEqual(TEXT("Other families remain identical"), Actual, Original);
            else
            {
                auto Midpoint = FMath::Lerp(Original, Profile.Palette.Highland, 0.5f);
                Midpoint.A = Original.A;
                TestEqual(TEXT("New endpoint stays within authored palette"), Actual, Midpoint);
                TestNotEqual(TEXT("Temperature endpoints no longer collapse"), Actual, Original);
                TestEqual(TEXT("Strength is capped"), CoolDryland(Profile, 5.0f), Actual);
            }
            TestEqual(TEXT("Profile and hot dryland endpoint are never mutated"), Profile.Palette.Dryland, Original);
        }
    }
    Profile.Archetype = EAPSPlanetSurfaceArchetype::Temperate;
    Profile.LiquidType = EAPSPlanetLiquidType::Water;
    Profile.Palette.Highland.R = std::numeric_limits<float>::quiet_NaN();
    TestEqual(TEXT("Invalid secondary endpoint fails closed"), CoolDryland(Profile, 1.0f), Original);
    Profile.Palette.Highland.R = std::numeric_limits<float>::infinity();
    TestEqual(TEXT("Infinite secondary endpoint fails closed"), CoolDryland(Profile, 1.0f), Original);
    return true;
}
#endif
