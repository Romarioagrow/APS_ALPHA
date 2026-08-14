#include "APSQuestSubsystem.h"

#include "APSEarlyAccessOnboardingDefinition.h"

void UAPSQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Runtime = MakeUnique<FAPSQuestRuntime>();

	UAPSEarlyAccessOnboardingDefinition* EarlyAccessDefinition =
		NewObject<UAPSEarlyAccessOnboardingDefinition>(this);
	FString RegistrationFailure;
	if (!RegisterQuestDefinition(EarlyAccessDefinition, RegistrationFailure))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Quest] built-in Early Access route registration failed: %s"),
			*RegistrationFailure);
	}
}

void UAPSQuestSubsystem::Deinitialize()
{
	Runtime.Reset();
	RegisteredDefinitions.Reset();
	Super::Deinitialize();
}

bool UAPSQuestSubsystem::RegisterQuestDefinition(UAPSQuestDefinition* Definition,
	FString& OutReason)
{
	if (!GetRuntime())
	{
		OutReason = TEXT("Quest runtime is not initialized");
		return false;
	}
	if (!Runtime->RegisterDefinition(Definition, OutReason))
	{
		return false;
	}
	RegisteredDefinitions.AddUnique(Definition);
	return true;
}

bool UAPSQuestSubsystem::StartQuest(FName QuestId, FGuid InstanceId, FString& OutReason)
{
	return GetRuntime() && Runtime->StartQuest(QuestId, InstanceId, OutReason);
}

bool UAPSQuestSubsystem::BindQuestEntity(FName QuestId, FName BindingName,
	const FAPSQuestEntityRef& Entity, FString& OutReason)
{
	return GetRuntime() && Runtime->BindEntity(QuestId, BindingName, Entity, OutReason);
}

bool UAPSQuestSubsystem::BeginQuestEventStream(FName QuestId, FGuid StreamId,
	FString& OutReason)
{
	return GetRuntime() && Runtime->BeginEventStream(QuestId, StreamId, OutReason);
}

bool UAPSQuestSubsystem::SubmitQuestEvent(const FAPSQuestEvent& Event, FString& OutReason)
{
	return GetRuntime() && Runtime->SubmitEvent(Event, OutReason);
}

bool UAPSQuestSubsystem::AcknowledgeQuestReward(FName QuestId, FGuid TransactionId,
	bool bApplied, FName FailureCode, FString& OutReason)
{
	return GetRuntime() && Runtime->AcknowledgeReward(
		QuestId, TransactionId, bApplied, FailureCode, OutReason);
}

bool UAPSQuestSubsystem::SuspendQuest(FName QuestId, FName FailureCode,
	FString& OutReason)
{
	return GetRuntime() && Runtime->SuspendQuest(QuestId, FailureCode, OutReason);
}

bool UAPSQuestSubsystem::RecoverQuest(FName QuestId, FName NodeId,
	EAPSQuestRecoveryPolicy Policy, FString& OutReason)
{
	return GetRuntime() && Runtime->RecoverQuest(QuestId, NodeId, Policy, OutReason);
}

FAPSQuestSaveData UAPSQuestSubsystem::ExportQuestSaveData() const
{
	return GetRuntime() ? Runtime->ExportSaveData() : FAPSQuestSaveData{};
}

bool UAPSQuestSubsystem::RestoreQuestSaveData(const FAPSQuestSaveData& SaveData,
	FString& OutReason)
{
	return GetRuntime() && Runtime->RestoreSaveData(SaveData, OutReason);
}

FString UAPSQuestSubsystem::DumpQuest(FName QuestId) const
{
	return GetRuntime() ? Runtime->DumpQuest(QuestId) : TEXT("Quest runtime is not initialized");
}

FAPSQuestInstanceChangedNative& UAPSQuestSubsystem::OnQuestInstanceChanged()
{
	check(GetRuntime());
	return Runtime->OnInstanceChanged();
}

FAPSQuestPromptPublishedNative& UAPSQuestSubsystem::OnQuestPromptPublished()
{
	check(GetRuntime());
	return Runtime->OnPromptPublished();
}

FAPSQuestRewardRequestedNative& UAPSQuestSubsystem::OnQuestRewardRequested()
{
	check(GetRuntime());
	return Runtime->OnRewardRequested();
}

FAPSQuestRuntime* UAPSQuestSubsystem::GetRuntime()
{
	return Runtime.Get();
}

const FAPSQuestRuntime* UAPSQuestSubsystem::GetRuntime() const
{
	return Runtime.Get();
}
