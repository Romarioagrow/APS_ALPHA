#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

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
		for (const EOrder Build : {EOrder::BuildStation, EOrder::BuildShipyard, EOrder::BuildHeadquarters})
		{
			TestEqual(TEXT("Only construction raises stations, shipyards and HQs"), DivisionCan(Division, Build),
				Division == EDivision::Construction);
		}
		TestNotEqual(TEXT("Cycling changes the division"), NextDivision(Division), Division);
	}

	// Construction after an outpost: each order raises its structure; the station comes first.
	EStructure Structure = EStructure::Count;
	TestTrue(TEXT("A station order raises a station"), StructureOf(EOrder::BuildStation, Structure) && Structure == EStructure::Station);
	TestTrue(TEXT("A shipyard order raises a shipyard"), StructureOf(EOrder::BuildShipyard, Structure) && Structure == EStructure::Shipyard);
	TestTrue(TEXT("A HQ order raises a headquarters"),
		StructureOf(EOrder::BuildHeadquarters, Structure) && Structure == EStructure::Headquarters);
	TestFalse(TEXT("An outpost is not a structure order"), StructureOf(EOrder::BuildOutpost, Structure));
	TestFalse(TEXT("A survey is not a structure order"), StructureOf(EOrder::Survey, Structure));
	TestFalse(TEXT("A station needs no station"), NeedsStation(EStructure::Station));
	TestTrue(TEXT("A shipyard needs a station"), NeedsStation(EStructure::Shipyard));
	TestTrue(TEXT("A HQ needs a station"), NeedsStation(EStructure::Headquarters));
	TestTrue(TEXT("Saved orders keep their numbers: the new orders come after Return"),
		static_cast<uint8>(EOrder::BuildStation) > static_cast<uint8>(EOrder::Return) && LastOrder == EOrder::Expedition);
	for (const EOrder Order : {EOrder::Move, EOrder::Survey, EOrder::BuildOutpost, EOrder::Return, EOrder::BuildStation,
		EOrder::BuildShipyard, EOrder::BuildHeadquarters, EOrder::Expedition})
	{
		TestFalse(TEXT("Every order has a name"), OrderName(Order).IsEmpty());
	}
	for (int32 Index = 0; Index < static_cast<int32>(EDivision::Count); ++Index)
	{
		const EDivision Division = static_cast<EDivision>(Index);
		TestEqual(TEXT("Expeditions are for exploration and science"), DivisionCan(Division, EOrder::Expedition),
			Division == EDivision::Exploration || Division == EDivision::Science);
	}

	// Anomalies: the same world always hides the same one, about two worlds in five hide one, the site off the poles.
	FAnomalyTraits Traits;
	int32 Kind = -1, KindAgain = -2;
	FVector Site, SiteAgain;
	int32 Hidden = 0;
	for (int32 World = 0; World < 400; ++World)
	{
		const FString Key = FString::Printf(TEXT("BODY:SYS0/S0/P%d"), World);
		const bool bHas = RollAnomaly(Key, Traits, Kind, Site);
		TestEqual(TEXT("A world's anomaly is the same every time"), RollAnomaly(Key, Traits, KindAgain, SiteAgain), bHas);
		if (bHas)
		{
			++Hidden;
			TestEqual(TEXT("The same kind"), KindAgain, Kind);
			TestTrue(TEXT("The same site"), SiteAgain.Equals(Site, 1.0e-6));
			TestTrue(TEXT("A known kind"), Kind >= 0 && Kind < AnomalyKindCount);
			TestTrue(TEXT("A unit direction"), FMath::IsNearlyEqual(Site.Size(), 1.0, 1.0e-4));
			TestTrue(TEXT("Away from the poles"), FMath::Abs(Site.Z) <= FMath::Sin(FMath::DegreesToRadians(60.0)) + 1.0e-4);
			TestFalse(TEXT("A barren world shows no biosignature"), Kind == 3);
		}
	}
	TestTrue(TEXT("About two worlds in five hide an anomaly"), Hidden > 120 && Hidden < 200);
	for (int32 Each = 0; Each < AnomalyKindCount; ++Each)
	{
		TestFalse(TEXT("Every anomaly has a name and a story"), AnomalyName(Each).IsEmpty() || AnomalyStory(Each).IsEmpty());
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
	TestTrue(TEXT("A station takes longer than an outpost"),
		WorkSeconds(EOrder::BuildStation, EDivision::Construction, 1) > WorkSeconds(EOrder::BuildOutpost, EDivision::Construction, 1));
	TestTrue(TEXT("A HQ takes longest"),
		WorkSeconds(EOrder::BuildHeadquarters, EDivision::Construction, 1) > WorkSeconds(EOrder::BuildShipyard, EDivision::Construction, 1));
	TestEqual(TEXT("A station at the world speeds work up by a quarter"),
		WorkSeconds(EOrder::BuildShipyard, EDivision::Construction, 2, true) * 1.25,
		WorkSeconds(EOrder::BuildShipyard, EDivision::Construction, 2, false), 1.0e-9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFleetCommandSaveExtrasTest,
	"APS.Gameplay.Fleet.SaveExtras",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFleetCommandSaveExtrasTest::RunTest(const FString& Parameters)
{
	// Civilization save version 4 (A6): the fleet's structures, the slipways' queues and the investigated anomalies come
	// back as they were written, after the older blocks.
	FAPSFleetSaveData Saved;
	FAPSFleetSaveData::FStructure& Station = Saved.Structures.AddDefaulted_GetRef();
	Station.Kind = static_cast<uint8>(APSFleet::EStructure::Shipyard);
	Station.BodyKey = TEXT("BODY:Vinawur Moon");
	Station.RelativeTransform = FTransform(FQuat(FVector::UpVector, 0.5), FVector(1.0e8, -2.0e7, 3.0e6));
	Station.Name = TEXT("SHIPYARD VINAWUR MOON");
	Station.ActorName = TEXT("APS_Fleet_Shipyard_2");
	FAPSFleetSaveData::FShipyardJob& Job = Saved.ShipyardJobs.AddDefaulted_GetRef();
	Job.ClassPath = TEXT("/Game/APS/APS_ALPHA/Core/Spaceships/XXS/BP_Spaceship_XXS_P1_05.BP_Spaceship_XXS_P1_05_C");
	Job.SizeClass = 1;
	Job.Name = TEXT("XXS P1 05");
	Job.Length = 14.0f;
	Job.Progress = 0.42f;
	Job.YardKey = TEXT("ACTOR:APS_Fleet_Shipyard_2");
	Saved.Investigations.Emplace(TEXT("BODY:Bup Moon"), 2);

	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	FAPSFleetSaveData::SerializeExtras(Writer, Saved);
	FAPSFleetSaveData Loaded;
	FMemoryReader Reader(Bytes, true);
	FAPSFleetSaveData::SerializeExtras(Reader, Loaded);
	TestFalse(TEXT("The extras read back cleanly"), Reader.IsError());
	TestTrue(TEXT("Nothing is left over"), Reader.AtEnd());
	if (!TestEqual(TEXT("One structure"), Loaded.Structures.Num(), 1) || !TestEqual(TEXT("One slipway job"), Loaded.ShipyardJobs.Num(), 1)
		|| !TestEqual(TEXT("One investigation"), Loaded.Investigations.Num(), 1))
	{
		return false;
	}
	TestEqual(TEXT("The structure's kind"), Loaded.Structures[0].Kind, Station.Kind);
	TestEqual(TEXT("Its world"), Loaded.Structures[0].BodyKey, Station.BodyKey);
	TestTrue(TEXT("Its place"), Loaded.Structures[0].RelativeTransform.Equals(Station.RelativeTransform, 1.0e-3));
	TestEqual(TEXT("Its actor name (a stable key)"), Loaded.Structures[0].ActorName, Station.ActorName);
	TestEqual(TEXT("The job's class"), Loaded.ShipyardJobs[0].ClassPath, Job.ClassPath);
	TestEqual(TEXT("Its progress"), Loaded.ShipyardJobs[0].Progress, Job.Progress);
	TestEqual(TEXT("Its shipyard"), Loaded.ShipyardJobs[0].YardKey, Job.YardKey);
	TestEqual(TEXT("An investigation on foot"), Loaded.Investigations[0].Value, static_cast<uint8>(2));
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

	// From a berth on the surface to a moon straight behind the planet (01.10, b6-anomaly-1: the science ships flew into
	// the planet toward the detour point and crawled at 2 km/s): every leg of the way stays outside the planet.
	const FVector MoonSlot(-Radius * 8.0, 0.0, 0.0);
	FVector Ship(Radius * 1.0005, 0.0, 0.0);
	double LowestLeg = TNumericLimits<double>::Max();
	bool bClear = false;
	for (int32 Leg = 0; Leg < 32 && !bClear; ++Leg)
	{
		if (!Detour(Ship, MoonSlot, Centre, Radius, Point))
		{
			bClear = true;
			break;
		}
		const FVector Next = RoundBody(Ship, MoonSlot, Centre, Radius);
		for (int32 Sample = 1; Sample <= 20; ++Sample)
		{
			LowestLeg = FMath::Min(LowestLeg, FMath::Lerp(Ship, Next, Sample / 20.0).Size());
		}
		Ship = Next;
	}
	TestTrue(TEXT("The way round a planet clears it within a few legs"), bClear);
	TestTrue(TEXT("No leg of it passes inside the planet"), LowestLeg > Radius);
	TestTrue(TEXT("From the surface the first leg climbs straight out"),
		RoundBody(FVector(Radius * 1.0005, 0.0, 0.0), MoonSlot, Centre, Radius).Equals(FVector(Radius * 1.4, 0.0, 0.0), 1.0));
	return true;
}

#endif
