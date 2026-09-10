#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSCivilizationManifestSanitizationTest,
	"APS.Civilization.Manifest.SanitizationAndRuntimeModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCivilizationManifestSanitizationTest::RunTest(const FString& Parameters)
{
	USpawnParameters* Spawn = NewObject<USpawnParameters>();
	TestNotNull(TEXT("spawn parameters are constructible"), Spawn);
	if (!Spawn)
	{
		return false;
	}

	Spawn->CivilizationName = TEXT("   ");
	Spawn->FoundingPopulation = -50;
	Spawn->StartingCredits = -100;
	Spawn->TechnologyLevel = 99;
	Spawn->StartingFleetSize = 1000;
	Spawn->StarOutposts = 100;
	Spawn->PlanetOutposts = -3;
	Spawn->OrbitalOutposts = 0;
	Spawn->GroundOutposts = 100;
	Spawn->ExplorationDivisionLevel = -1;
	Spawn->FleetDivisionLevel = 100;
	Spawn->SanitizeForGeneration();

	TestEqual(TEXT("blank name receives a stable default"), Spawn->CivilizationName,
		FString(TEXT("APOSFERA CIVILIZATION")));
	TestEqual(TEXT("population is positive"), Spawn->FoundingPopulation, 1);
	TestEqual(TEXT("credits are non-negative"), Spawn->StartingCredits, static_cast<int64>(0));
	TestEqual(TEXT("technology is bounded"), Spawn->TechnologyLevel, 10);
	TestEqual(TEXT("fleet is bounded for runtime spawning"), Spawn->StartingFleetSize,
		USpawnParameters::MaxStartingFleetSize);
	TestEqual(TEXT("star outposts are bounded"), Spawn->StarOutposts,
		USpawnParameters::MaxInfrastructurePerCategory);
	TestEqual(TEXT("planet outposts are non-negative"), Spawn->PlanetOutposts, 0);
	TestEqual(TEXT("home orbital station is mandatory"), Spawn->OrbitalOutposts, 1);
	TestEqual(TEXT("ground settlements are bounded"), Spawn->GroundOutposts,
		USpawnParameters::MaxInfrastructurePerCategory);
	TestEqual(TEXT("division lower bound"), Spawn->ExplorationDivisionLevel, 0);
	TestEqual(TEXT("division upper bound"), Spawn->FleetDivisionLevel, 20);

	const int32 ExpectedInfrastructure =
		USpawnParameters::MaxInfrastructurePerCategory * 2 + 1;
	TestEqual(TEXT("manifest infrastructure count includes the home station"),
		Spawn->GetPlannedInfrastructureActorCount(), ExpectedInfrastructure);
	TestEqual(TEXT("manifest total includes HQ, shipyard, fleet and infrastructure"),
		Spawn->GetPlannedPhysicalActorCount(),
		2 + USpawnParameters::MaxStartingFleetSize + ExpectedInfrastructure);

	UCivilization* Civilization = NewObject<UCivilization>();
	TestNotNull(TEXT("runtime civilization is constructible"), Civilization);
	if (Civilization)
	{
		Civilization->InitializeFromSpawnParameters(Spawn);
		TestEqual(TEXT("runtime fleet mirrors the sanitized physical manifest"),
			Civilization->FleetSize, Spawn->StartingFleetSize);
		TestEqual(TEXT("runtime orbital infrastructure mirrors the manifest"),
			Civilization->Infrastructure.OrbitalStations, Spawn->OrbitalOutposts);
		TestEqual(TEXT("runtime divisions mirror sanitized readiness"),
			Civilization->Divisions.FleetCommand, Spawn->FleetDivisionLevel);
	}

	return true;
}

#endif
