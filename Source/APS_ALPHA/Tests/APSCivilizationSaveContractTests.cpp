#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/SavedActorData.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSCivilizationSaveArchiveContractTest,
	"APS.Civilization.Save.ManifestAndStableEntityRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCivilizationSaveArchiveContractTest::RunTest(const FString& Parameters)
{
	USpawnParameters* Spawn = NewObject<USpawnParameters>();
	Spawn->CivilizationName = TEXT("APS SAVE CONTRACT");
	Spawn->BP_HomeSpaceship = ASpaceship::StaticClass();
	FAPSCivilizationRuntimeManifest Manifest =
		FAPSCivilizationRuntimeManifestFactory::Build(
			515151, FGuid(0xaabbccdd, 0x10203040, 0x50607080, 0x90abcdef),
			TEXT("SYS0/S0/P4"), Spawn,
			FSoftClassPath(AAPSCivilizationBaseModule::StaticClass()),
			FSoftClassPath(AAPSCivilizationLandingPad::StaticClass()));
	Manifest.MaterializationState = EAPSCivilizationMaterializationState::Materialized;
	for (int32 Index = 0; Index < Manifest.Entities.Num(); ++Index)
	{
		FAPSCivilizationManifestEntity& Entity = Manifest.Entities[Index];
		Entity.PlanetRelativeTransform = FTransform(
			FRotator(2.0 * Index, 11.0 * Index, -3.0 * Index),
			FVector(1000.0 * Index, -250.0 * Index, 7000.0 + 500.0 * Index),
			FVector::OneVector);
		Entity.bHasPersistedTransform = true;
	}

	UGameSave* SourceSave = NewObject<UGameSave>();
	SourceSave->SaveSlotName = TEXT("APS_CIVILIZATION_MEMORY_ONLY");
	SourceSave->bHasCivilizationManifest = true;
	SourceSave->CivilizationManifest = Manifest;
	FActorSaveData& ActorRecord = SourceSave->ActorSaveDataArray.AddDefaulted_GetRef();
	ActorRecord.ActorName = TEXT("APS_CIVILIZATION_ENTITY");
	ActorRecord.ActorTransform = Manifest.Entities[2].PlanetRelativeTransform;
	ActorRecord.StableEntityId = Manifest.Entities[2].StableId;
	ActorRecord.ActorClass = Manifest.Entities[2].ActorClass.ToString();

	TArray<uint8> Bytes;
	TestTrue(TEXT("UE SaveGame archive writes manifest bytes"),
		UGameplayStatics::SaveGameToMemory(SourceSave, Bytes));
	TestTrue(TEXT("serialized SaveGame payload is non-empty"), !Bytes.IsEmpty());

	UGameSave* RestoredSave = Cast<UGameSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("UE SaveGame archive restores UGameSave"), RestoredSave))
	{
		return false;
	}
	TestTrue(TEXT("manifest presence survives archive"),
		RestoredSave->bHasCivilizationManifest);
	FString ValidationReason;
	TestTrue(TEXT("restored manifest remains structurally valid"),
		RestoredSave->CivilizationManifest.IsStructurallyValid(&ValidationReason));
	TestEqual(TEXT("manifest schema survives archive"),
		RestoredSave->CivilizationManifest.SchemaVersion,
		FAPSCivilizationRuntimeManifest::LatestSchemaVersion);
	TestEqual(TEXT("manifest stable id survives archive"),
		RestoredSave->CivilizationManifest.ManifestId, Manifest.ManifestId);
	TestEqual(TEXT("civilization stable id survives archive"),
		RestoredSave->CivilizationManifest.CivilizationId, Manifest.CivilizationId);
	TestEqual(TEXT("materialization provenance survives archive"),
		RestoredSave->CivilizationManifest.MaterializationState,
		EAPSCivilizationMaterializationState::Materialized);
	for (int32 Index = 0; Index < Manifest.Entities.Num(); ++Index)
	{
		const FAPSCivilizationManifestEntity& RestoredEntity =
			RestoredSave->CivilizationManifest.Entities[Index];
		TestEqual(FString::Printf(TEXT("entity %d stable id survives"), Index),
			RestoredEntity.StableId, Manifest.Entities[Index].StableId);
		TestTrue(FString::Printf(TEXT("entity %d persisted transform survives"), Index),
			RestoredEntity.PlanetRelativeTransform.Equals(
				Manifest.Entities[Index].PlanetRelativeTransform, 0.01));
	}
	TestEqual(TEXT("actor save record count survives archive"),
		RestoredSave->ActorSaveDataArray.Num(), 1);
	if (RestoredSave->ActorSaveDataArray.Num() == 1)
	{
		TestEqual(TEXT("actor StableEntityId survives archive"),
			RestoredSave->ActorSaveDataArray[0].StableEntityId,
			Manifest.Entities[2].StableId);
	}
	return true;
}

#endif
