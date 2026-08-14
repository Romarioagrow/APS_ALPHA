#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Quests/Adapters/APSCivilizationQuestAdapter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace APSQuestCivilizationAdapterTests
{
	FAPSCivilizationRuntimeManifest MakeManifest()
	{
		USpawnParameters* Spawn = NewObject<USpawnParameters>();
		Spawn->BP_HomeSpaceship = ASpaceship::StaticClass();
		FAPSCivilizationRuntimeManifest Manifest =
			FAPSCivilizationRuntimeManifestFactory::Build(
				314159, FGuid(10, 20, 30, 40), TEXT("SYS0/S0/P0"), Spawn,
				FSoftClassPath(AAPSCivilizationBaseModule::StaticClass()),
				FSoftClassPath(AAPSCivilizationLandingPad::StaticClass()));
		Manifest.MaterializationState = EAPSCivilizationMaterializationState::Materialized;
		return Manifest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestCivilizationReadyAdapterTest,
	"APS.Quest.Adapters.CivilizationReadyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestCivilizationReadyAdapterTest::RunTest(const FString& Parameters)
{
	using namespace APSQuestCivilizationAdapterTests;
	FAPSCivilizationRuntimeManifest Manifest = MakeManifest();
	FAPSCivilizationQuestReadySnapshot Materialized;
	FString Reason;
	TestTrue(TEXT("Materialized manifest normalizes"),
		FAPSCivilizationQuestAdapter::NormalizeReadyManifest(
			Manifest, Materialized, Reason));
	TestEqual(TEXT("Manifest is the owner stream"),
		Materialized.ReadyEvent.StreamId, Manifest.ManifestId);
	TestEqual(TEXT("Ready fact targets exact base"), Materialized.ReadyEvent.Target.Guid,
		Manifest.FindEntity(EAPSCivilizationEntityRole::BaseModule)->StableId);
	TestEqual(TEXT("Home system preserves canonical identity"), Materialized.HomeSystem.Guid,
		Manifest.HomeSystemId);

	Manifest.MaterializationState = EAPSCivilizationMaterializationState::LoadedFromSave;
	FAPSCivilizationQuestReadySnapshot Loaded;
	TestTrue(TEXT("Loaded manifest normalizes"),
		FAPSCivilizationQuestAdapter::NormalizeReadyManifest(Manifest, Loaded, Reason));
	TestEqual(TEXT("Provenance shares semantic EventId"),
		Loaded.ReadyEvent.EventId, Materialized.ReadyEvent.EventId);
	TestEqual(TEXT("Provenance shares semantic CorrelationId"),
		Loaded.ReadyEvent.CorrelationId, Materialized.ReadyEvent.CorrelationId);
	TestEqual(TEXT("Provenance shares stream and sequence"),
		Loaded.ReadyEvent.StreamId, Materialized.ReadyEvent.StreamId);
	TestEqual(TEXT("Ready sequence is one"), Loaded.ReadyEvent.Sequence,
		static_cast<int64>(1));
	TestNotEqual(TEXT("Provenance remains diagnostic payload"),
		Loaded.ReadyEvent.ContextLabels[0], Materialized.ReadyEvent.ContextLabels[0]);

	FAPSCivilizationRuntimeManifest Blocked = Manifest;
	Blocked.MaterializationState = EAPSCivilizationMaterializationState::Blocked;
	TestFalse(TEXT("Blocked is not a completion fact"),
		FAPSCivilizationQuestAdapter::NormalizeReadyManifest(Blocked, Loaded, Reason));

	FAPSCivilizationRuntimeManifest BrokenChain = Manifest;
	FAPSCivilizationManifestEntity* Ship =
		BrokenChain.FindEntity(EAPSCivilizationEntityRole::SelectedShip);
	Ship->ParentStableId = BrokenChain.ManifestId;
	TestFalse(TEXT("Broken parent chain is rejected"),
		FAPSCivilizationQuestAdapter::NormalizeReadyManifest(
			BrokenChain, Loaded, Reason));
	TestTrue(TEXT("Parent-chain diagnostic is explicit"), Reason.Contains(TEXT("parent chain")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
