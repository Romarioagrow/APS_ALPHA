#include "APS_ALPHA/Core/World/APSPlanetSurfaceTransitionSubsystem.h"

namespace APSPlanetSurfaceTransition
{
	constexpr int32 KnownReadinessMask =
		static_cast<int32>(EAPSPlanetSurfaceReadiness::Profile)
		| static_cast<int32>(EAPSPlanetSurfaceReadiness::LOD0)
		| static_cast<int32>(EAPSPlanetSurfaceReadiness::Collision);

	constexpr int32 AnchorReadyMask = KnownReadinessMask;
}

bool FAPSPlanetSurfaceTransitionFact::IsExactlyEqual(
	const FAPSPlanetSurfaceTransitionFact& Other) const
{
	return StreamId == Other.StreamId
		&& EventId == Other.EventId
		&& CorrelationId == Other.CorrelationId
		&& Sequence == Other.Sequence
		&& BodyStableKey == Other.BodyStableKey
		&& LandingContextStableKey == Other.LandingContextStableKey
		&& Phase == Other.Phase
		&& ReadinessBits == Other.ReadinessBits
		&& Reason == Other.Reason;
}

bool FAPSPlanetSurfaceTransitionSnapshot::TryGetLatestCommittedFact(
	const EAPSPlanetSurfaceTransitionPhase Phase,
	FAPSPlanetSurfaceTransitionFact& OutFact) const
{
	for (const FAPSPlanetSurfaceTransitionFact& Fact : LatestCommittedFacts)
	{
		if (Fact.Phase == Phase)
		{
			OutFact = Fact;
			return true;
		}
	}

	OutFact = FAPSPlanetSurfaceTransitionFact{};
	return false;
}

FString UAPSPlanetSurfaceTransitionSubsystem::BuildLandingContextStableKey(
	const FString& BodyStableKey,
	const int64 PlacementKey)
{
	if (BodyStableKey.IsEmpty()
		|| PlacementKey <= 0
		|| static_cast<uint64>(PlacementKey) > static_cast<uint64>(MAX_uint32))
	{
		return FString{};
	}

	return FString::Printf(
		TEXT("%s/EA-LANDING-%u"),
		*BodyStableKey,
		static_cast<uint32>(PlacementKey));
}

bool UAPSPlanetSurfaceTransitionSubsystem::CommitTransition(
	const FAPSPlanetSurfaceTransitionCommitRequest& Request,
	FAPSPlanetSurfaceTransitionFact& OutFact)
{
	OutFact = FAPSPlanetSurfaceTransitionFact{};
	if (!ValidateRequest(Request))
	{
		return false;
	}

	const FString ContextKey = BuildSnapshotMapKey(
		Request.BodyStableKey,
		Request.LandingContextStableKey);
	FAPSPlanetSurfaceTransitionSnapshot& Snapshot =
		SnapshotsByContext.FindOrAdd(ContextKey);
	Snapshot.BodyStableKey = Request.BodyStableKey;
	Snapshot.LandingContextStableKey = Request.LandingContextStableKey;

	FAPSPlanetSurfaceTransitionFact ExistingFact;
	if (Snapshot.TryGetLatestCommittedFact(Request.Phase, ExistingFact)
		&& MatchesRequestPayload(ExistingFact, Request))
	{
		OutFact = ExistingFact;
		return true;
	}

	int64& LastSequence = LastSequenceByBody.FindOrAdd(Request.BodyStableKey);
	if (LastSequence == MAX_int64)
	{
		return false;
	}

	++LastSequence;
	FAPSPlanetSurfaceTransitionFact NewFact;
	NewFact.StreamId = BuildStreamId(Request.BodyStableKey);
	NewFact.EventId = BuildEventId(NewFact.StreamId, LastSequence);
	NewFact.CorrelationId = Request.CorrelationId;
	NewFact.Sequence = LastSequence;
	NewFact.BodyStableKey = Request.BodyStableKey;
	NewFact.LandingContextStableKey = Request.LandingContextStableKey;
	NewFact.Phase = Request.Phase;
	NewFact.ReadinessBits = Request.ReadinessBits;
	NewFact.Reason = Request.Reason;

	StoreLatestFact(Snapshot, NewFact);
	OutFact = NewFact;
	TransitionCommitted.Broadcast(NewFact);
	return true;
}

