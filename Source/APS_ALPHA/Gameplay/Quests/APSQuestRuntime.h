#pragma once

#include "CoreMinimal.h"
#include "APSQuestDefinition.h"
#include "APSQuestRuntime.generated.h"

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestEventStreamCursor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FGuid StreamId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	int64 LastConsumedSequence{0};
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestNamedEntityBinding
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName BindingName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FAPSQuestEntityRef Entity;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestNodeRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	EAPSQuestNodeState State{EAPSQuestNodeState::Dormant};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	int32 Progress{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName LastFailureCode;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestRewardLedgerEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FGuid TransactionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName RewardId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	EAPSQuestRewardState State{EAPSQuestRewardState::Requested};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName FailureCode;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestInstanceSaveData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName QuestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	int32 DefinitionVersion{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FGuid InstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	EAPSQuestInstanceState State{EAPSQuestInstanceState::Inactive};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FGuid EventStreamId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	int64 LastConsumedSequence{0};

	/** Independent monotonic cursors for owner event streams (Production, Surface, etc.). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	TArray<FAPSQuestEventStreamCursor> EventStreams;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	TArray<FAPSQuestNodeRuntimeState> Nodes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	TArray<FAPSQuestNamedEntityBinding> Bindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	TArray<FGuid> ConsumedEventIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	TArray<FGuid> ConsumedTerminalCorrelations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	TArray<FAPSQuestRewardLedgerEntry> RewardLedger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	FName LastFailureCode;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestSaveData
{
	GENERATED_BODY()

	static constexpr int32 LatestSchemaVersion = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	int32 SchemaVersion{LatestSchemaVersion};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Quest")
	TArray<FAPSQuestInstanceSaveData> Instances;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FAPSQuestInstanceChangedNative,
	FName, const FAPSQuestInstanceSaveData&);
DECLARE_MULTICAST_DELEGATE_OneParam(FAPSQuestPromptPublishedNative,
	const FAPSQuestPromptSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FAPSQuestRewardRequestedNative,
	const FAPSQuestRewardCommand&);

/**
 * Deterministic, world-agnostic Quest runtime. Owner adapters normalize external
 * facts before calling SubmitEvent; this type never scans or mutates world actors.
 */
class APS_ALPHA_API FAPSQuestRuntime
{
public:
	static constexpr int32 MaximumDedupeEntries = 256;

	bool RegisterDefinition(const UAPSQuestDefinition* Definition, FString& OutReason);
	bool StartQuest(FName QuestId, const FGuid& InstanceId, FString& OutReason);
	bool BindEntity(FName QuestId, FName BindingName, const FAPSQuestEntityRef& Entity,
		FString& OutReason);
	bool BeginEventStream(FName QuestId, const FGuid& StreamId, FString& OutReason);
	bool SubmitEvent(const FAPSQuestEvent& Event, FString& OutReason);
	bool AcknowledgeReward(FName QuestId, const FGuid& TransactionId, bool bApplied,
		FName FailureCode, FString& OutReason);
	bool SuspendQuest(FName QuestId, FName FailureCode, FString& OutReason);
	bool RecoverQuest(FName QuestId, FName NodeId, EAPSQuestRecoveryPolicy Policy,
		FString& OutReason);

	FAPSQuestSaveData ExportSaveData() const;
	bool RestoreSaveData(const FAPSQuestSaveData& SaveData, FString& OutReason);

	const FAPSQuestInstanceSaveData* FindInstance(FName QuestId) const;
	FString DumpQuest(FName QuestId) const;

	FAPSQuestInstanceChangedNative& OnInstanceChanged() { return InstanceChanged; }
	FAPSQuestPromptPublishedNative& OnPromptPublished() { return PromptPublished; }
	FAPSQuestRewardRequestedNative& OnRewardRequested() { return RewardRequested; }

private:
	const UAPSQuestDefinition* FindDefinition(FName QuestId) const;
	FAPSQuestInstanceSaveData* FindMutableInstance(FName QuestId);
	FAPSQuestNodeRuntimeState* FindMutableNode(FAPSQuestInstanceSaveData& Instance,
		FName NodeId) const;
	FAPSQuestEventStreamCursor* FindMutableStreamCursor(
		FAPSQuestInstanceSaveData& Instance, const FGuid& StreamId) const;
	const FAPSQuestNamedEntityBinding* FindBinding(
		const FAPSQuestInstanceSaveData& Instance, FName BindingName) const;
	bool MatchesEntity(const FAPSQuestInstanceSaveData& Instance,
		EAPSQuestBindingMatch Match, const FAPSQuestEntityRef& Exact, FName Binding,
		const FAPSQuestEntityRef& Actual, FString& OutReason) const;
	bool MatchesPredicate(const FAPSQuestInstanceSaveData& Instance,
		const FAPSQuestEventPredicate& Predicate, const FAPSQuestEvent& Event,
		bool bRequireResult, FString& OutReason) const;
	void PublishPrompt(const FAPSQuestInstanceSaveData& Instance,
		const FAPSQuestObjectiveNodeDefinition& Node);
	void RequestRewards(FAPSQuestInstanceSaveData& Instance,
		const FAPSQuestObjectiveNodeDefinition& Node);
	void CompleteNode(FAPSQuestInstanceSaveData& Instance,
		const UAPSQuestDefinition& Definition, FAPSQuestNodeRuntimeState& NodeState);
	void RefreshInstanceCompletion(FAPSQuestInstanceSaveData& Instance,
		const UAPSQuestDefinition& Definition);
	static FGuid MakeRewardTransactionId(const FAPSQuestInstanceSaveData& Instance,
		FName NodeId, FName RewardId);
	static void AppendBoundedGuid(TArray<FGuid>& Values, const FGuid& Value);

	TMap<FName, const UAPSQuestDefinition*> Definitions;
	TMap<FName, FAPSQuestInstanceSaveData> Instances;
	FAPSQuestInstanceChangedNative InstanceChanged;
	FAPSQuestPromptPublishedNative PromptPublished;
	FAPSQuestRewardRequestedNative RewardRequested;
};
