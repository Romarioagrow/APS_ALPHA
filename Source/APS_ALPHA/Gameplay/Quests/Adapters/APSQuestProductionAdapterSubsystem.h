#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionEvent.h"
#include "APSQuestProductionAdapterSubsystem.generated.h"

class UAPSProductionEventSubsystem;

/**
 * Tickless Production-to-Quest boundary. It consumes only events published after
 * subscription and never polls, replays, or calls a gameplay executor.
 */
UCLASS()
class APS_ALPHA_API UAPSQuestProductionAdapterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void Subscribe(UWorld& InWorld);
	void HandleProductionEventPublished(const FAPSProductionEvent& Source);
	void Unsubscribe();

	TWeakObjectPtr<UAPSProductionEventSubsystem> ProductionSubsystem;
	FDelegateHandle EventPublishedHandle;
};