bool UAPSPlanetSurfaceTransitionSubsystem::GetTransitionSnapshot(
	const FString& BodyStableKey,
	const FString& LandingContextStableKey,
	FAPSPlanetSurfaceTransitionSnapshot& OutSnapshot) const
{
	OutSnapshot = FAPSPlanetSurfaceTransitionSnapshot{};
	if (BodyStableKey.IsEmpty()
		|| (!LandingContextStableKey.IsEmpty()
			&& !ValidateLandingContextStableKey(BodyStableKey, LandingContextStableKey)))
	{
		return false;
	}

	OutSnapshot.BodyStableKey = BodyStableKey;
	OutSnapshot.LandingContextStableKey = LandingContextStableKey;

	const FAPSPlanetSurfaceTransitionSnapshot* ArrivalSnapshot =
		SnapshotsByContext.Find(BuildSnapshotMapKey(BodyStableKey, FString{}));
	if (ArrivalSnapshot)
	{
		FAPSPlanetSurfaceTransitionFact ArrivalFact;
		if (ArrivalSnapshot->TryGetLatestCommittedFact(
			EAPSPlanetSurfaceTransitionPhase::ArrivalCommitted,
			ArrivalFact))
		{
			OutSnapshot.LatestCommittedFacts.Add(ArrivalFact);
		}
	}

	if (!LandingContextStableKey.IsEmpty())
	{
		const FAPSPlanetSurfaceTransitionSnapshot* ContextSnapshot =
			SnapshotsByContext.Find(BuildSnapshotMapKey(
				BodyStableKey,
				LandingContextStableKey));
		if (ContextSnapshot)
		{
			for (const FAPSPlanetSurfaceTransitionFact& Fact
				: ContextSnapshot->LatestCommittedFacts)
			{
				StoreLatestFact(OutSnapshot, Fact);
			}
		}
	}

	OutSnapshot.LatestCommittedFacts.Sort(
		[](const FAPSPlanetSurfaceTransitionFact& Left,
			const FAPSPlanetSurfaceTransitionFact& Right)
		{
			return static_cast<uint8>(Left.Phase) < static_cast<uint8>(Right.Phase);
		});
	return OutSnapshot.LatestCommittedFacts.Num() > 0;
}

bool UAPSPlanetSurfaceTransitionSubsystem::TryGetLatestCommittedFact(
	const FString& BodyStableKey,
	const FString& LandingContextStableKey,
	const EAPSPlanetSurfaceTransitionPhase Phase,
	FAPSPlanetSurfaceTransitionFact& OutFact) const
{
	FAPSPlanetSurfaceTransitionSnapshot Snapshot;
	if (!GetTransitionSnapshot(BodyStableKey, LandingContextStableKey, Snapshot))
	{
		OutFact = FAPSPlanetSurfaceTransitionFact{};
		return false;
	}

	return Snapshot.TryGetLatestCommittedFact(Phase, OutFact);
}

bool UAPSPlanetSurfaceTransitionSubsystem::RestoreTransitionSnapshot(
	const FAPSPlanetSurfaceTransitionSnapshot& Snapshot)
{
	if (Snapshot.BodyStableKey.IsEmpty()
		|| Snapshot.LatestCommittedFacts.IsEmpty()
		|| (!Snapshot.LandingContextStableKey.IsEmpty()
			&& !ValidateLandingContextStableKey(
				Snapshot.BodyStableKey,
				Snapshot.LandingContextStableKey)))
	{
		return false;
	}

	TSet<EAPSPlanetSurfaceTransitionPhase> RestoredPhases;
	TSet<int64> RestoredSequences;
	int64 MaximumSequence = LastSequenceByBody.FindRef(Snapshot.BodyStableKey);
	for (const FAPSPlanetSurfaceTransitionFact& Fact : Snapshot.LatestCommittedFacts)
	{
		FAPSPlanetSurfaceTransitionCommitRequest Request;
		Request.BodyStableKey = Fact.BodyStableKey;
		Request.LandingContextStableKey = Fact.LandingContextStableKey;
		Request.Phase = Fact.Phase;
		Request.CorrelationId = Fact.CorrelationId;
		Request.Reason = Fact.Reason;
		Request.ReadinessBits = Fact.ReadinessBits;

		const bool bSnapshotContextMatches =
			Fact.Phase == EAPSPlanetSurfaceTransitionPhase::ArrivalCommitted
				? Fact.LandingContextStableKey.IsEmpty()
				: Fact.LandingContextStableKey == Snapshot.LandingContextStableKey;
		if (Fact.BodyStableKey != Snapshot.BodyStableKey
			|| !bSnapshotContextMatches
			|| !ValidateRequest(Request)
			|| Fact.Sequence <= 0
			|| RestoredPhases.Contains(Fact.Phase)
			|| RestoredSequences.Contains(Fact.Sequence)
			|| Fact.StreamId != BuildStreamId(Fact.BodyStableKey)
			|| Fact.EventId != BuildEventId(Fact.StreamId, Fact.Sequence))
		{
			return false;
		}

		RestoredPhases.Add(Fact.Phase);
		RestoredSequences.Add(Fact.Sequence);
		MaximumSequence = FMath::Max(MaximumSequence, Fact.Sequence);

		const FAPSPlanetSurfaceTransitionSnapshot* ExistingSnapshot =
			SnapshotsByContext.Find(BuildSnapshotMapKey(
				Fact.BodyStableKey,
				Fact.LandingContextStableKey));
		FAPSPlanetSurfaceTransitionFact ExistingFact;
		if (ExistingSnapshot
			&& ExistingSnapshot->TryGetLatestCommittedFact(Fact.Phase, ExistingFact)
			&& (ExistingFact.Sequence > Fact.Sequence
				|| (ExistingFact.Sequence == Fact.Sequence
					&& !ExistingFact.IsExactlyEqual(Fact))))
		{
			return false;
		}
	}

	for (const FAPSPlanetSurfaceTransitionFact& Fact : Snapshot.LatestCommittedFacts)
	{
		const FString ContextKey = BuildSnapshotMapKey(
			Fact.BodyStableKey,
			Fact.LandingContextStableKey);
		FAPSPlanetSurfaceTransitionSnapshot& Target =
			SnapshotsByContext.FindOrAdd(ContextKey);
		Target.BodyStableKey = Fact.BodyStableKey;
		Target.LandingContextStableKey = Fact.LandingContextStableKey;

		StoreLatestFact(Target, Fact);
	}

	LastSequenceByBody.FindOrAdd(Snapshot.BodyStableKey) = MaximumSequence;
	return true;
}

