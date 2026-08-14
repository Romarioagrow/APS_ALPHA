#include "APSQuestCivilizationAdapterSubsystem.h"

#include "APSCivilizationQuestAdapter.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Quests/APSEarlyAccessOnboardingDefinition.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestSubsystem.h"
#include "Engine/GameInstance.h"

void UAPSQuestCivilizationAdapterSubsystem::Deinitialize()
{
	Unsubscribe();
	Super::Deinitialize();
}

void UAPSQuestCivilizationAdapterSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Subscribe(InWorld);
}

bool UAPSQuestCivilizationAdapterSubsystem::DoesSupportWorldType(
	const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAPSQuestCivilizationAdapterSubsystem::Subscribe(UWorld& InWorld)
{
	Unsubscribe();
	UAPSCivilizationMaterializationSubsystem* Subsystem =
		InWorld.GetSubsystem<UAPSCivilizationMaterializationSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Quest.CivilizationAdapter] materialization subsystem unavailable"));
		return;
	}

	CivilizationSubsystem = Subsystem;
	MaterializationStateChangedHandle = Subsystem->OnMaterializationStateChanged().AddUObject(
		this, &UAPSQuestCivilizationAdapterSubsystem::HandleMaterializationStateChanged);
	// Required late-subscriber path: read immediately after binding, never poll.
	TryConsumeReadyManifest(Subsystem->GetRuntimeManifest());
}

void UAPSQuestCivilizationAdapterSubsystem::HandleMaterializationStateChanged(
	const FAPSCivilizationRuntimeManifest& Manifest,
	const EAPSCivilizationMaterializationState Previous,
	const EAPSCivilizationMaterializationState Current)
{
	(void)Previous;
	if (Current == EAPSCivilizationMaterializationState::Materialized
		|| Current == EAPSCivilizationMaterializationState::LoadedFromSave)
	{
		TryConsumeReadyManifest(Manifest);
	}
}

bool UAPSQuestCivilizationAdapterSubsystem::TryConsumeReadyManifest(
	const FAPSCivilizationRuntimeManifest& Manifest)
{
	UAPSCivilizationMaterializationSubsystem* Civilization = CivilizationSubsystem.Get();
	if (!Civilization || !Civilization->IsMaterializationComplete())
	{
		return false;
	}
	const bool bReady = Manifest.MaterializationState
		== EAPSCivilizationMaterializationState::Materialized
		|| Manifest.MaterializationState == EAPSCivilizationMaterializationState::LoadedFromSave;
	if (!bReady)
	{
		return false;
	}

	FAPSCivilizationQuestReadySnapshot Snapshot;
	FString Reason;
	if (!FAPSCivilizationQuestAdapter::NormalizeReadyManifest(Manifest, Snapshot, Reason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Quest.CivilizationAdapter] manifest rejected: %s"), *Reason);
		return false;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UAPSQuestSubsystem* Quest = GameInstance
		? GameInstance->GetSubsystem<UAPSQuestSubsystem>() : nullptr;
	if (!Quest)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Quest.CivilizationAdapter] Quest subsystem unavailable"));
		return false;
	}

	auto Reject = [&Reason](const TCHAR* Stage)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Quest.CivilizationAdapter] %s rejected: %s"), Stage, *Reason);
		return false;
	};
	using Contract = FAPSEarlyAccessOnboardingContract;
	if (!Quest->StartQuest(Contract::QuestId, Snapshot.ManifestId, Reason))
	{
		return Reject(TEXT("start"));
	}
	if (!Quest->BindQuestEntity(Contract::QuestId, Contract::BaseBinding,
		Snapshot.Base, Reason)
		|| !Quest->BindQuestEntity(Contract::QuestId, Contract::LandingPadBinding,
			Snapshot.LandingPad, Reason)
		|| !Quest->BindQuestEntity(Contract::QuestId, Contract::SelectedShipBinding,
			Snapshot.SelectedShip, Reason)
		|| !Quest->BindQuestEntity(Contract::QuestId, Contract::HomeSystemBinding,
			Snapshot.HomeSystem, Reason))
	{
		return Reject(TEXT("binding"));
	}
	if (!Quest->BeginQuestEventStream(Contract::QuestId, Snapshot.ManifestId, Reason))
	{
		return Reject(TEXT("stream"));
	}
	if (!Quest->SubmitQuestEvent(Snapshot.ReadyEvent, Reason))
	{
		return Reject(TEXT("ready fact"));
	}

	UE_LOG(LogTemp, Log,
		TEXT("[APS.Quest.CivilizationAdapter] actor-ready fact accepted manifest=%s provenance=%d"),
		*Snapshot.ManifestId.ToString(EGuidFormats::DigitsWithHyphensLower),
		static_cast<int32>(Snapshot.Provenance));
	return true;
}

void UAPSQuestCivilizationAdapterSubsystem::Unsubscribe()
{
	if (UAPSCivilizationMaterializationSubsystem* Subsystem = CivilizationSubsystem.Get())
	{
		if (MaterializationStateChangedHandle.IsValid())
		{
			Subsystem->OnMaterializationStateChanged().Remove(
				MaterializationStateChangedHandle);
		}
	}
	MaterializationStateChangedHandle.Reset();
	CivilizationSubsystem.Reset();
}
