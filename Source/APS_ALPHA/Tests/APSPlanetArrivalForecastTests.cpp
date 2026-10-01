#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/World/APSPlanetArrivalForecast.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetArrivalForecastTest,
	"APS.World.PlanetArrivalForecast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetArrivalForecastTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetArrivalForecast;
	// A body of 1000 km at the origin; activation reach 96 radii.
	constexpr double Radius = 1.0e8;
	constexpr double Reach = 96.0 * Radius;
	constexpr double Lead = 20.0;

	// Head on at 100 000 km/s from 1 000 000 km: the surface about 10 s away.
	const FResult HeadOn = Evaluate(FVector(1.0e11, 0.0, 0.0), FVector(1.1e11, 0.0, 0.0), 1.0, Radius);
	TestEqual(TEXT("Closing speed is the approach speed"), HeadOn.ClosingSpeedCmPerSecond, 1.0e10, 1.0);
	TestEqual(TEXT("Seconds to the surface"), HeadOn.SecondsToSurface, 9.99, 1.0e-6);
	TestEqual(TEXT("A head-on line passes through the centre"), HeadOn.MissDistanceCm, 0.0, 1.0);
	TestTrue(TEXT("Head on within the lead arrives"), IsArriving(HeadOn, Lead, Reach, false));
	TestFalse(TEXT("A shorter lead waits"), IsArriving(HeadOn, 5.0, Reach, false));
	TestFalse(TEXT("A zero lead turns the forecast off"), IsArriving(HeadOn, 0.0, Reach, false));

	// Flying away never arrives.
	const FResult Away = Evaluate(FVector(1.1e11, 0.0, 0.0), FVector(1.0e11, 0.0, 0.0), 1.0, Radius);
	TestFalse(TEXT("Receding does not arrive"), IsArriving(Away, Lead, Reach, false));
	TestEqual(TEXT("Receding has no closing speed"), Away.ClosingSpeedCmPerSecond, 0.0);

	// Standing still or no time passed: no forecast.
	TestFalse(TEXT("Standing still does not arrive"),
		IsArriving(Evaluate(FVector(1.0e11, 0.0, 0.0), FVector(1.0e11, 0.0, 0.0), 1.0, Radius), Lead, Reach, false));
	TestFalse(TEXT("A zero step does not arrive"),
		IsArriving(Evaluate(FVector(1.0e11, 0.0, 0.0), FVector(1.1e11, 0.0, 0.0), 0.0, Radius), Lead, Reach, false));

	// Passing 500 000 km to the side of the body at 1 000 000 km: beyond the reach and the cone.
	const FResult Passing = Evaluate(FVector(1.0e11, 5.0e10, 0.0), FVector(1.1e11, 5.0e10, 0.0), 1.0, Radius);
	TestEqual(TEXT("The line misses by the offset"), Passing.MissDistanceCm, 5.0e10, 1.0e3);
	TestFalse(TEXT("A flyby does not arrive"), IsArriving(Passing, Lead, Reach, false));
	TestFalse(TEXT("Even a held forecast drops a flyby"), IsArriving(Passing, Lead, Reach, true));

	// Aimed roughly at it (a line 50 000 km off at 1 000 000 km, inside the narrow cone) arrives.
	const FResult Aimed = Evaluate(FVector(1.0e11, 5.0e9, 0.0), FVector(1.1e11, 5.0e9, 0.0), 1.0, Radius);
	TestTrue(TEXT("Roughly aimed arrives"), IsArriving(Aimed, Lead, Reach, false));

	// A held forecast is looser: 25 s away arrives only while it holds.
	const FResult Slower = Evaluate(FVector(1.0e11, 0.0, 0.0), FVector(1.04e11, 0.0, 0.0), 1.0, Radius);
	TestFalse(TEXT("25 s is beyond a new forecast"), IsArriving(Slower, Lead, Reach, false));
	TestTrue(TEXT("25 s keeps a held forecast"), IsArriving(Slower, Lead, Reach, true));

	// Astronomical distances stay finite: 4 AU at 2.5 million km/s is about 4 minutes away.
	const FResult Far = Evaluate(FVector(6.0e13, 0.0, 0.0), FVector(6.025e13, 0.0, 0.0), 1.0, Radius);
	TestEqual(TEXT("Far closing speed"), Far.ClosingSpeedCmPerSecond, 2.5e11, 1.0);
	TestEqual(TEXT("Far seconds to the surface"), Far.SecondsToSurface, 239.9996, 1.0e-3);
	TestFalse(TEXT("Four AU away is not yet arrival"), IsArriving(Far, Lead, Reach, false));

	// Confirmation: a new body needs two passes in a row, the confirmed one survives a single miss.
	TTracker<int32> Tracker;
	TestFalse(TEXT("One pass is not a course"), Tracker.Step(5, false));
	TestEqual(TEXT("Nothing confirmed after one pass"), Tracker.Confirmed, 0);
	TestTrue(TEXT("Two passes confirm"), Tracker.Step(5, false));
	TestEqual(TEXT("Confirmed body"), Tracker.Confirmed, 5);
	TestFalse(TEXT("A holding forecast stays"), Tracker.Step(7, true));
	TestEqual(TEXT("A better newcomer does not steal a holding forecast"), Tracker.Confirmed, 5);
	TestFalse(TEXT("One miss keeps it"), Tracker.Step(0, false));
	TestEqual(TEXT("Still confirmed after one miss"), Tracker.Confirmed, 5);
	TestTrue(TEXT("Two misses end it"), Tracker.Step(0, false));
	TestEqual(TEXT("Ended"), Tracker.Confirmed, 0);
	Tracker.Step(6, false);
	TestFalse(TEXT("A change of target starts over"), Tracker.Step(8, false));
	TestTrue(TEXT("The new target confirms on its second pass"), Tracker.Step(8, false));
	TestEqual(TEXT("New target"), Tracker.Confirmed, 8);
	Tracker.Reset();
	TestEqual(TEXT("Reset clears"), Tracker.Confirmed, 0);
	return true;
}

#endif
