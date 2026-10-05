#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/SavedActorData.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "GameFramework/Pawn.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGeneratedWorldSaveSnapshotContractTest,
	"APS.World.Save.GeneratedModelAndPlayerRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGeneratedWorldSaveSnapshotContractTest::RunTest(const FString& Parameters)
{
	UGeneratedWorld* SourceModel = NewObject<UGeneratedWorld>();
	SourceModel->GenerationSeed = 424242;
	SourceModel->PlanetarySystemType = EPlanetarySystemType::GasGiantsSystem;
	SourceModel->PlanetsAmount = 7;
	SourceModel->SetPreviewDisplayNameOverride(TEXT("SYS0/S0/P2"), TEXT("Aurelia"));
	SourceModel->CanonicalStellarDataset.BuildSerial = 19;
	SourceModel->CanonicalStellarDataset.WorldGenerationSeed = 424242;
	FAPSCanonicalClusterSystemRecord& Record =
		SourceModel->CanonicalStellarDataset.ClusterRecords.AddDefaulted_GetRef();
	Record.StableId = FGuid(0x12345678, 0x90abcdef, 0x10203040, 0x50607080);
	Record.CanonicalIndex = 0;

	UGameSave* SourceSave = NewObject<UGameSave>();
	SourceSave->SaveFormatVersion = APSWorldSaveSnapshot::LatestSaveFormatVersion;
	SourceSave->SaveSlotName = TEXT("APS_WORLD_SNAPSHOT_MEMORY_ONLY");
	SourceSave->GeneratedWorldsDataArray.Add(SourceModel->SaveWorldData());
	TestTrue(TEXT("generated world snapshot captures"),
		APSWorldSaveSnapshot::Capture(SourceModel, SourceSave->GeneratedWorldModelData));
	USpawnParameters* SourceSpawn = NewObject<USpawnParameters>();
	SourceSpawn->CivilizationName = TEXT("EXACT REPLAY CIVILIZATION");
	SourceSpawn->StartingFleetSize = 7;
	SourceSpawn->CharacterSpawnPlace = ECharSpawnPlace::PlanetSurface;
	SourceSpawn->BP_CharacterClass = APawn::StaticClass();
	SourceSpawn->BP_HomeSpaceship = ASpaceship::StaticClass();
	SourceSpawn->StarterComplexTurnDegrees = -63.25;
	SourceSave->bHadGeneratedCivilization = true;
	TestTrue(TEXT("civilization spawn recipe captures"),
		APSWorldSaveSnapshot::CaptureSpawnParameters(
			SourceSpawn, SourceSave->SpawnParametersData));
	SourceSave->bHasPlayerPawnState = true;
	SourceSave->PlayerPawnClass = TEXT("/Script/Engine.DefaultPawn");
	SourceSave->PlayerPawnTransform = FTransform(
		FRotator(5.0, 75.0, 0.0), FVector(1200.0, -3400.0, 5600.0));
	SourceSave->PlayerControlRotation = FRotator(-12.0, 80.0, 0.0);

	TArray<uint8> Bytes;
	TestTrue(TEXT("save archive writes snapshot"),
		UGameplayStatics::SaveGameToMemory(SourceSave, Bytes));
	UGameSave* RestoredSave = Cast<UGameSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("save archive restores snapshot"), RestoredSave))
	{
		return false;
	}

	UGeneratedWorld* RestoredModel = APSWorldSaveSnapshot::Restore(
		RestoredSave, GetTransientPackage(), RestoredSave->SaveSlotName);
	if (!TestNotNull(TEXT("generated world model restores"), RestoredModel))
	{
		return false;
	}
	TestEqual(TEXT("generation seed survives"), RestoredModel->GenerationSeed, 424242);
	TestEqual(TEXT("system type survives"), RestoredModel->PlanetarySystemType,
		EPlanetarySystemType::GasGiantsSystem);
	TestEqual(TEXT("planet count survives"), RestoredModel->PlanetsAmount, 7);
	TestEqual(TEXT("canonical record survives"),
		RestoredModel->CanonicalStellarDataset.ClusterRecords.Num(), 1);
	const FString* RestoredName =
		RestoredModel->FindPreviewDisplayNameOverride(TEXT("SYS0/S0/P2"));
	TestTrue(TEXT("authored body name survives"),
		RestoredName && *RestoredName == TEXT("Aurelia"));
	TestTrue(TEXT("player transform survives save archive"),
		RestoredSave->PlayerPawnTransform.Equals(SourceSave->PlayerPawnTransform, 0.01));
	TestTrue(TEXT("control rotation survives save archive"),
		RestoredSave->PlayerControlRotation.Equals(SourceSave->PlayerControlRotation, 0.01));
	TestTrue(TEXT("civilization route survives save archive"),
		RestoredSave->bHadGeneratedCivilization);
	USpawnParameters* RestoredSpawn = APSWorldSaveSnapshot::RestoreSpawnParameters(
		RestoredSave, GetTransientPackage());
	if (!TestNotNull(TEXT("civilization spawn recipe restores"), RestoredSpawn))
	{
		return false;
	}
	TestEqual(TEXT("civilization name survives"), RestoredSpawn->CivilizationName,
		SourceSpawn->CivilizationName);
	TestEqual(TEXT("fleet size survives"), RestoredSpawn->StartingFleetSize, 7);
	TestEqual(TEXT("surface start survives"), RestoredSpawn->CharacterSpawnPlace,
		ECharSpawnPlace::PlanetSurface);
	TestEqual(TEXT("selected ship class survives"), RestoredSpawn->BP_HomeSpaceship.Get(),
		ASpaceship::StaticClass());
	// The home complex's turn sets the frame the saved positions use: the replay must stand it exactly where it stood.
	TestEqual(TEXT("home complex turn survives"), RestoredSpawn->StarterComplexTurnDegrees, -63.25);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSRealScaleWorldSaveContractTest,
	"APS.World.Save.RealScaleFlagRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSRealScaleWorldSaveContractTest::RunTest(const FString& Parameters)
{
	// Rio 05.10 (real scale experiment): REAL SCALE and its sealed layout survive the snapshot and the commit duplicate;
	// a new or older model stays OFF with no sealed layout (zero keeps every legacy dataset hash).
	TestFalse(TEXT("a new world starts with REAL SCALE off"), GetDefault<UGeneratedWorld>()->bRealScale);
	const FAPSCanonicalStellarDataset LegacyDataset;
	TestEqual(TEXT("a legacy dataset has no sealed real-scale unit"), LegacyDataset.RealScaleCmPerUnit, 0.0);
	TestEqual(TEXT("a legacy dataset has no sealed cluster share"), LegacyDataset.RealScaleClusterToGalaxy, 0.0);

	UGeneratedWorld* SourceModel = NewObject<UGeneratedWorld>();
	SourceModel->GenerationSeed = 515253;
	SourceModel->bRealScale = true;
	SourceModel->CanonicalStellarDataset.RealScaleCmPerUnit = 3.3e14;
	SourceModel->CanonicalStellarDataset.RealScaleClusterToGalaxy = 0.0625;

	UGameSave* SourceSave = NewObject<UGameSave>();
	SourceSave->SaveFormatVersion = APSWorldSaveSnapshot::LatestSaveFormatVersion;
	SourceSave->SaveSlotName = TEXT("APS_REAL_SCALE_MEMORY_ONLY");
	SourceSave->GeneratedWorldsDataArray.Add(SourceModel->SaveWorldData());
	TestTrue(TEXT("real-scale world snapshot captures"),
		APSWorldSaveSnapshot::Capture(SourceModel, SourceSave->GeneratedWorldModelData));
	TArray<uint8> Bytes;
	TestTrue(TEXT("save archive writes the real-scale snapshot"),
		UGameplayStatics::SaveGameToMemory(SourceSave, Bytes));
	UGameSave* RestoredSave = Cast<UGameSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("save archive restores the real-scale snapshot"), RestoredSave))
	{
		return false;
	}
	UGeneratedWorld* RestoredModel = APSWorldSaveSnapshot::Restore(
		RestoredSave, GetTransientPackage(), RestoredSave->SaveSlotName);
	if (!TestNotNull(TEXT("real-scale model restores"), RestoredModel))
	{
		return false;
	}
	TestTrue(TEXT("REAL SCALE survives the snapshot"), RestoredModel->bRealScale);
	TestEqual(TEXT("sealed real-scale unit survives"),
		RestoredModel->CanonicalStellarDataset.RealScaleCmPerUnit, 3.3e14);
	TestEqual(TEXT("sealed cluster share survives"),
		RestoredModel->CanonicalStellarDataset.RealScaleClusterToGalaxy, 0.0625);

	// The menu commits a duplicate of its model into the GameInstance (UWorldGenerationViewModel::CommitAndOpenLevel).
	const UGeneratedWorld* Committed = DuplicateObject<UGeneratedWorld>(SourceModel, GetTransientPackage());
	TestTrue(TEXT("REAL SCALE survives the commit duplicate"), Committed && Committed->bRealScale);

	// A save without the model snapshot (only the legacy summary, which never carried the flag) loads OFF.
	UGameSave* LegacySave = NewObject<UGameSave>();
	LegacySave->SaveSlotName = TEXT("APS_REAL_SCALE_LEGACY_MEMORY_ONLY");
	LegacySave->GeneratedWorldsDataArray.Add(SourceModel->SaveWorldData());
	UGeneratedWorld* LegacyModel = APSWorldSaveSnapshot::Restore(
		LegacySave, GetTransientPackage(), LegacySave->SaveSlotName);
	if (!TestNotNull(TEXT("legacy summary restores"), LegacyModel))
	{
		return false;
	}
	TestFalse(TEXT("a model without the flag loads with REAL SCALE off"), LegacyModel->bRealScale);
	TestEqual(TEXT("a model without the flag has no sealed layout"),
		LegacyModel->CanonicalStellarDataset.RealScaleCmPerUnit, 0.0);
	return true;
}

#endif
