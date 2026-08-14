#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "APS_ALPHA/Gameplay/Interaction/APSInteractable.h"
#include "APSInteractionContractTestActor.generated.h"

class USceneComponent;

/** Minimal native fixture for the owner-published Interaction event contract. */
UCLASS(Transient, NotBlueprintable)
class APS_ALPHA_API AAPSInteractionContractTestActor : public AActor,
	public IAPSInteractable
{
	GENERATED_BODY()

public:
	AAPSInteractionContractTestActor();

	virtual FGuid GetInteractionTargetStableId_Implementation() const override;
	virtual bool QueryInteraction_Implementation(const FAPSInteractionContext& Context,
		FAPSInteractionPromptDescriptor& OutPrompt) const override;
	virtual FAPSInteractionExecutionResult ExecuteInteraction_Implementation(
		const FAPSInteractionExecutionRequest& Request) override;

	FGuid TargetStableId;
	EAPSTargetIdentityDomain TargetIdentityDomain{EAPSTargetIdentityDomain::CivilizationEntity};
	FGuid PromptId;
	int64 Revision{1};
	FName ActionId{TEXT("APS.Production.Open")};
	FName Verb{TEXT("APS.Interaction.Open")};
	EAPSInteractionExecutionStatus ExecutionStatus{EAPSInteractionExecutionStatus::Succeeded};
	FName ResultCode{TEXT("APS.Production.PanelRequested")};
	FName FailureCode;
	int32 ExecutionCount{0};

private:
	/** Mirrors the scene-only root shape of AAPSProductionConsole. */
	UPROPERTY(VisibleAnywhere, Category="APS|Interaction Test")
	TObjectPtr<USceneComponent> InteractionRoot;
};
