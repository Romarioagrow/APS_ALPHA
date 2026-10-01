#pragma once

#include "CoreMinimal.h"
#include "APSCivilizationRuntimeManifest.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSCivilizationJournalSubsystem.generated.h"

/** One line of the civilization journal (U5). */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSCivilizationJournalEntry
{
	GENERATED_BODY()

	/** Game time of the event, seconds since the world began. */
	UPROPERTY(BlueprintReadOnly, Category = "Civilization|Journal")
	double WorldSeconds{0.0};

	UPROPERTY(BlueprintReadOnly, Category = "Civilization|Journal")
	FDateTime RealTime;

	/** Colony, Start, Flight, Production, Quest... */
	UPROPERTY(BlueprintReadOnly, Category = "Civilization|Journal")
	FName Category;

	UPROPERTY(BlueprintReadOnly, Category = "Civilization|Journal")
	FText Text;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAPSCivilizationJournalEntryAdded, const FAPSCivilizationJournalEntry&);

/**
 * Civilization journal (Rio, 29.09: a log of what the civilization did). Collects the events of this session from the
 * systems that already publish them (colony materialization, start, flight) and keeps them in order for the colony
 * terminal. Nothing is simulated here; saving the journal is a later step.
 */
UCLASS()
class APS_ALPHA_API UAPSCivilizationJournalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Records one event; past MaxEntries the oldest are dropped. */
	void AddEntry(FName Category, const FText& Text);

	/** For code without the subsystem at hand (flight model, start sequence); does nothing outside a game world. */
	static void Post(const UObject* WorldContext, FName Category, const FText& Text);

	const TArray<FAPSCivilizationJournalEntry>& GetEntries() const { return Entries; }
	/** Loads (APSCivilizationSave): the saved entries go before this session's, the oldest dropped past MaxEntries. */
	void RestoreEntries(TArray<FAPSCivilizationJournalEntry>&& Saved);
	FOnAPSCivilizationJournalEntryAdded& OnEntryAdded() { return EntryAdded; }

	static constexpr int32 MaxEntries = 500;

private:
	void HandleMaterializationState(const FAPSCivilizationRuntimeManifest& Manifest,
		EAPSCivilizationMaterializationState Previous, EAPSCivilizationMaterializationState Current);

	TArray<FAPSCivilizationJournalEntry> Entries;
	FOnAPSCivilizationJournalEntryAdded EntryAdded;
	FDelegateHandle MaterializationHandle;
};
