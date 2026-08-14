#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/World/APSPlanetSurfaceTransitionSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceTransitionImmutableReconciliationTest,
	"APS.Surface.Transition.ImmutableReconciliationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceTransitionImmutableReconciliationTest::RunTest(
	const FString& Parameters)
{
	UAPSPlanetSurfaceTransitionSubsystem* Authority =
		NewObject<UAPSPlanetSurfaceTransitionSubsystem>();
	if (!TestNotNull(TEXT("Surface transition authority"), Authority))
	{
		return false;
	}

	const FString BodyStableKey(TEXT("SYS0/S0/P0"));
	const int64 PlacementKey = 123456789;
	const FString LandingContextStableKey =
		UAPSPlanetSurfaceTransitionSubsystem::BuildLandingContextStableKey(
			BodyStableKey,
			PlacementKey);
	TestEqual(
		TEXT("Landing context is the exact authoritative placement identity"),
		LandingContextStableKey,
		FString(TEXT("SYS0/S0/P0/EA-LANDING-123456789")));
	TestTrue(
		TEXT("Zero placement key is never converted to a landing identity"),
		UAPSPlanetSurfaceTransitionSubsystem::BuildLandingContextStableKey(
			BodyStableKey,
			0).IsEmpty());

	int32 BroadcastCount = 0;
	TArray<FAPSPlanetSurfaceTransitionFact> BroadcastFacts;
	Authority->OnTransitionCommitted().AddLambda(
		[&BroadcastCount, &BroadcastFacts](
			const FAPSPlanetSurfaceTransitionFact& Fact)
		{
			++BroadcastCount;
			BroadcastFacts.Add(Fact);
		});

	FAPSPlanetSurfaceTransitionCommitRequest ArrivalRequest;
	ArrivalRequest.BodyStableKey = BodyStableKey;
	ArrivalRequest.Phase = EAPSPlanetSurfaceTransitionPhase::ArrivalCommitted;
	ArrivalRequest.CorrelationId = TEXT("NAV-CMD-42");
	ArrivalRequest.Reason = TEXT("Navigation handoff committed");
	FAPSPlanetSurfaceTransitionFact ArrivalFact;
	TestTrue(TEXT("Arrival commits at body scope"),
		Authority->CommitTransition(ArrivalRequest, ArrivalFact));
	TestEqual(TEXT("Arrival is the first body sequence"), ArrivalFact.Sequence, int64{1});
	TestTrue(TEXT("Arrival has no landing context"),
		ArrivalFact.LandingContextStableKey.IsEmpty());
	TestEqual(TEXT("Owner propagates correlation byte-identically"),
		ArrivalFact.CorrelationId, ArrivalRequest.CorrelationId);

	FAPSPlanetSurfaceTransitionCommitRequest LandingRequest;
	LandingRequest.BodyStableKey = BodyStableKey;
	LandingRequest.LandingContextStableKey = LandingContextStableKey;
	LandingRequest.Phase = EAPSPlanetSurfaceTransitionPhase::LandingCommitted;
	LandingRequest.CorrelationId = ArrivalRequest.CorrelationId;
	LandingRequest.Reason = TEXT("Deterministic landing accepted");
	FAPSPlanetSurfaceTransitionFact LandingFact;
	TestTrue(TEXT("Landing commits against the owner landing key"),
		Authority->CommitTransition(LandingRequest, LandingFact));
	TestEqual(TEXT("Landing advances the body sequence"), LandingFact.Sequence, int64{2});

	FAPSPlanetSurfaceTransitionFact DuplicateLandingFact;
	TestTrue(TEXT("An exact repeated commit is idempotent"),
		Authority->CommitTransition(LandingRequest, DuplicateLandingFact));
	TestTrue(TEXT("Idempotent commit returns the identical owner fact"),
		DuplicateLandingFact.IsExactlyEqual(LandingFact));
	TestEqual(TEXT("Idempotent commit does not multicast again"), BroadcastCount, 2);

	FAPSPlanetSurfaceTransitionCommitRequest InvalidAnchorRequest = LandingRequest;
	InvalidAnchorRequest.Phase = EAPSPlanetSurfaceTransitionPhase::AnchorReady;
	InvalidAnchorRequest.ReadinessBits =
		static_cast<int32>(EAPSPlanetSurfaceReadiness::Profile)
		| static_cast<int32>(EAPSPlanetSurfaceReadiness::LOD0);
	FAPSPlanetSurfaceTransitionFact RejectedFact;
	TestFalse(TEXT("AnchorReady rejects missing collision readiness"),
		Authority->CommitTransition(InvalidAnchorRequest, RejectedFact));
	TestEqual(TEXT("Rejected readiness does not multicast"), BroadcastCount, 2);

	FAPSPlanetSurfaceTransitionCommitRequest AnchorRequest = InvalidAnchorRequest;
	AnchorRequest.ReadinessBits |=
		static_cast<int32>(EAPSPlanetSurfaceReadiness::Collision);
	AnchorRequest.Reason = TEXT("Profile, LOD0 and collision latched");
	FAPSPlanetSurfaceTransitionFact AnchorFact;
	TestTrue(TEXT("AnchorReady accepts all authoritative gates"),
		Authority->CommitTransition(AnchorRequest, AnchorFact));
	TestEqual(TEXT("AnchorReady sequence"), AnchorFact.Sequence, int64{3});

	auto CommitContextPhase = [this, Authority, &LandingRequest](
		const EAPSPlanetSurfaceTransitionPhase Phase,
		const TCHAR* Reason,
		FAPSPlanetSurfaceTransitionFact& OutFact)
	{
		FAPSPlanetSurfaceTransitionCommitRequest Request = LandingRequest;
		Request.Phase = Phase;
		Request.Reason = Reason;
		return Authority->CommitTransition(Request, OutFact);
	};

	FAPSPlanetSurfaceTransitionFact EnteredFact;
	FAPSPlanetSurfaceTransitionFact ExitedFact;
	FAPSPlanetSurfaceTransitionFact ReenteredFact;
	TestTrue(TEXT("Entered commits"), CommitContextPhase(
		EAPSPlanetSurfaceTransitionPhase::Entered,
		TEXT("Character entered surface gameplay"),
		EnteredFact));
	TestTrue(TEXT("Exited commits"), CommitContextPhase(
		EAPSPlanetSurfaceTransitionPhase::Exited,
		TEXT("Character exited surface gameplay"),
		ExitedFact));
	TestTrue(TEXT("Repeated cyclic phase creates a newer owner fact"),
		CommitContextPhase(
			EAPSPlanetSurfaceTransitionPhase::Entered,
			TEXT("Character re-entered after orbit"),
			ReenteredFact));
	TestEqual(TEXT("Re-entered sequence advances monotonically"),
		ReenteredFact.Sequence, int64{6});
	TestNotEqual(TEXT("Re-entered phase receives a new EventId"),
		ReenteredFact.EventId, EnteredFact.EventId);

	FAPSPlanetSurfaceTransitionSnapshot Snapshot;
	TestTrue(TEXT("Late read returns a stateful context snapshot"),
		Authority->GetTransitionSnapshot(
			BodyStableKey,
			LandingContextStableKey,
			Snapshot));
	TestEqual(TEXT("Snapshot retains one latest fact per achieved phase"),
		Snapshot.LatestCommittedFacts.Num(), 5);

	FAPSPlanetSurfaceTransitionFact QueriedFact;
	TestTrue(TEXT("Context snapshot includes body-level ArrivalCommitted"),
		Snapshot.TryGetLatestCommittedFact(
			EAPSPlanetSurfaceTransitionPhase::ArrivalCommitted,
			QueriedFact));
	TestTrue(TEXT("Arrival late-read identity and payload are byte-identical"),
		QueriedFact.IsExactlyEqual(ArrivalFact));
	TestTrue(TEXT("Snapshot retains the newest Entered fact"),
		Snapshot.TryGetLatestCommittedFact(
			EAPSPlanetSurfaceTransitionPhase::Entered,
			QueriedFact));
	TestTrue(TEXT("Newest Entered envelope is unchanged"),
		QueriedFact.IsExactlyEqual(ReenteredFact));
	TestTrue(TEXT("A later Entered commit does not erase Exited"),
		Snapshot.TryGetLatestCommittedFact(
			EAPSPlanetSurfaceTransitionPhase::Exited,
			QueriedFact));
	TestTrue(TEXT("Exited envelope is unchanged"),
		QueriedFact.IsExactlyEqual(ExitedFact));

	FAPSPlanetSurfaceTransitionFact DirectAnchorFact;
	TestTrue(TEXT("One-phase query is side-effect-free and available"),
		Authority->TryGetLatestCommittedFact(
			BodyStableKey,
			LandingContextStableKey,
			EAPSPlanetSurfaceTransitionPhase::AnchorReady,
			DirectAnchorFact));
	TestTrue(TEXT("One-phase query returns the committed envelope"),
		DirectAnchorFact.IsExactlyEqual(AnchorFact));
	TestEqual(TEXT("Queries never emit history"), BroadcastCount, 6);

	UAPSPlanetSurfaceTransitionSubsystem* RestoredAuthority =
		NewObject<UAPSPlanetSurfaceTransitionSubsystem>();
	int32 RestoreBroadcastCount = 0;
	RestoredAuthority->OnTransitionCommitted().AddLambda(
		[&RestoreBroadcastCount](const FAPSPlanetSurfaceTransitionFact&)
		{
			++RestoreBroadcastCount;
		});
	TestTrue(TEXT("Owner snapshot restores after save/load"),
		RestoredAuthority->RestoreTransitionSnapshot(Snapshot));
	TestEqual(TEXT("Restore never replays multicast history"), RestoreBroadcastCount, 0);

	FAPSPlanetSurfaceTransitionSnapshot RestoredSnapshot;
	TestTrue(TEXT("Restored latest state is immediately readable"),
		RestoredAuthority->GetTransitionSnapshot(
			BodyStableKey,
			LandingContextStableKey,
			RestoredSnapshot));
	TestEqual(TEXT("Restore retains every achieved phase"),
		RestoredSnapshot.LatestCommittedFacts.Num(),
		Snapshot.LatestCommittedFacts.Num());
	for (const FAPSPlanetSurfaceTransitionFact& ExpectedFact
		: Snapshot.LatestCommittedFacts)
	{
		FAPSPlanetSurfaceTransitionFact ActualFact;
		TestTrue(
			*FString::Printf(TEXT("Restored phase %d is readable"),
				static_cast<int32>(ExpectedFact.Phase)),
			RestoredSnapshot.TryGetLatestCommittedFact(ExpectedFact.Phase, ActualFact));
		TestTrue(
			*FString::Printf(TEXT("Restored phase %d remains byte-identical"),
				static_cast<int32>(ExpectedFact.Phase)),
			ActualFact.IsExactlyEqual(ExpectedFact));
	}

	FAPSPlanetSurfaceTransitionCommitRequest RecoveredRequest = LandingRequest;
	RecoveredRequest.Phase = EAPSPlanetSurfaceTransitionPhase::Recovered;
	RecoveredRequest.Reason = TEXT("Save recovery reconciled");
	FAPSPlanetSurfaceTransitionFact RecoveredFact;
	TestTrue(TEXT("New commit succeeds after restore"),
		RestoredAuthority->CommitTransition(RecoveredRequest, RecoveredFact));
	TestEqual(TEXT("Sequence resumes after the restored maximum"),
		RecoveredFact.Sequence, int64{7});
	TestEqual(TEXT("Only the new post-restore fact multicasts"),
		RestoreBroadcastCount, 1);

	FAPSPlanetSurfaceTransitionCommitRequest InvalidContextRequest = LandingRequest;
	InvalidContextRequest.LandingContextStableKey =
		TEXT("SYS0/S0/P1/EA-LANDING-123456789");
	TestFalse(TEXT("Mismatched body/context identity is rejected"),
		Authority->CommitTransition(InvalidContextRequest, RejectedFact));
	InvalidContextRequest.LandingContextStableKey.Reset();
	TestFalse(TEXT("Non-arrival phases require an owner landing context"),
		Authority->CommitTransition(InvalidContextRequest, RejectedFact));

	return true;
}

#endif