bool UAPSPlanetSurfaceTransitionSubsystem::ValidateRequest(
	const FAPSPlanetSurfaceTransitionCommitRequest& Request)
{
	const uint8 PhaseValue = static_cast<uint8>(Request.Phase);
	if (Request.BodyStableKey.IsEmpty()
		|| PhaseValue > static_cast<uint8>(EAPSPlanetSurfaceTransitionPhase::Recovered)
		|| (Request.ReadinessBits & ~APSPlanetSurfaceTransition::KnownReadinessMask) != 0)
	{
		return false;
	}

	if (Request.Phase == EAPSPlanetSurfaceTransitionPhase::ArrivalCommitted)
	{
		return Request.LandingContextStableKey.IsEmpty();
	}

	if (!ValidateLandingContextStableKey(
		Request.BodyStableKey,
		Request.LandingContextStableKey))
	{
		return false;
	}

	return Request.Phase != EAPSPlanetSurfaceTransitionPhase::AnchorReady
		|| (Request.ReadinessBits & APSPlanetSurfaceTransition::AnchorReadyMask)
			== APSPlanetSurfaceTransition::AnchorReadyMask;
}

bool UAPSPlanetSurfaceTransitionSubsystem::ValidateLandingContextStableKey(
	const FString& BodyStableKey,
	const FString& LandingContextStableKey)
{
	if (BodyStableKey.IsEmpty() || LandingContextStableKey.IsEmpty())
	{
		return false;
	}

	const FString Prefix = BodyStableKey + TEXT("/EA-LANDING-");
	if (!LandingContextStableKey.StartsWith(Prefix, ESearchCase::CaseSensitive))
	{
		return false;
	}

	const FString PlacementKeyText = LandingContextStableKey.Mid(Prefix.Len());
	if (PlacementKeyText.IsEmpty())
	{
		return false;
	}

	for (const TCHAR Character : PlacementKeyText)
	{
		if (Character < TEXT('0') || Character > TEXT('9'))
		{
			return false;
		}
	}

	uint64 PlacementKey = 0;
	return LexTryParseString(PlacementKey, *PlacementKeyText)
		&& PlacementKey > 0
		&& PlacementKey <= static_cast<uint64>(MAX_uint32)
		&& LandingContextStableKey == BuildLandingContextStableKey(
			BodyStableKey,
			static_cast<int64>(PlacementKey));
}

FString UAPSPlanetSurfaceTransitionSubsystem::BuildStreamId(
	const FString& BodyStableKey)
{
	return FString::Printf(TEXT("APS.Surface/%s"), *BodyStableKey);
}

FString UAPSPlanetSurfaceTransitionSubsystem::BuildEventId(
	const FString& StreamId,
	const int64 Sequence)
{
	return FString::Printf(TEXT("%s/EVENT-%lld"), *StreamId, Sequence);
}

FString UAPSPlanetSurfaceTransitionSubsystem::BuildSnapshotMapKey(
	const FString& BodyStableKey,
	const FString& LandingContextStableKey)
{
	return FString::Printf(
		TEXT("%d:%s%d:%s"),
		BodyStableKey.Len(),
		*BodyStableKey,
		LandingContextStableKey.Len(),
		*LandingContextStableKey);
}

bool UAPSPlanetSurfaceTransitionSubsystem::MatchesRequestPayload(
	const FAPSPlanetSurfaceTransitionFact& Fact,
	const FAPSPlanetSurfaceTransitionCommitRequest& Request)
{
	return Fact.CorrelationId == Request.CorrelationId
		&& Fact.BodyStableKey == Request.BodyStableKey
		&& Fact.LandingContextStableKey == Request.LandingContextStableKey
		&& Fact.Phase == Request.Phase
		&& Fact.ReadinessBits == Request.ReadinessBits
		&& Fact.Reason == Request.Reason;
}

void UAPSPlanetSurfaceTransitionSubsystem::StoreLatestFact(
	FAPSPlanetSurfaceTransitionSnapshot& Snapshot,
	const FAPSPlanetSurfaceTransitionFact& Fact)
{
	for (FAPSPlanetSurfaceTransitionFact& ExistingFact : Snapshot.LatestCommittedFacts)
	{
		if (ExistingFact.Phase == Fact.Phase)
		{
			ExistingFact = Fact;
			return;
		}
	}

	Snapshot.LatestCommittedFacts.Add(Fact);
}
