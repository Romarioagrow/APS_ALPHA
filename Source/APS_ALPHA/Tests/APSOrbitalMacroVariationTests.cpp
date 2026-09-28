#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSOrbitalMacroVariation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSOrbitalMacroFamilyGateTest,
    "APS.Contracts.PlanetSurface.OrbitalMacroFamilyGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSOrbitalMacroFamilyGateTest::RunTest(const FString& Parameters)
{
    for (uint8 Value = 0; Value <= APSPlanetTypes::LastValue; ++Value)
    {
        const auto Type = static_cast<EPlanetType>(Value);
        const bool Expected = Type == EPlanetType::Terrestrial || Type == EPlanetType::Oasis;
        TestEqual(FString::Printf(TEXT("Only rendered families opt in: %u"),Value),
            APSOrbitalMacroVariation::Allows(Type),Expected);
    }
    TestFalse(TEXT("Unknown future family keeps the native material"),
        APSOrbitalMacroVariation::Allows(static_cast<EPlanetType>(255)));
    return true;
}
#endif
