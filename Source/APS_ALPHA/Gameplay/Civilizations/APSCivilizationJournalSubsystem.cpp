#include "APSCivilizationJournalSubsystem.h"

#include "APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "APSCivilizationJournal"

bool UAPSCivilizationJournalSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAPSCivilizationJournalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UAPSCivilizationMaterializationSubsystem* Colony = InWorld.GetSubsystem<UAPSCivilizationMaterializationSubsystem>())
	{
		MaterializationHandle = Colony->OnMaterializationStateChanged().AddUObject(
			this, &UAPSCivilizationJournalSubsystem::HandleMaterializationState);
	}
	const UGameInstance* GameInstance = InWorld.GetGameInstance();
	const UMainGameplayInstance* GameplayState = GameInstance
		? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (!GameplayState || !GameplayState->bSpawnGeneratedCivilization)
	{
		return;
	}
	const UCivilization* Civilization = GameplayState->CurrentCivilization.Get();
	const USpawnParameters* Spawn = GameplayState->SpawnParameters;
	const UEnum* PlaceEnum = StaticEnum<ECharSpawnPlace>();
	const FString Name = Civilization ? Civilization->Name
		: (Spawn ? Spawn->CivilizationName : FString(TEXT("The civilization")));
	AddEntry(TEXT("Start"), FText::Format(LOCTEXT("SessionStart", "{0}: {1} begins at {2}."),
		GameplayState->bIsLoadingMode ? LOCTEXT("Loaded", "Save loaded") : LOCTEXT("NewWorld", "New world"),
		FText::FromString(Name),
		Spawn && PlaceEnum ? PlaceEnum->GetDisplayNameTextByValue(static_cast<int64>(Spawn->CharacterSpawnPlace))
			: LOCTEXT("UnknownPlace", "an unknown place")));
}

void UAPSCivilizationJournalSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UAPSCivilizationMaterializationSubsystem* Colony = World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>())
		{
			Colony->OnMaterializationStateChanged().Remove(MaterializationHandle);
		}
	}
	MaterializationHandle.Reset();
	Entries.Reset();
	Super::Deinitialize();
}

void UAPSCivilizationJournalSubsystem::AddEntry(const FName Category, const FText& Text)
{
	FAPSCivilizationJournalEntry Entry;
	Entry.WorldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Entry.RealTime = FDateTime::Now();
	Entry.Category = Category;
	Entry.Text = Text;
	Entries.Add(MoveTemp(Entry));
	if (Entries.Num() > MaxEntries)
	{
		Entries.RemoveAt(0, Entries.Num() - MaxEntries);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Journal] %s: %s"), *Category.ToString(), *Text.ToString());
	EntryAdded.Broadcast(Entries.Last());
}

void UAPSCivilizationJournalSubsystem::Post(const UObject* WorldContext, const FName Category, const FText& Text)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (UAPSCivilizationJournalSubsystem* Journal = World && World->IsGameWorld()
		? World->GetSubsystem<UAPSCivilizationJournalSubsystem>() : nullptr)
	{
		Journal->AddEntry(Category, Text);
	}
}

void UAPSCivilizationJournalSubsystem::HandleMaterializationState(const FAPSCivilizationRuntimeManifest& Manifest,
	EAPSCivilizationMaterializationState Previous, EAPSCivilizationMaterializationState Current)
{
	switch (Current)
	{
	case EAPSCivilizationMaterializationState::Materialized:
		AddEntry(TEXT("Colony"), LOCTEXT("ColonyFounded", "Colony founded on the home planet: a base module and a landing pad."));
		break;
	case EAPSCivilizationMaterializationState::LoadedFromSave:
		AddEntry(TEXT("Colony"), LOCTEXT("ColonyRestored", "Colony restored from the save."));
		break;
	case EAPSCivilizationMaterializationState::Blocked:
		AddEntry(TEXT("Colony"), LOCTEXT("ColonyBlocked", "No safe site for the colony was found on the home planet."));
		break;
	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
