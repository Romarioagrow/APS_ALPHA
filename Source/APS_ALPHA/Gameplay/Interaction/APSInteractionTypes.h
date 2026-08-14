#pragma once

#include "CoreMinimal.h"
#include "APSInteractionTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EAPSInteractionContextKind : uint8
{
	WorldActor,
	Vehicle,
	HeadquartersConsole,
	CraftingStation,
	BuildSite,
	Shipyard,
	DebugLauncher
};

UENUM(BlueprintType)
enum class EAPSInteractionPromptKind : uint8
{
	ContextAction,
	OpenPanel,
	Confirm,
	Progress,
	Status
};

UENUM(BlueprintType)
enum class EAPSInteractionAvailability : uint8
{
	Hidden,
	Available,
	Unavailable,
	InProgress
};

UENUM(BlueprintType)
enum class EAPSInteractionActionKind : uint8
{
	Primary,
	Secondary,
	Open,
	Use,
	TakeControl,
	Craft,
	Build,
	Shipyard,
	Debug
};

UENUM(BlueprintType)
enum class EAPSInteractionInputMode : uint8
{
	Press,
	Hold
};

UENUM(BlueprintType)
enum class EAPSInteractionExecutionStatus : uint8
{
	Succeeded,
	Failed,
	Deferred
};

/** Semantic domain of an authoritative actor StableId. Gameplay never infers this from class/name. */
UENUM(BlueprintType)
enum class EAPSSubjectIdentityDomain : uint8
{
	None,
	GameplayEntity,
	Player
};

/** Semantic domain of the authoritative target StableId. */
UENUM(BlueprintType)
enum class EAPSTargetIdentityDomain : uint8
{
	None,
	GameplayEntity,
	CivilizationEntity
};

/** Presentation-neutral localization token. UI owns FText creation and styling. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSLocalizedTextDescriptor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FName Namespace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FName Key;

	/** Development fallback only; never an authoritative styled prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FString DefaultText;

	/** Positional, presentation-only format arguments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	TArray<FString> FormatArguments;

	bool IsSet() const { return !Key.IsNone() || !DefaultText.IsEmpty(); }
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSInteractionActionDescriptor
{
	GENERATED_BODY()

	/** Stable semantic ID within the prompt contract, not an input key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	EAPSInteractionActionKind ActionKind{EAPSInteractionActionKind::Primary};

	/** Enhanced Input action name resolved to the active device glyph by Dev UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FName InputActionName{TEXT("Interact")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bEnabled{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FAPSLocalizedTextDescriptor UnavailableReason;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	EAPSInteractionInputMode InputMode{EAPSInteractionInputMode::Press};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0.0"))
	double HoldDurationSeconds{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0.0", ClampMax="1.0"))
	double HoldProgress{0.0};

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};

/** Immutable snapshot consumed by a UI-owned presenter. Contains no widget or actor references. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSInteractionPromptDescriptor
{
	GENERATED_BODY()

	/** Stable for the life of this prompt; revisions change without changing PromptId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FGuid PromptId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	int64 Revision{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	EAPSInteractionPromptKind PromptKind{EAPSInteractionPromptKind::ContextAction};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	EAPSInteractionContextKind ContextKind{EAPSInteractionContextKind::WorldActor};

	/** Canonical actor/entity identity. Production prompts require a valid value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FGuid TargetStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	EAPSTargetIdentityDomain TargetIdentityDomain{EAPSTargetIdentityDomain::None};

	/** Stable gameplay context backing the prompt; may equal TargetStableId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FGuid ContextStableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FAPSLocalizedTextDescriptor DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0.0"))
	double DistanceCm{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0.0"))
	double RangeCm{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	EAPSInteractionAvailability Availability{EAPSInteractionAvailability::Hidden};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	TArray<FAPSInteractionActionDescriptor> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	int32 Priority{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bBlocking{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bDismissible{true};

	/** Negative means no expiry. Uses world time and is never persisted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	double ExpirationWorldSeconds{-1.0};

	bool IsStructurallyValid(bool bRequireAuthoritativeTargetId,
		FString* OutReason = nullptr) const;
};

/** Transient gameplay query context. It is never sent to UI or persisted. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSInteractionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	TObjectPtr<AActor> InstigatorActor;

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FGuid SubjectStableId;

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	EAPSSubjectIdentityDomain SubjectIdentityDomain{EAPSSubjectIdentityDomain::None};

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FVector ViewOrigin{FVector::ZeroVector};

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FVector ViewDirection{FVector::ForwardVector};

	UPROPERTY(BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0.0"))
	double MaximumRangeCm{600.0};

	/** Minimum camera-forward alignment to the candidate bounds centre. */
	UPROPERTY(BlueprintReadWrite, Category="Interaction", meta=(ClampMin="-1.0", ClampMax="1.0"))
	double MinimumFocusDot{0.70};

	/** Permits invalid canonical IDs only for a visibly debug-only launcher path. */
	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	bool bDebugIdentityOverride{false};
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSInteractionExecutionRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FGuid CorrelationId;

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FName ActionId;

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	int64 ExpectedRevision{0};

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FGuid SubjectStableId;

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	EAPSSubjectIdentityDomain SubjectIdentityDomain{EAPSSubjectIdentityDomain::None};

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	FGuid TargetStableId;

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	EAPSTargetIdentityDomain TargetIdentityDomain{EAPSTargetIdentityDomain::None};

	UPROPERTY(BlueprintReadWrite, Category="Interaction", meta=(ClampMin="1"))
	int32 Quantity{1};

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	TObjectPtr<AActor> InstigatorActor;

	bool IsStructurallyValid(bool bRequireAuthoritativeIds,
		FString* OutReason = nullptr) const;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSInteractionExecutionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FGuid CorrelationId;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	EAPSInteractionExecutionStatus Status{EAPSInteractionExecutionStatus::Failed};

	/** Explicit semantic verb authored by the executed interactable. */
	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FName Verb;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FName ResultCode;

	/** Stable non-localized code. Empty unless Status is Failed. */
	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FName FailureCode;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	int32 Quantity{0};
};

/** Immutable, presentation-neutral fact emitted only after an interaction executes. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSInteractionExecutionEvent
{
	GENERATED_BODY()

	/** Interaction-owned stream identity. It never aliases a manifest or world ID. */
	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FGuid StreamId;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FGuid EventId;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FGuid CorrelationId;

	/** Monotonic within StreamId. Validation rejects non-positive values. */
	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	int64 Sequence{0};

	/** Explicit verb supplied by the interactable; never inferred from ActionId or UI state. */
	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FName Verb;

	/** The already validated semantic action that caused execution. */
	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FName ActionId;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FGuid SubjectStableId;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	EAPSSubjectIdentityDomain SubjectIdentityDomain{EAPSSubjectIdentityDomain::None};

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FGuid TargetStableId;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	EAPSTargetIdentityDomain TargetIdentityDomain{EAPSTargetIdentityDomain::None};

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	int32 Quantity{0};

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	EAPSInteractionExecutionStatus Status{EAPSInteractionExecutionStatus::Failed};

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FName ResultCode;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FName FailureCode;

	/** Explicit debug classification; Quest adapters must reject these events. */
	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	bool bDebugOnly{false};

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
};
