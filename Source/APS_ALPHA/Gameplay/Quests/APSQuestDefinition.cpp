#include "APSQuestDefinition.h"

namespace
{
	bool ValidateMatch(const TCHAR* Side, EAPSQuestBindingMatch Match,
		const FAPSQuestEntityRef& Exact, FName Binding, TArray<FString>& OutErrors,
		FName NodeId)
	{
		if (Match == EAPSQuestBindingMatch::Exact && !Exact.IsValid())
		{
			OutErrors.Add(FString::Printf(TEXT("Node %s has invalid exact %s identity"),
				*NodeId.ToString(), Side));
			return false;
		}
		if (Match == EAPSQuestBindingMatch::NamedBinding && Binding.IsNone())
		{
			OutErrors.Add(FString::Printf(TEXT("Node %s has empty %s binding"),
				*NodeId.ToString(), Side));
			return false;
		}
		return true;
	}
}

const FAPSQuestBindingDefinition* UAPSQuestDefinition::FindBindingDefinition(
	FName BindingName) const
{
	return BindingDefinitions.FindByPredicate(
		[BindingName](const FAPSQuestBindingDefinition& Binding)
		{
			return Binding.BindingName == BindingName;
		});
}

const FAPSQuestObjectiveNodeDefinition* UAPSQuestDefinition::FindNode(FName NodeId) const
{
	return Nodes.FindByPredicate([NodeId](const FAPSQuestObjectiveNodeDefinition& Node)
	{
		return Node.NodeId == NodeId;
	});
}

void UAPSQuestDefinition::GetSuccessors(FName NodeId, TArray<FName>& OutSuccessors) const
{
	OutSuccessors.Reset();
	for (const FAPSQuestEdgeDefinition& Edge : SuccessEdges)
	{
		if (Edge.FromNodeId == NodeId)
		{
			OutSuccessors.Add(Edge.ToNodeId);
		}
	}
}

