#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "APSQuestTypes.generated.h"

UENUM(BlueprintType)
enum class EAPSQuestEntityKind : uint8
{
	None,
	GameplayEntity,
	CivilizationEntity,
	StarSystem,
	CelestialBody,
	LandingContext,
	Player
};

UENUM(BlueprintType)
enum class EAPSQuestEventResult : uint8
{
	Requested,
	Started,
	Succeeded,
	Failed,
	Cancelled
};

UENUM(BlueprintType)
enum class EAPSQuestBindingMatch : uint8
{
	Any,
	Exact,
	NamedBinding
};

UENUM(BlueprintType)
enum class EAPSQuestNodeState : uint8
{
	Dormant,
	Active,
	Satisfied,
	Completed,
	Suspended,
	Failed
};

UENUM(BlueprintType)
enum class EAPSQuestInstanceState : uint8
{
	Inactive,
	Running,
	Completed,
	Suspended,
	Failed
};

UENUM(BlueprintType)
enum class EAPSQuestRewardState : uint8
{
	Requested,
	Applied,
	Failed
};

UENUM(BlueprintType)
enum class EAPSQuestRecoveryPolicy : uint8
{
	Retry,
	ResumeSameIdentity,
	RestartNode,
	DebugSkip
};

UENUM(BlueprintType)
enum class EAPSQuestPromptState : uint8
{
	Active,
	Cleared
};

/**
 * A typed canonical identity. Quest never converts actor names or transforms into
 * authoritative identity and never forces unlike identity domains into one fake GUID.
 */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestEntityRef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	EAPSQuestEntityKind Kind{EAPSQuestEntityKind::None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FGuid Guid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FString StableKey;

	bool IsValid() const
	{
		switch (Kind)
		{
		case EAPSQuestEntityKind::GameplayEntity:
		case EAPSQuestEntityKind::CivilizationEntity:
		case EAPSQuestEntityKind::StarSystem:
		case EAPSQuestEntityKind::Player:
			return Guid.IsValid();
		case EAPSQuestEntityKind::CelestialBody:
		case EAPSQuestEntityKind::LandingContext:
			return !StableKey.IsEmpty();
		default:
			return false;
		}
	}

	bool Matches(const FAPSQuestEntityRef& Other) const
	{
		return Kind == Other.Kind && Guid == Other.Guid && StableKey == Other.StableKey;
	}

	FString ToDebugString() const
	{
		return FString::Printf(TEXT("%d:%s:%s"), static_cast<int32>(Kind),
			*Guid.ToString(EGuidFormats::DigitsWithHyphensLower), *StableKey);
	}
};

/** Immutable fact normalized by an owner-provided adapter. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestEvent
{
	GENERATED_BODY()

	static constexpr int32 LatestSchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	int32 SchemaVersion{LatestSchemaVersion};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FGuid StreamId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FGuid EventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FGuid CorrelationId;

	/** Monotonic within StreamId. Zero is reserved for non-authoritative debug input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	int64 Sequence{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FName Verb;

	/** Authoritative recipe/buildable/ship definition identity, never a display/class string. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	int32 DefinitionSchemaVersion{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	EAPSQuestEventResult Result{EAPSQuestEventResult::Succeeded};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FAPSQuestEntityRef Subject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FAPSQuestEntityRef Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest", meta=(ClampMin="1"))
	int32 Quantity{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	FName FailureCode;

	/** Classification only; never used as authoritative identity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest")
	TArray<FName> ContextLabels;

	bool IsTerminal() const
	{
		return Result == EAPSQuestEventResult::Succeeded
			|| Result == EAPSQuestEventResult::Failed
			|| Result == EAPSQuestEventResult::Cancelled;
	}

	bool IsStructurallyValid(FString* OutReason = nullptr) const
	{
		auto Fail = [OutReason](const TCHAR* Reason)
		{
			if (OutReason)
			{
				*OutReason = Reason;
			}
			return false;
		};

		if (SchemaVersion <= 0 || SchemaVersion > LatestSchemaVersion)
		{
			return Fail(TEXT("Unsupported quest event schema"));
		}
		if (!EventId.IsValid() || Verb.IsNone())
		{
			return Fail(TEXT("Quest event requires EventId and Verb"));
		}
		if (Quantity < 1)
		{
			return Fail(TEXT("Quest event quantity must be positive"));
		}
		if (DefinitionSchemaVersion < 0
			|| (DefinitionId.IsValid() && DefinitionSchemaVersion < 1))
		{
			return Fail(TEXT("Quest event definition schema version is invalid"));
		}
		if (IsTerminal() && !CorrelationId.IsValid())
		{
			return Fail(TEXT("Terminal quest event requires CorrelationId"));
		}
		if (Result == EAPSQuestEventResult::Succeeded && !FailureCode.IsNone())
		{
			return Fail(TEXT("Successful quest event cannot carry FailureCode"));
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestPromptAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	FText Label;
};

/** Presentation-only immutable value copied to the UI-owned presenter. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestPromptSnapshot
{
	GENERATED_BODY()

	/** Changes on world travel so a late HUD cannot resurrect a previous-world prompt. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FGuid SessionEpoch;

	/** Stable for one quest-instance/node prompt across retries and presentation sessions. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FGuid PromptStableId;

	/** Quest InstanceId; no actor or widget identity participates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FGuid ContextStableId;

	/** Monotonic within SessionEpoch, including clear tombstones. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	int64 Revision{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	EAPSQuestPromptState State{EAPSQuestPromptState::Cleared};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FName PromptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FName QuestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FName NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FText Title;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FText Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FAPSQuestPromptAction> Actions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	int32 Priority{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	bool bModal{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	bool bDismissible{true};
};

/** Idempotent request for the system that owns the actual reward mutation. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestRewardCommand
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FGuid TransactionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FName QuestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FName NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FName RewardId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FName RewardType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	FAPSQuestEntityRef Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	int32 Quantity{1};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Quest")
	TMap<FName, FString> Parameters;
};
