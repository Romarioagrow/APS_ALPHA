#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "APSQuestTypes.h"
#include "APSQuestDefinition.generated.h"

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestBindingDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName BindingName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	EAPSQuestEntityKind ExpectedKind{EAPSQuestEntityKind::None};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	bool bRequired{true};
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestEventPredicate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName Verb;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FPrimaryAssetId RequiredDefinitionId;

	/** Zero accepts unversioned facts; a typed RequiredDefinitionId requires at least one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="0"))
	int32 MinimumDefinitionSchemaVersion{0};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	EAPSQuestEventResult RequiredResult{EAPSQuestEventResult::Succeeded};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	EAPSQuestBindingMatch SubjectMatch{EAPSQuestBindingMatch::Any};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FAPSQuestEntityRef ExactSubject;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName SubjectBinding;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	EAPSQuestBindingMatch TargetMatch{EAPSQuestBindingMatch::Any};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FAPSQuestEntityRef ExactTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName TargetBinding;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="1"))
	int32 MinimumQuantity{1};
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestPromptDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName PromptId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FAPSQuestPromptAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	int32 Priority{0};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	bool bModal{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	bool bDismissible{true};
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestRewardDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName RewardId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName RewardType;

	/** Optional named entity binding resolved when the reward command is emitted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName TargetBinding;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="1"))
	int32 Quantity{1};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TMap<FName, FString> Parameters;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestObjectiveNodeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FAPSQuestEventPredicate Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="1"))
	int32 RequiredProgress{1};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FAPSQuestPromptDefinition Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FAPSQuestRewardDefinition> Rewards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	bool bRequired{true};
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSQuestEdgeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName FromNodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName ToNodeId;
};

/** Immutable authored quest graph. */
UCLASS(BlueprintType)
class APS_ALPHA_API UAPSQuestDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin="1"))
	int32 DefinitionVersion{1};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName EntryNodeId;

	/** Declares the canonical identity domain accepted by every named binding. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FAPSQuestBindingDefinition> BindingDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FAPSQuestObjectiveNodeDefinition> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FAPSQuestEdgeDefinition> SuccessEdges;

	const FAPSQuestBindingDefinition* FindBindingDefinition(FName BindingName) const;
	const FAPSQuestObjectiveNodeDefinition* FindNode(FName NodeId) const;
	void GetSuccessors(FName NodeId, TArray<FName>& OutSuccessors) const;
	bool ValidateDefinition(TArray<FString>& OutErrors) const;
};