bool UAPSQuestDefinition::ValidateDefinition(TArray<FString>& OutErrors) const
{
	OutErrors.Reset();
	if (QuestId.IsNone())
	{
		OutErrors.Add(TEXT("QuestId is required"));
	}
	if (DefinitionVersion < 1)
	{
		OutErrors.Add(TEXT("DefinitionVersion must be positive"));
	}
	if (EntryNodeId.IsNone())
	{
		OutErrors.Add(TEXT("EntryNodeId is required"));
	}
	if (Nodes.IsEmpty())
	{
		OutErrors.Add(TEXT("At least one objective node is required"));
		return false;
	}

	TSet<FName> BindingNames;
	for (const FAPSQuestBindingDefinition& Binding : BindingDefinitions)
	{
		if (Binding.BindingName.IsNone() || Binding.ExpectedKind == EAPSQuestEntityKind::None)
		{
			OutErrors.Add(TEXT("Binding definitions require a name and canonical identity kind"));
			continue;
		}
		if (BindingNames.Contains(Binding.BindingName))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate binding definition %s"),
				*Binding.BindingName.ToString()));
		}
		BindingNames.Add(Binding.BindingName);
	}

	TSet<FName> NodeIds;
	for (const FAPSQuestObjectiveNodeDefinition& Node : Nodes)
	{
		if (Node.NodeId.IsNone())
		{
			OutErrors.Add(TEXT("Objective node has an empty NodeId"));
			continue;
		}
		if (NodeIds.Contains(Node.NodeId))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate node %s"), *Node.NodeId.ToString()));
		}
		NodeIds.Add(Node.NodeId);
		if (Node.Trigger.Verb.IsNone())
		{
			OutErrors.Add(FString::Printf(TEXT("Node %s requires a trigger verb"),
				*Node.NodeId.ToString()));
		}
		if (Node.Trigger.MinimumDefinitionSchemaVersion < 0
			|| (Node.Trigger.RequiredDefinitionId.IsValid()
				&& Node.Trigger.MinimumDefinitionSchemaVersion < 1))
		{
			OutErrors.Add(FString::Printf(TEXT("Node %s has an invalid definition identity schema"),
				*Node.NodeId.ToString()));
		}
		if (Node.RequiredProgress < 1 || Node.Trigger.MinimumQuantity < 1)
		{
			OutErrors.Add(FString::Printf(TEXT("Node %s requires positive progress quantities"),
				*Node.NodeId.ToString()));
		}
		ValidateMatch(TEXT("subject"), Node.Trigger.SubjectMatch, Node.Trigger.ExactSubject,
			Node.Trigger.SubjectBinding, OutErrors, Node.NodeId);
		ValidateMatch(TEXT("target"), Node.Trigger.TargetMatch, Node.Trigger.ExactTarget,
			Node.Trigger.TargetBinding, OutErrors, Node.NodeId);

		if (Node.Trigger.SubjectMatch == EAPSQuestBindingMatch::NamedBinding
			&& !BindingNames.Contains(Node.Trigger.SubjectBinding))
		{
			OutErrors.Add(FString::Printf(TEXT("Node %s references undeclared subject binding %s"),
				*Node.NodeId.ToString(), *Node.Trigger.SubjectBinding.ToString()));
		}
		if (Node.Trigger.TargetMatch == EAPSQuestBindingMatch::NamedBinding
			&& !BindingNames.Contains(Node.Trigger.TargetBinding))
		{
			OutErrors.Add(FString::Printf(TEXT("Node %s references undeclared target binding %s"),
				*Node.NodeId.ToString(), *Node.Trigger.TargetBinding.ToString()));
		}

		TSet<FName> RewardIds;
		for (const FAPSQuestRewardDefinition& Reward : Node.Rewards)
		{
			if (Reward.RewardId.IsNone() || Reward.RewardType.IsNone() || Reward.Quantity < 1)
			{
				OutErrors.Add(FString::Printf(TEXT("Node %s has an invalid reward"),
					*Node.NodeId.ToString()));
			}
			if (!Reward.TargetBinding.IsNone()
				&& !BindingNames.Contains(Reward.TargetBinding))
			{
				OutErrors.Add(FString::Printf(TEXT("Node %s reward %s references undeclared binding %s"),
					*Node.NodeId.ToString(), *Reward.RewardId.ToString(),
					*Reward.TargetBinding.ToString()));
			}
			if (RewardIds.Contains(Reward.RewardId))
			{
				OutErrors.Add(FString::Printf(TEXT("Node %s has duplicate reward %s"),
					*Node.NodeId.ToString(), *Reward.RewardId.ToString()));
			}
			RewardIds.Add(Reward.RewardId);
		}
	}

	if (!NodeIds.Contains(EntryNodeId))
	{
		OutErrors.Add(FString::Printf(TEXT("Entry node %s does not exist"), *EntryNodeId.ToString()));
	}

	TSet<FString> EdgeKeys;
	TMap<FName, int32> InDegree;
	for (FName NodeId : NodeIds)
	{
		InDegree.Add(NodeId, 0);
	}
	for (const FAPSQuestEdgeDefinition& Edge : SuccessEdges)
	{
		if (!NodeIds.Contains(Edge.FromNodeId) || !NodeIds.Contains(Edge.ToNodeId))
		{
			OutErrors.Add(FString::Printf(TEXT("Edge %s -> %s references a missing node"),
				*Edge.FromNodeId.ToString(), *Edge.ToNodeId.ToString()));
			continue;
		}
		if (Edge.FromNodeId == Edge.ToNodeId)
		{
			OutErrors.Add(FString::Printf(TEXT("Self cycle at node %s"), *Edge.FromNodeId.ToString()));
		}
		const FString EdgeKey = Edge.FromNodeId.ToString() + TEXT("\x1f")
			+ Edge.ToNodeId.ToString();
		if (EdgeKeys.Contains(EdgeKey))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate edge %s -> %s"),
				*Edge.FromNodeId.ToString(), *Edge.ToNodeId.ToString()));
		}
		EdgeKeys.Add(EdgeKey);
		++InDegree.FindChecked(Edge.ToNodeId);
	}

	if (NodeIds.Contains(EntryNodeId))
	{
		TSet<FName> Reachable;
		TArray<FName> Work{EntryNodeId};
		while (!Work.IsEmpty())
		{
			const FName Current = Work.Pop(EAllowShrinking::No);
			if (Reachable.Contains(Current))
			{
				continue;
			}
			Reachable.Add(Current);
			TArray<FName> Successors;
			GetSuccessors(Current, Successors);
			Work.Append(Successors);
		}
		for (const FAPSQuestObjectiveNodeDefinition& Node : Nodes)
		{
			if (Node.bRequired && !Reachable.Contains(Node.NodeId))
			{
				OutErrors.Add(FString::Printf(TEXT("Required node %s is unreachable"),
					*Node.NodeId.ToString()));
			}
		}
	}

	// V1 keeps recovery out of graph edges and rejects every authored cycle.
	TArray<FName> ZeroInDegree;
	for (const TPair<FName, int32>& Pair : InDegree)
	{
		if (Pair.Value == 0)
		{
			ZeroInDegree.Add(Pair.Key);
		}
	}
	int32 VisitedCount = 0;
	while (!ZeroInDegree.IsEmpty())
	{
		const FName Current = ZeroInDegree.Pop(EAllowShrinking::No);
		++VisitedCount;
		TArray<FName> Successors;
		GetSuccessors(Current, Successors);
		for (FName Successor : Successors)
		{
			int32& Degree = InDegree.FindChecked(Successor);
			if (--Degree == 0)
			{
				ZeroInDegree.Add(Successor);
			}
		}
	}
	if (VisitedCount != NodeIds.Num())
	{
		OutErrors.Add(TEXT("Quest graph contains a cycle"));
	}

	return OutErrors.IsEmpty();
}
