#include "APSEarlyAccessOnboardingDefinition.h"

#define LOCTEXT_NAMESPACE "APSEarlyAccessOnboarding"

const FName FAPSEarlyAccessOnboardingContract::QuestId(TEXT("APS.Onboarding.EarlyAccess.FirstRoute"));
const FName FAPSEarlyAccessOnboardingContract::BaseBinding(TEXT("Base"));
const FName FAPSEarlyAccessOnboardingContract::LandingPadBinding(TEXT("LandingPad"));
const FName FAPSEarlyAccessOnboardingContract::SelectedShipBinding(TEXT("SelectedShip"));
const FName FAPSEarlyAccessOnboardingContract::PlayerCharacterBinding(TEXT("PlayerCharacter"));
const FName FAPSEarlyAccessOnboardingContract::DestinationBodyBinding(TEXT("DestinationBody"));
const FName FAPSEarlyAccessOnboardingContract::LandingContextBinding(TEXT("LandingContext"));
const FName FAPSEarlyAccessOnboardingContract::HomeSystemBinding(TEXT("HomeSystem"));

const FName FAPSEarlyAccessOnboardingContract::CivilizationReadyNode(TEXT("CivilizationReady"));
const FName FAPSEarlyAccessOnboardingContract::CharacterPossessedNode(TEXT("CharacterPossessed"));
const FName FAPSEarlyAccessOnboardingContract::BaseOpenedNode(TEXT("BaseOpened"));
const FName FAPSEarlyAccessOnboardingContract::StructurePlacedNode(TEXT("StructurePlaced"));
const FName FAPSEarlyAccessOnboardingContract::LandingPadReachedNode(TEXT("LandingPadReached"));
const FName FAPSEarlyAccessOnboardingContract::ShipPossessedNode(TEXT("ShipPossessed"));
const FName FAPSEarlyAccessOnboardingContract::EnginesStartedNode(TEXT("EnginesStarted"));
const FName FAPSEarlyAccessOnboardingContract::TakeoffCommittedNode(TEXT("TakeoffCommitted"));
const FName FAPSEarlyAccessOnboardingContract::DestinationSelectedNode(TEXT("DestinationSelected"));
const FName FAPSEarlyAccessOnboardingContract::ArrivalCommittedNode(TEXT("ArrivalCommitted"));
const FName FAPSEarlyAccessOnboardingContract::LandingCommittedNode(TEXT("LandingCommitted"));
const FName FAPSEarlyAccessOnboardingContract::SurfaceAnchorReadyNode(TEXT("SurfaceAnchorReady"));
const FName FAPSEarlyAccessOnboardingContract::SurfaceEnteredNode(TEXT("SurfaceEntered"));
const FName FAPSEarlyAccessOnboardingContract::SurfaceExitedNode(TEXT("SurfaceExited"));
const FName FAPSEarlyAccessOnboardingContract::SystemSelectionRestoredNode(
	TEXT("SystemSelectionRestored"));

namespace
{
	void AddBinding(UAPSQuestDefinition& Definition, FName Name, EAPSQuestEntityKind Kind)
	{
		FAPSQuestBindingDefinition& Binding = Definition.BindingDefinitions.AddDefaulted_GetRef();
		Binding.BindingName = Name;
		Binding.ExpectedKind = Kind;
	}

	FAPSQuestObjectiveNodeDefinition& AddNode(UAPSQuestDefinition& Definition,
		FName NodeId, FName Verb, const FText& Title, const FText& Body,
		FName SubjectBinding = NAME_None, FName TargetBinding = NAME_None)
	{
		FAPSQuestObjectiveNodeDefinition& Node = Definition.Nodes.AddDefaulted_GetRef();
		Node.NodeId = NodeId;
		Node.Trigger.Verb = Verb;
		if (!SubjectBinding.IsNone())
		{
			Node.Trigger.SubjectMatch = EAPSQuestBindingMatch::NamedBinding;
			Node.Trigger.SubjectBinding = SubjectBinding;
		}
		if (!TargetBinding.IsNone())
		{
			Node.Trigger.TargetMatch = EAPSQuestBindingMatch::NamedBinding;
			Node.Trigger.TargetBinding = TargetBinding;
		}
		Node.Prompt.PromptId = FName(*FString::Printf(TEXT("APS.Prompt.EA.%s"),
			*NodeId.ToString()));
		Node.Prompt.Title = Title;
		Node.Prompt.Body = Body;
		FAPSQuestPromptAction& FocusAction = Node.Prompt.Actions.AddDefaulted_GetRef();
		FocusAction.ActionId = TEXT("FocusObjective");
		FocusAction.Label = LOCTEXT("FocusObjectiveAction", "Show objective");
		return Node;
	}

