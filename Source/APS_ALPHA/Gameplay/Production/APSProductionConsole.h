#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "APS_ALPHA/Gameplay/Interaction/APSInteractable.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APSProductionTypes.h"
#include "APSProductionConsole.generated.h"

class UAPSCivilizationMaterializationSubsystem;
class USceneComponent;
/**
 * Reusable actor-driven access point for HQ, crafting, build and shipyard panels.
 * It consumes canonical identity from Civilization actors and never creates world IDs.
 */
UCLASS(Blueprintable)
class APS_ALPHA_API AAPSProductionConsole : public AActor, public IAPSInteractable
{
	GENERATED_BODY()

public:
	AAPSProductionConsole();

	/** Canonical integration seam for non-Civilization actors. IDs are supplied, never hashed here. */
	UFUNCTION(BlueprintCallable, Category="APS|Production Console")
	bool ConfigureCanonicalIdentity(FGuid ContextStableId, FGuid OwnerStableId,
		FGuid SpawnPadStableId, FString& OutFailure);

	/** Idempotent actor-ready registration. Useful for Blueprint-created child consoles. */
	UFUNCTION(BlueprintCallable, Category="APS|Production Console")
	bool InitializeProductionContext(FString& OutFailure);

	virtual FGuid GetInteractionTargetStableId_Implementation() const override;
	virtual bool QueryInteraction_Implementation(const FAPSInteractionContext& Context,
		FAPSInteractionPromptDescriptor& OutPrompt) const override;
	virtual FAPSInteractionExecutionResult ExecuteInteraction_Implementation(
		const FAPSInteractionExecutionRequest& Request) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SubscribeToCivilizationLifecycle();
	void UnsubscribeFromCivilizationLifecycle();
	void HandleCivilizationMaterializationStateChanged(
		const FAPSCivilizationRuntimeManifest& Manifest,
		EAPSCivilizationMaterializationState Previous,
		EAPSCivilizationMaterializationState Current);
	bool ResolveCanonicalIdentity(FGuid& OutContextStableId, FGuid& OutOwnerStableId,
		FGuid& OutSpawnPadStableId, FString& OutFailure) const;
	const class UAPSCivilizationIdentityComponent* FindIdentity(const AActor* Actor) const;
	EAPSInteractionActionKind ResolveActionKind() const;

	UPROPERTY(VisibleAnywhere, Category="APS|Production Console")
	TObjectPtr<USceneComponent> InteractionRoot;

	UPROPERTY(EditAnywhere, Category="APS|Production Console")
	EAPSProductionDomain Domain{EAPSProductionDomain::Crafting};

	UPROPERTY(EditAnywhere, Category="APS|Production Console")
	EAPSInteractionContextKind PromptContextKind{EAPSInteractionContextKind::CraftingStation};

	UPROPERTY(EditAnywhere, Category="APS|Production Console")
	FAPSLocalizedTextDescriptor DisplayName;

	UPROPERTY(EditAnywhere, Category="APS|Production Console", meta=(ClampMin="1.0"))
	double InteractionRangeCm{600.0};

	UPROPERTY(EditAnywhere, Category="APS|Production Console", meta=(ClampMin="1"))
	int32 QueueCapacity{8};

	UPROPERTY(EditAnywhere, Category="APS|Production Console", meta=(ClampMin="1"))
	int32 MaximumConcurrentJobs{1};

	/** Optional actor whose Civilization identity represents this console context. */
	UPROPERTY(EditAnywhere, Category="APS|Production Console")
	TObjectPtr<AActor> ContextIdentityActor;

	/** Shipyard-only canonical pad actor. Its Civilization identity supplies SpawnPadStableId. */
	UPROPERTY(EditAnywhere, Category="APS|Production Console")
	TObjectPtr<AActor> SpawnPadIdentityActor;

	FGuid ConfiguredContextStableId;
	FGuid ConfiguredOwnerStableId;
	FGuid ConfiguredSpawnPadStableId;
	FGuid RegisteredContextStableId;
	FGuid PromptId;
	TWeakObjectPtr<UAPSCivilizationMaterializationSubsystem> CivilizationSubsystem;
	FDelegateHandle MaterializationStateChangedHandle;
	bool bContextRegistered{false};
};
