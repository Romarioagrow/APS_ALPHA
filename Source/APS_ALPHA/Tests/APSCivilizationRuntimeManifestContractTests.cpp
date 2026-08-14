#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSCivilizationRuntimeManifestDeterminismTest,
	"APS.Civilization.RuntimeManifest.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCivilizationRuntimeManifestDeterminismTest::RunTest(const FString& Parameters)
{
	USpawnParameters* Spawn = NewObject<USpawnParameters>();
	Spawn->CivilizationName = TEXT("Aster Union");
	Spawn->FoundingPopulation = 24000;
	Spawn->BP_HomeSpaceship = ASpaceship::StaticClass();
	const FGuid HomeSystemId(0x12345678, 0x90abcdef, 0x11223344, 0x55667788);
	const FSoftClassPath BaseClass(AAPSCivilizationBaseModule::StaticClass());
	const FSoftClassPath PadClass(AAPSCivilizationLandingPad::StaticClass());

	const FAPSCivilizationRuntimeManifest First =
		FAPSCivilizationRuntimeManifestFactory::Build(
			271828, HomeSystemId, TEXT("SYS0/S0/P2"), Spawn, BaseClass, PadClass);
	const FAPSCivilizationRuntimeManifest Second =
		FAPSCivilizationRuntimeManifestFactory::Build(
			271828, HomeSystemId, TEXT("sys0/s0/p2"), Spawn, BaseClass, PadClass);

	FString ValidationReason;
	TestTrue(TEXT("manifest v1 is structurally valid"),
		First.IsStructurallyValid(&ValidationReason));
	TestEqual(TEXT("same canonical civilization keeps manifest id"),
		First.ManifestId, Second.ManifestId);
	TestEqual(TEXT("same canonical civilization keeps civilization id"),
		First.CivilizationId, Second.CivilizationId);
	TestEqual(TEXT("starter entity count is fixed for the foundation slice"),
		First.Entities.Num(), 3);
	for (int32 Index = 0; Index < First.Entities.Num(); ++Index)
	{
		TestEqual(FString::Printf(TEXT("entity %d keeps stable id"), Index),
			First.Entities[Index].StableId, Second.Entities[Index].StableId);
		TestEqual(FString::Printf(TEXT("entity %d keeps deterministic name"), Index),
			First.Entities[Index].MakeDeterministicActorName(),
			Second.Entities[Index].MakeDeterministicActorName());
	}

	Spawn->CivilizationName = TEXT("Renamed Aster Union");
	const FAPSCivilizationRuntimeManifest Renamed =
		FAPSCivilizationRuntimeManifestFactory::Build(
			271828, HomeSystemId, TEXT("SYS0/S0/P2"), Spawn, BaseClass, PadClass);
	TestEqual(TEXT("display-name edits do not mutate civilization identity"),
		First.CivilizationId, Renamed.CivilizationId);
	TestEqual(TEXT("display-name edits do not mutate starter manifest identity"),
		First.ManifestId, Renamed.ManifestId);
	TestNotEqual(TEXT("display-name remains a mutable payload"),
		First.CivilizationDescriptor, Renamed.CivilizationDescriptor);

	const FAPSCivilizationRuntimeManifest SecondCivilization =
		FAPSCivilizationRuntimeManifestFactory::Build(
			271828, HomeSystemId, TEXT("SYS0/S0/P2"), Spawn, BaseClass, PadClass, 1);
	TestNotEqual(TEXT("persisted ordinal creates a distinct civilization"),
		First.CivilizationId, SecondCivilization.CivilizationId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSCivilizationRuntimeManifestMigrationTest,
	"APS.Civilization.RuntimeManifest.VersionMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCivilizationRuntimeManifestMigrationTest::RunTest(const FString& Parameters)
{
	USpawnParameters* Spawn = NewObject<USpawnParameters>();
	Spawn->BP_HomeSpaceship = ASpaceship::StaticClass();
	FAPSCivilizationRuntimeManifest Legacy =
		FAPSCivilizationRuntimeManifestFactory::Build(
			10101, FGuid(1, 2, 3, 4), TEXT("SYS0/S0/P0"), Spawn,
			FSoftClassPath(AAPSCivilizationBaseModule::StaticClass()),
			FSoftClassPath(AAPSCivilizationLandingPad::StaticClass()));
	Legacy.SchemaVersion = 0;
	for (FAPSCivilizationManifestEntity& Entity : Legacy.Entities)
	{
		Entity.OwnerCivilizationId.Invalidate();
		Entity.FactionId.Invalidate();
	}

	FString MigrationReason;
	TestTrue(TEXT("schema zero migrates to manifest v1"),
		Legacy.MigrateToLatest(&MigrationReason));
	TestEqual(TEXT("migration records current schema"), Legacy.SchemaVersion,
		FAPSCivilizationRuntimeManifest::LatestSchemaVersion);
	for (const FAPSCivilizationManifestEntity& Entity : Legacy.Entities)
	{
		TestEqual(TEXT("migration restores entity civilization ownership"),
			Entity.OwnerCivilizationId, Legacy.CivilizationId);
		TestEqual(TEXT("migration restores entity faction ownership"),
			Entity.FactionId, Legacy.FactionId);
	}
	return true;
}

#endif
