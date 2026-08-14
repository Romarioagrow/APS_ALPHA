#pragma once

#include "CoreMinimal.h"
#include "APSQuestDefinition.h"
#include "APSEarlyAccessOnboardingDefinition.generated.h"

/** Stable names shared by owner-provided adapters and the native first-route definition. */
struct APS_ALPHA_API FAPSEarlyAccessOnboardingContract
{
	static const FName QuestId;

	static const FName BaseBinding;
	static const FName LandingPadBinding;
	static const FName SelectedShipBinding;
	static const FName PlayerCharacterBinding;
	static const FName DestinationBodyBinding;
	static const FName LandingContextBinding;
	static const FName HomeSystemBinding;

	static const FName CivilizationReadyNode;
	static const FName CharacterPossessedNode;
	static const FName BaseOpenedNode;
	static const FName StructurePlacedNode;
	static const FName LandingPadReachedNode;
	static const FName ShipPossessedNode;
	static const FName EnginesStartedNode;
	static const FName TakeoffCommittedNode;
	static const FName DestinationSelectedNode;
	static const FName ArrivalCommittedNode;
	static const FName LandingCommittedNode;
	static const FName SurfaceAnchorReadyNode;
	static const FName SurfaceEnteredNode;
	static const FName SurfaceExitedNode;
	static const FName SystemSelectionRestoredNode;
};

/** Native immutable archetype for the first Steam Early Access onboarding route. */
UCLASS(BlueprintType)
class APS_ALPHA_API UAPSEarlyAccessOnboardingDefinition : public UAPSQuestDefinition
{
	GENERATED_BODY()

public:
	UAPSEarlyAccessOnboardingDefinition();
};
