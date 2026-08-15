#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "APSQuestRuntime.h"
#include "APSQuestSubsystem.generated.h"

/** Game-instance lifetime facade for the isolated deterministic Quest runtime. */
UCLASS()
class APS_ALPHA_API UAPSQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool RegisterQuestDefinition(UAPSQuestDefinition* Definition, FString& OutReason);

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool StartQuest(FName QuestId, FGuid InstanceId, FString& OutReason);

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool BindQuestEntity(FName QuestId, FName BindingName, const FAPSQuestEntityRef& Entity,
		FString& OutReason);

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool BeginQuestEventStream(FName QuestId, FGuid StreamId, FString& OutReason);

	/** C++-only authoritative ingress for owner adapters. */
	bool SubmitQuestEvent(const FAPSQuestEvent& Event, FString& OutReason);

	/** C++-only development ingress; runtime rejects it in Shipping and suppresses rewards. */
	bool DebugInjectQuestEvent(const FAPSQuestEvent& Event, FString& OutReason);

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool AcknowledgeQuestReward(FName QuestId, FGuid TransactionId, bool bApplied,
		FName FailureCode, FString& OutReason);

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool SuspendQuest(FName QuestId, FName FailureCode, FString& OutReason);

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool RecoverQuest(FName QuestId, FName NodeId, EAPSQuestRecoveryPolicy Policy,
		FString& OutReason);

	UFUNCTION(BlueprintPure, Category="Quest")
	FAPSQuestSaveData ExportQuestSaveData() const;

	UFUNCTION(BlueprintCallable, Category="Quest")
	bool RestoreQuestSaveData(const FAPSQuestSaveData& SaveData, FString& OutReason);

	UFUNCTION(BlueprintPure, Category="Quest|Debug")
	FString DumpQuest(FName QuestId) const;

	/** UI subscribes first, then copies this value to close the bootstrap race. */
	UFUNCTION(BlueprintPure, Category="Quest|Prompt")
	bool TryGetCurrentPromptSnapshot(FAPSQuestPromptSnapshot& OutSnapshot) const;

	void BeginPromptPresentationSession(const FGuid& SessionEpoch);
	void EndPromptPresentationSession(const FGuid& SessionEpoch);

	FAPSQuestInstanceChangedNative& OnQuestInstanceChanged();
	FAPSQuestPromptPublishedNative& OnQuestPromptPublished();
	FAPSQuestRewardRequestedNative& OnQuestRewardRequested();

private:
	FAPSQuestRuntime* GetRuntime();
	const FAPSQuestRuntime* GetRuntime() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAPSQuestDefinition>> RegisteredDefinitions;

	TUniquePtr<FAPSQuestRuntime> Runtime;
};
