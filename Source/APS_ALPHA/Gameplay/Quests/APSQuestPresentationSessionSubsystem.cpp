#include "APSQuestPresentationSessionSubsystem.h"

#include "APSQuestSubsystem.h"
#include "Engine/GameInstance.h"

void UAPSQuestPresentationSessionSubsystem::Deinitialize()
{
	if (SessionEpoch.IsValid())
	{
		UWorld* World = GetWorld();
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (UAPSQuestSubsystem* Quest = GameInstance
			? GameInstance->GetSubsystem<UAPSQuestSubsystem>() : nullptr)
		{
			Quest->EndPromptPresentationSession(SessionEpoch);
		}
	}
	SessionEpoch.Invalidate();
	Super::Deinitialize();
}

void UAPSQuestPresentationSessionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UGameInstance* GameInstance = InWorld.GetGameInstance();
	if (UAPSQuestSubsystem* Quest = GameInstance
		? GameInstance->GetSubsystem<UAPSQuestSubsystem>() : nullptr)
	{
		SessionEpoch = FGuid::NewGuid();
		Quest->BeginPromptPresentationSession(SessionEpoch);
	}
}

bool UAPSQuestPresentationSessionSubsystem::DoesSupportWorldType(
	const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
