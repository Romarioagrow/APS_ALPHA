#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSProductionEvent.h"
#include "APSProductionTypes.h"
#include "APSProductionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAPSProductionSnapshotInvalidated,
	const FGuid& /* ContextStableId */);

/**
 * Authoritative, save-agnostic production runtime shared by Crafting, Building and Shipyard.
 * Actor-gated and debug-global contexts are separate registrations and can never alias access.
 */
UCLASS()
class APS_ALPHA_API UAPSProductionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category="APS|Production")
	bool RegisterDefinition(const FAPSProductionDefinition& Definition,
		FString& OutFailure);

	UFUNCTION(BlueprintCallable, Category="APS|Production")
	bool RegisterContext(const FAPSProductionContextRegistration& Registration,
		FString& OutFailure);

	UFUNCTION(BlueprintCallable, Category="APS|Production")
	bool UnregisterContext(FGuid ContextStableId, AActor* ContextActor,
		EAPSProductionAccessMode AccessMode, FString& OutFailure);

	UFUNCTION(BlueprintCallable, Category="APS|Production")
	bool QuerySnapshot(FGuid ContextStableId, AActor* ContextActor,
		EAPSProductionAccessMode AccessMode, FAPSProductionSnapshot& OutSnapshot,
		FString& OutFailure) const;

	UFUNCTION(BlueprintCallable, Category="APS|Production")
	FAPSProductionCommandResult ExecuteCommand(const FAPSProductionCommand& Command);

	/** Deterministic seam used by Tick and focused automation. */
	UFUNCTION(BlueprintCallable, Category="APS|Production")
	void AdvanceProduction(double DeltaSeconds);

	/** Building/Surface and Shipyard/Ships adapters resolve an awaiting job exactly once. */
	bool ResolveMaterialization(const FGuid& ContextStableId, const FGuid& JobId,
		const FGuid& MaterializedActorStableId, const FSoftClassPath& ResolvedClassSnapshot,
		bool bSucceeded, FName FailureCode, FString& OutFailure);

	/** Gameplay resource adapter seam; never exposed as a UI action. */
	bool SetInventoryAmount(const FAPSProductionInventoryRecord& Record,
		FString& OutFailure);

	/** Gameplay resource adapter seam for atomic positive/negative deltas. */
	bool ApplyInventoryDelta(const FGuid& OwnerStableId,
		const FAPSProductionAmount& Amount, bool bAdd, FString& OutFailure);

	int64 GetInventoryQuantity(const FGuid& OwnerStableId,
		const FPrimaryAssetId& ItemId) const;

	void ExportPersistenceState(FAPSProductionPersistenceState& OutState) const;
	bool RestorePersistenceState(const FAPSProductionPersistenceState& State,
		FString& OutFailure);

	FOnAPSProductionSnapshotInvalidated& OnSnapshotInvalidated()
	{
		return SnapshotInvalidated;
	}

private:
	struct FInventoryEntry
	{
		int32 SchemaVersion{1};
		int64 Quantity{0};
	};

	struct FContextState
	{
		FGuid ContextStableId;
		FGuid OwnerStableId;
		EAPSProductionDomain Domain{EAPSProductionDomain::Crafting};
		EAPSProductionAccessMode AccessMode{EAPSProductionAccessMode::ActorGated};
		TWeakObjectPtr<AActor> ContextActor;
		int32 QueueCapacity{8};
		int32 MaximumConcurrentJobs{1};
		FGuid SpawnPadStableId;
		FGuid SnapshotId;
		int64 Revision{1};
		FPrimaryAssetId SelectedDefinitionId;
		TArray<FAPSProductionJobRecord> Jobs;
		int64 NextQueueOrdinal{1};
	};

	bool ValidateAccess(const FContextState& Context, AActor* ContextActor,
		EAPSProductionAccessMode AccessMode, FString& OutFailure) const;
	bool ValidateCommand(const FAPSProductionCommand& Command,
		const FContextState& Context, FString& OutFailure) const;
	bool CanAfford(const FContextState& Context,
		const FAPSProductionDefinition& Definition, int32 Quantity) const;
	bool ReserveCosts(const FContextState& Context,
		const FAPSProductionDefinition& Definition, int32 Quantity,
		FString& OutFailure);
	void RefundCosts(const FContextState& Context,
		const FAPSProductionDefinition& Definition, int32 Quantity);
	bool ApplyOutputs(const FContextState& Context,
		const FAPSProductionDefinition& Definition, int32 Quantity,
		FString& OutFailure);
	bool PublishJobEvent(const FContextState& Context,
		const FAPSProductionJobRecord& Job, EAPSProductionEventResult Result,
		FName ResultCode, FName FailureCode, FString& OutFailure) const;
	void StartQueuedJobs(FContextState& Context);
	void CompleteJob(FContextState& Context, FAPSProductionJobRecord& Job);
	void BumpRevision(FContextState& Context);
	int32 CountActiveJobs(const FContextState& Context) const;
	int32 CountInProgressJobs(const FContextState& Context) const;
	static bool CheckedScaleQuantity(int64 UnitQuantity, int32 Multiplier,
		int64& OutQuantity);
	static FName VerbForDomain(EAPSProductionDomain Domain);

	TMap<FPrimaryAssetId, FAPSProductionDefinition> Definitions;
	TMap<FGuid, FContextState> Contexts;
	TMap<FGuid, TMap<FPrimaryAssetId, FInventoryEntry>> Inventories;
	FOnAPSProductionSnapshotInvalidated SnapshotInvalidated;
};
