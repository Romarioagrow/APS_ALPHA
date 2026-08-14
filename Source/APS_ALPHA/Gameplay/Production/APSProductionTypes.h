#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "APS_ALPHA/Gameplay/Interaction/APSInteractionTypes.h"
#include "APSProductionTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EAPSProductionDomain : uint8
{
	Crafting,
	Building,
	Shipyard
};

UENUM(BlueprintType)
enum class EAPSProductionAccessMode : uint8
{
	/** Canonical actor identity and the registered context actor are mandatory. */
	ActorGated,

	/** Explicit development-only global launcher. Never aliases ActorGated access. */
	DebugLauncher
};

UENUM(BlueprintType)
enum class EAPSProductionJobState : uint8
{
	Queued,
	InProgress,
	AwaitingMaterialization,
	Succeeded,
	Failed,
	Cancelled
};

UENUM(BlueprintType)
enum class EAPSProductionCommandStatus : uint8
{
	Accepted,
	Rejected
};

/** Integer inventory amount keyed by an authoritative primary-asset definition. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionAmount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FPrimaryAssetId ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 ItemSchemaVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int64 Quantity{1};

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};

/** Data-only recipe/buildable/ship definition registered by its owning catalog adapter. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 DefinitionSchemaVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	EAPSProductionDomain Domain{EAPSProductionDomain::Crafting};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FName Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FAPSLocalizedTextDescriptor DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	TArray<FAPSProductionAmount> Costs;

	/** Crafting outputs only. Building and Shipyard completion is actor orchestration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	TArray<FAPSProductionAmount> Outputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="0.0"))
	double DurationSeconds{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 MaximumBatchSize{1};

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};

/** Runtime context binding. Canonical IDs are supplied by Civilization/world owners, never minted here. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionContextRegistration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid ContextStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid OwnerStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	EAPSProductionDomain Domain{EAPSProductionDomain::Crafting};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	EAPSProductionAccessMode AccessMode{EAPSProductionAccessMode::ActorGated};

	/** Required and identity-checked for ActorGated; must be null for DebugLauncher. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	TObjectPtr<AActor> ContextActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 QueueCapacity{8};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 MaximumConcurrentJobs{1};

	/** Canonical physical pad identity. Required for actor-gated Shipyard contexts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid SpawnPadStableId;

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};

/** Typed UI/gameplay action. Actor references are transient and never copied into snapshots or saves. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionCommand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid CorrelationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	int64 ExpectedRevision{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid SubjectStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid ContextStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid JobId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 Quantity{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	EAPSProductionAccessMode AccessMode{EAPSProductionAccessMode::ActorGated};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	TObjectPtr<AActor> InstigatorActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	TObjectPtr<AActor> ContextActor;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionCommandResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FGuid CorrelationId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FGuid JobId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	EAPSProductionCommandStatus Status{EAPSProductionCommandStatus::Rejected};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FName ResultCode;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FName FailureCode;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	int64 Revision{0};
};

/** Persisted generic job record. DefinitionId remains authoritative; class is diagnostics/migration only. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionJobRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid JobId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid CorrelationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid SubjectStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid ContextStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid OwnerStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 DefinitionSchemaVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	EAPSProductionDomain Domain{EAPSProductionDomain::Crafting};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int32 Quantity{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	EAPSProductionJobState State{EAPSProductionJobState::Queued};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="0.0"))
	double ProgressSeconds{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="0.0"))
	double DurationSeconds{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production", meta=(ClampMin="1"))
	int64 QueueOrdinal{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FName FailureCode;

	/** Non-authoritative Dev Ships migration/diagnostics snapshot. Never selects a ship definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FSoftClassPath ResolvedClassSnapshot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid SpawnPadStableId;

	/** Canonical actor ID supplied only after Building/Shipyard materialization succeeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid MaterializedActorStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	bool bInputsReserved{false};

	/** Explicit development-only provenance; debug jobs are excluded from persistence export. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	bool bDebugOnly{false};

	bool IsTerminal() const;
	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionDefinitionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	int32 DefinitionSchemaVersion{1};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FName Category;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FAPSLocalizedTextDescriptor DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	TArray<FAPSProductionAmount> Costs;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	TArray<FAPSProductionAmount> Outputs;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	int32 MaximumBatchSize{1};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	bool bAvailable{false};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FAPSLocalizedTextDescriptor UnavailableReason;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	TArray<FName> ActionIds;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionJobSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FGuid JobId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	int32 Quantity{1};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	EAPSProductionJobState State{EAPSProductionJobState::Queued};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	int32 QueuePosition{0};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	double ProgressNormalized{0.0};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FAPSLocalizedTextDescriptor StatusReason;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	TArray<FName> ActionIds;
};

/** Immutable presenter snapshot: no actors, widgets, save structs, brushes or raw keys. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FGuid SnapshotId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	int64 Revision{0};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FGuid ContextStableId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FGuid OwnerStableId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	EAPSProductionDomain Domain{EAPSProductionDomain::Crafting};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	EAPSProductionAccessMode AccessMode{EAPSProductionAccessMode::ActorGated};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	FPrimaryAssetId CurrentSelectionId;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	int32 QueueCapacity{0};

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	TArray<FAPSProductionDefinitionSnapshot> Definitions;

	UPROPERTY(BlueprintReadOnly, Category="APS|Production")
	TArray<FAPSProductionJobSnapshot> Jobs;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionInventoryRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FGuid OwnerStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	FPrimaryAssetId ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	int32 ItemSchemaVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	int64 Quantity{0};
};

/** Save-agnostic projection. A future save owner serializes this without exposing save structs here. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionPersistenceState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	int32 SchemaVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	TArray<FAPSProductionInventoryRecord> Inventories;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production")
	TArray<FAPSProductionJobRecord> Jobs;
};

