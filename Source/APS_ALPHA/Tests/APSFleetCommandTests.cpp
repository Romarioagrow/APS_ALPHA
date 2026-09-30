#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFleetCommandRulesTest,
	"APS.Gameplay.Fleet.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFleetCommandRulesTest::RunTest(const FString& Parameters)
{
	using namespace APSFleet;
	// Divisions by class, the flagship leading the main fleet.
	TestEqual(TEXT("The flagship leads the main fleet"), DefaultDivision(ESpaceshipSizeClass::XXS, true), EDivision::MainFleet);
	TestEqual(TEXT("XS scouts explore"), DefaultDivision(ESpaceshipSizeClass::XS, false), EDivision::Exploration);
	TestEqual(TEXT("S ships do science"), DefaultDivision(ESpaceshipSizeClass::S, false), EDivision::Science);
	TestEqual(TEXT("M ships build"), DefaultDivision(ESpaceshipSizeClass::M, false), EDivision::Construction);
	TestEqual(TEXT("L ships join the line"), DefaultDivision(ESpaceshipSizeClass::L, false), EDivision::MainFleet);

	// Orders by division.
	for (int32 Index = 0; Index < static_cast<int32>(EDivision::Count); ++Index)
	{
		const EDivision Division = static_cast<EDivision>(Index);
		TestTrue(TEXT("Every division moves"), DivisionCan(Division, EOrder::Move));
		TestTrue(TEXT("Every division returns"), DivisionCan(Division, EOrder::Return));
		TestEqual(TEXT("Only exploration and science survey"), DivisionCan(Division, EOrder::Survey),
			Division == EDivision::Exploration || Division == EDivision::Science);
		TestEqual(TEXT("Only construction builds outposts"), DivisionCan(Division, EOrder::BuildOutpost),
			Division == EDivision::Construction);
		TestNotEqual(TEXT("Cycling changes the division"), NextDivision(Division), Division);
	}
	TestEqual(TEXT("Cycling wraps around"), NextDivision(EDivision::Construction), EDivision::MainFleet);
	TestEqual(TEXT("Exploration surveys"), SurveyBy(EDivision::Exploration), ESurvey::Surveyed);
	TestEqual(TEXT("Science studies"), SurveyBy(EDivision::Science), ESurvey::Studied);
	TestTrue(TEXT("A study is more than a survey"), ESurvey::Studied > ESurvey::Surveyed);

	// Work time falls with the division level.
	TestTrue(TEXT("A higher level works faster"),
		WorkSeconds(EOrder::Survey, EDivision::Exploration, 3) < WorkSeconds(EOrder::Survey, EDivision::Exploration, 0));
	TestTrue(TEXT("Outposts take longer than surveys"),
		WorkSeconds(EOrder::BuildOutpost, EDivision::Construction, 1) > WorkSeconds(EOrder::Survey, EDivision::Exploration, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFleetCommandFlightTest,
	"APS.Gameplay.Fleet.Autopilot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFleetCommandFlightTest::RunTest(const FString& Parameters)
{
	using namespace APSFleet;
	constexpr double Dt = 1.0 / 60.0;
	// From rest the ship gains speed smoothly: no single frame jumps.
	double Speed = 0.0;
	for (int32 Frame = 0; Frame < 1500; ++Frame)
	{
		const double Next = StepSpeed(Speed, 1.5e13, 1.0e12, 1.0e13, Dt);
		TestTrue(TEXT("Speed never falls while far away"), Next >= Speed);
		Speed = Next;
	}
	// About 22 s from rest to 10 million km/s (the 2.5x-a-second gain, the CRUISE band's feel): a planet 1 AU away.
	TestTrue(TEXT("It reaches interplanetary speed within 25 seconds"), Speed >= 1.0e12);
	TestTrue(TEXT("It gains at most 2.5x a second once moving"), FMath::Pow(2.5, Dt) * 1.0001 >= StepSpeed(1.0e6, 1.5e13, 1.0e12, 1.0e13, Dt) / 1.0e6);
	TestTrue(TEXT("Close to the target it slows with the distance"), StepSpeed(1.0e9, 1000.0, 1.0e12, 1.0e13, Dt) <= 800.0 + 1.0e-6);
	TestTrue(TEXT("Near a surface the speed is one surface distance a second"),
		StepSpeed(1.0e9, 1.0e12, 5.0e6, 1.0e13, Dt) <= 5.0e6 + 1.0e-6);
	TestTrue(TEXT("The class cap holds"), StepSpeed(1.0e12, 1.0e15, 1.0e15, 1.0e7, Dt) <= 1.0e7 + 1.0e-6);
	FSpaceshipClassPreset Scout;
	Scout.bSupportsSpaceWrap = false;
	FSpaceshipClassPreset Cruiser;
	TestTrue(TEXT("Ships without SpaceWrap stay at planetary speed"), ClassCap(Scout) < ClassCap(Cruiser));

	// Detours: round a planet in the way, never through it.
	const FVector Centre(0.0, 0.0, 0.0);
	constexpr double Radius = 6.0e8;
	FVector Point;
	TestTrue(TEXT("A way straight through a planet gets a detour"),
		Detour(FVector(-2.0e9, 1.0e7, 0.0), FVector(2.0e9, 0.0, 0.0), Centre, Radius, Point));
	TestTrue(TEXT("The detour point clears the planet"), Point.Size() >= Radius * 1.3);
	TestFalse(TEXT("A clear way needs none"), Detour(FVector(-2.0e9, 2.0e9, 0.0), FVector(2.0e9, 2.0e9, 0.0), Centre, Radius, Point));
	TestFalse(TEXT("A way ending in orbit of the planet is not refused"),
		Detour(FVector(0.0, 2.0e9, 0.0), FVector(0.0, SlotRadius(Radius), 0.0), Centre, Radius, Point));
	TestTrue(TEXT("The orbit slot is outside the planet"), SlotRadius(Radius) > Radius * 1.2);
	TestTrue(TEXT("A low berth behind the planet is reached round it"),
		Detour(FVector(0.0, 2.0e9, 0.0), FVector(0.0, -Radius * 1.01, 0.0), Centre, Radius, Point));
	TestFalse(TEXT("A low berth approached from above needs no detour"),
		Detour(FVector(0.0, -2.0e9, 0.0), FVector(0.0, -Radius * 1.01, 0.0), Centre, Radius, Point));
	return true;
}

#endif
