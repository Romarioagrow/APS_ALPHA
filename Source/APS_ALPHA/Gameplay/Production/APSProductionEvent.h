#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "APSProductionEvent.generated.h"

UENUM(BlueprintType)
enum class EAPSProductionEventResult : uint8
{
	Requested,
	Started,
	Succeeded,
	Failed,
	Cancelled
};

/** Neutral gameplay event consumed by Quest adapters and persistence projections. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="APS|Production Event")
	FGuid EventId;

	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	FGuid CorrelationId;

	/** Stable semantic verb such as APS.Crafting.Craft or APS.Shipyard.Complete. */
	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	FName Verb;

	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	FGuid SubjectStableId;

	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	FGuid TargetStableId;

	/** Recipe/buildable/ship type identity; actor/job identity remains TargetStableId. */
	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event", meta=(ClampMin="1"))
	int32 DefinitionSchemaVersion{1};

	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event", meta=(ClampMin="1"))
	int32 Quantity{1};

	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	EAPSProductionEventResult Result{EAPSProductionEventResult::Requested};

	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	FName ResultCode;

	/** Stable non-localized code, required for Failed and empty otherwise. */
	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	FName FailureCode;

	/** Non-authoritative classification only; never objective state by itself. */
	UPROPERTY(BlueprintReadWrite, Category="APS|Production Event")
	TArray<FName> ContextTags;

	/** Monotonic per world/save stream. */
	UPROPERTY(BlueprintReadOnly, Category="APS|Production Event")
	int64 Sequence{0};

	/** Telemetry only; Quest must use Sequence and dedupe IDs for ordering. */
	UPROPERTY(BlueprintReadOnly, Category="APS|Production Event")
	double TimestampWorldSeconds{0.0};

	bool IsTerminal() const;
	bool IsStructurallyValid(bool bRequireSubjectId, bool bRequireTargetId,
		FString* OutReason = nullptr) const;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionEventPublishPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	bool bRequireSubjectStableId{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	bool bRequireTargetStableId{true};

	/** Must be explicit and visible; never enabled for production actor-gated access. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	bool bDebugOnly{false};
};

/** Persisted lifecycle identity required to continue one correlation after load. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionEventCorrelationRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	FGuid CorrelationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	FName Verb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	FGuid SubjectStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	FGuid TargetStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event", meta=(ClampMin="1"))
	int32 DefinitionSchemaVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event", meta=(ClampMin="1"))
	int32 Quantity{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	EAPSProductionEventResult Result{EAPSProductionEventResult::Requested};

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};

/** Save-agnostic event stream projection. Debug-only correlations never enter it. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSProductionEventStreamState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	int32 SchemaVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	int64 LastSequence{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Production Event")
	TArray<FAPSProductionEventCorrelationRecord> Correlations;

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};