	void AddEdge(UAPSQuestDefinition& Definition, FName From, FName To)
	{
		FAPSQuestEdgeDefinition& Edge = Definition.SuccessEdges.AddDefaulted_GetRef();
		Edge.FromNodeId = From;
		Edge.ToNodeId = To;
	}
}

UAPSEarlyAccessOnboardingDefinition::UAPSEarlyAccessOnboardingDefinition()
{
	using Contract = FAPSEarlyAccessOnboardingContract;
	QuestId = Contract::QuestId;
	DefinitionVersion = 1;
	EntryNodeId = Contract::CivilizationReadyNode;

	AddBinding(*this, Contract::BaseBinding, EAPSQuestEntityKind::CivilizationEntity);
	AddBinding(*this, Contract::LandingPadBinding, EAPSQuestEntityKind::CivilizationEntity);
	AddBinding(*this, Contract::SelectedShipBinding, EAPSQuestEntityKind::CivilizationEntity);
	AddBinding(*this, Contract::PlayerCharacterBinding, EAPSQuestEntityKind::Player);
	AddBinding(*this, Contract::DestinationBodyBinding, EAPSQuestEntityKind::CelestialBody);
	AddBinding(*this, Contract::LandingContextBinding, EAPSQuestEntityKind::LandingContext);
	AddBinding(*this, Contract::HomeSystemBinding, EAPSQuestEntityKind::StarSystem);

	AddNode(*this, Contract::CivilizationReadyNode, TEXT("APS.Civilization.Manifest.Ready"),
		LOCTEXT("CivilizationReadyTitle", "Your foothold is ready"),
		LOCTEXT("CivilizationReadyBody", "Confirm the materialized base, landing pad, and selected ship."),
		NAME_None, Contract::BaseBinding);
	AddNode(*this, Contract::CharacterPossessedNode, TEXT("APS.Player.Character.Possessed"),
		LOCTEXT("CharacterPossessedTitle", "Take control"),
		LOCTEXT("CharacterPossessedBody", "Continue as the assigned character at the base."),
		Contract::PlayerCharacterBinding);
	AddNode(*this, Contract::BaseOpenedNode, TEXT("APS.Interaction.Open"),
		LOCTEXT("BaseOpenedTitle", "Inspect the station"),
		LOCTEXT("BaseOpenedBody", "Open or use the marked base interaction."),
		Contract::PlayerCharacterBinding, Contract::BaseBinding);
	AddNode(*this, Contract::StructurePlacedNode, TEXT("APS.Build.Place"),
		LOCTEXT("StructurePlacedTitle", "Build your first structure"),
		LOCTEXT("StructurePlacedBody", "Place one valid buildable and wait for committed success."),
		Contract::PlayerCharacterBinding);
	AddNode(*this, Contract::LandingPadReachedNode, TEXT("APS.Navigation.Reach"),
		LOCTEXT("LandingPadReachedTitle", "Reach the landing pad"),
		LOCTEXT("LandingPadReachedBody", "Travel to the pad assigned to your civilization."),
		Contract::PlayerCharacterBinding, Contract::LandingPadBinding);
	AddNode(*this, Contract::ShipPossessedNode, TEXT("APS.Flight.Ship.Possessed"),
		LOCTEXT("ShipPossessedTitle", "Board the selected ship"),
		LOCTEXT("ShipPossessedBody", "Take control of the exact ship assigned in the manifest."),
		Contract::PlayerCharacterBinding, Contract::SelectedShipBinding);
	AddNode(*this, Contract::EnginesStartedNode, TEXT("APS.Flight.Engine.Started"),
		LOCTEXT("EnginesStartedTitle", "Start the engines"),
		LOCTEXT("EnginesStartedBody", "Bring the selected ship online."),
		NAME_None, Contract::SelectedShipBinding);
	AddNode(*this, Contract::TakeoffCommittedNode, TEXT("APS.Flight.Takeoff.Committed"),
		LOCTEXT("TakeoffCommittedTitle", "Take off"),
		LOCTEXT("TakeoffCommittedBody", "Leave the pad and establish stable flight."),
		NAME_None, Contract::SelectedShipBinding);
	AddNode(*this, Contract::DestinationSelectedNode, TEXT("APS.Navigation.Body.Selected"),
		LOCTEXT("DestinationSelectedTitle", "Choose a destination"),
		LOCTEXT("DestinationSelectedBody", "Select the bound planet or moon as the navigation target."),
		Contract::PlayerCharacterBinding, Contract::DestinationBodyBinding);
	AddNode(*this, Contract::ArrivalCommittedNode, TEXT("APS.Navigation.Arrival.Committed"),
		LOCTEXT("ArrivalCommittedTitle", "Arrive at the destination"),
		LOCTEXT("ArrivalCommittedBody", "Complete navigation to the same canonical body."),
		NAME_None, Contract::DestinationBodyBinding);
	AddNode(*this, Contract::LandingCommittedNode, TEXT("APS.Surface.Landing.Committed"),
		LOCTEXT("LandingCommittedTitle", "Commit the landing"),
		LOCTEXT("LandingCommittedBody", "Establish the deterministic landing context."),
		Contract::DestinationBodyBinding, Contract::LandingContextBinding);
	AddNode(*this, Contract::SurfaceAnchorReadyNode, TEXT("APS.Surface.Anchor.Ready"),
		LOCTEXT("SurfaceAnchorReadyTitle", "Prepare the surface"),
		LOCTEXT("SurfaceAnchorReadyBody", "Wait for profile, LOD0, and spatial collision readiness."),
		Contract::DestinationBodyBinding, Contract::LandingContextBinding);
	AddNode(*this, Contract::SurfaceEnteredNode, TEXT("APS.Surface.Entered"),
		LOCTEXT("SurfaceEnteredTitle", "Enter the surface"),
		LOCTEXT("SurfaceEnteredBody", "Enter the ready surface environment."),
		Contract::DestinationBodyBinding, Contract::LandingContextBinding);
	AddNode(*this, Contract::SurfaceExitedNode, TEXT("APS.Surface.Exited"),
		LOCTEXT("SurfaceExitedTitle", "Return to orbit"),
		LOCTEXT("SurfaceExitedBody", "Exit the same landing context and return to orbital space."),
		Contract::DestinationBodyBinding, Contract::LandingContextBinding);
	FAPSQuestObjectiveNodeDefinition& Complete = AddNode(*this,
		Contract::SystemSelectionRestoredNode, TEXT("APS.Navigation.System.Selected"),
		LOCTEXT("SystemSelectionRestoredTitle", "Restore system navigation"),
		LOCTEXT("SystemSelectionRestoredBody", "Select the canonical home system to finish onboarding."),
		Contract::PlayerCharacterBinding, Contract::HomeSystemBinding);
	FAPSQuestRewardDefinition& Reward = Complete.Rewards.AddDefaulted_GetRef();
	Reward.RewardId = TEXT("CompleteEarlyAccessOnboarding");
	Reward.RewardType = TEXT("APS.Progression.Onboarding.Complete");
	Reward.TargetBinding = Contract::PlayerCharacterBinding;

	const TArray<FName> Route = {
		Contract::CivilizationReadyNode,
		Contract::CharacterPossessedNode,
		Contract::BaseOpenedNode,
		Contract::StructurePlacedNode,
		Contract::LandingPadReachedNode,
		Contract::ShipPossessedNode,
		Contract::EnginesStartedNode,
		Contract::TakeoffCommittedNode,
		Contract::DestinationSelectedNode,
		Contract::ArrivalCommittedNode,
		Contract::LandingCommittedNode,
		Contract::SurfaceAnchorReadyNode,
		Contract::SurfaceEnteredNode,
		Contract::SurfaceExitedNode,
		Contract::SystemSelectionRestoredNode
	};
	for (int32 Index = 1; Index < Route.Num(); ++Index)
	{
		AddEdge(*this, Route[Index - 1], Route[Index]);
	}
}

#undef LOCTEXT_NAMESPACE
