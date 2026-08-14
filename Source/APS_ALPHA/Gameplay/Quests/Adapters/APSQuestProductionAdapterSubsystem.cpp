#include "APSQuestProductionAdapterSubsystem.h"

#include "APSProductionQuestAdapter.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionEventSubsystem.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestSubsystem.h"
#include "Engine/GameInstance.h"

void UAPSQuestProductionAdapterSubsystem::Deinitialize()
{
	Unsubscribe();
	Super::Deinitialize();
}

void UAPSQuestProductionAdapterSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Subscribe(InWorld);
}

bool UAPSQuestProductionAdapterSubsystem::DoesSupportWorldType(
	const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAPSQuestProductionAdapterSubsystem::Subscribe(UWorld& InWorld)
{
	Unsubscribe();
	UAPSProductionEventSubsystem* Subsystem =
		InWorld.GetSubsystem<UAPSProductionEventSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Quest.ProductionAdapter] Production subsystem unavailable"));
		return;
	}

	ProductionSubsystem = Subsystem;
	EventPublishedHandle = Subsystem->OnEventPublished().AddUObject(
		this, &UAPSQuestProductionAdapterSubsystem::HandleProductionEventPublished);
}

void UAPSQuestProductionAdapterSubsystem::HandleProductionEventPublished(
	const FAPSProductionEvent& Source)
{
	FAPSQuestEvent Event;
	FString Reason;
	if (!FAPSProductionQuestAdapter::Normalize(Source, Event, Reason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Quest.ProductionAdapter] event rejected: %s"), *Reason);
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UAPSQuestSubsystem* Quest = GameInstance
		? GameInstance->GetSubsystem<UAPSQuestSubsystem>() : nullptr;
	if (!Quest)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Quest.ProductionAdapter] Quest subsystem unavailable"));
		return;
	}

	if (!Quest->SubmitQuestEvent(Event, Reason))
	{
		// Production is global; an event outside a running Quest is an expected no-op.
		UE_LOG(LogTemp, VeryVerbose,
			TEXT("[APS.Quest.ProductionAdapter] event not consumed: %s"), *Reason);
	}
}

void UAPSQuestProductionAdapterSubsystem::Unsubscribe()
{
	if (UAPSProductionEventSubsystem* Subsystem = ProductionSubsystem.Get())
	{
		if (EventPublishedHandle.IsValid())
		{
			Subsystem->OnEventPublished().Remove(EventPublishedHandle);
		}
	}
	EventPublishedHandle.Reset();
	ProductionSubsystem.Reset();
}
