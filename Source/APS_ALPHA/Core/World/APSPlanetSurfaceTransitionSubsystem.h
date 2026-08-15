#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "APSPlanetSurfaceTransitionSubsystem.generated.h"

/** Authoritative phases emitted by the Surface owner. */
UENUM(BlueprintType)
enum class EAPSPlanetSurfaceTransitionPhase : uint8
{
	ArrivalCommitted,
	LandingCommitted,
	AnchorReady,
	Entered,
	Exited,
	Recovered
};

/** Readiness evidence carried by a committed Surface transition. */
UENUM(BlueprintType, meta = (Bitflags))
enum class EAPSPlanetSurfaceReadiness : uint8
{
	None = 0,
	Profile = 1 << 0,
	LOD0 = 1 << 1,
	Collision = 1 << 2
};
ENUM_CLASS_FLAGS(EAPSPlanetSurfaceReadiness);

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSPlanetSurfaceTransitionCommitRequest
{
	GENERATED_BODY()

	/** Canonical generated-world body key, for example SYS0/S0/P0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Planet Surface|Transition")
	FString BodyStableKey;

	/**
	 * Empty for ArrivalCommitted. Every later phase uses the exact Surface-owned
	 * <BodyStableKey>/EA-LANDING-<PlacementKey> value without caller rewriting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Planet Surface|Transition")
	FString LandingContextStableKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Planet Surface|Transition")
	EAPSPlanetSurfaceTransitionPhase Phase{EAPSPlanetSurfaceTransitionPhase::ArrivalCommitted};

	/** Optional authoritative command correlation propagated unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Planet Surface|Transition")
	FString CorrelationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Planet Surface|Transition")
	FString Reason;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Planet Surface|Transition",
		meta = (Bitmask, BitmaskEnum = "/Script/APS_ALPHA.EAPSPlanetSurfaceReadiness"))
	int32 ReadinessBits{0};
};

/** Immutable identity and payload assigned by the Surface owner at commit time. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSPlanetSurfaceTransitionFact
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString StreamId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString CorrelationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	int64 Sequence{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString BodyStableKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString LandingContextStableKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	EAPSPlanetSurfaceTransitionPhase Phase{EAPSPlanetSurfaceTransitionPhase::ArrivalCommitted};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition",
		meta = (Bitmask, BitmaskEnum = "/Script/APS_ALPHA.EAPSPlanetSurfaceReadiness"))
	int32 ReadinessBits{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString Reason;

	bool IsExactlyEqual(const FAPSPlanetSurfaceTransitionFact& Other) const;
};

/**
 * Stateful late-read view. It retains the latest immutable fact for each achieved
 * phase instead of collapsing the route to a single event.
 */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSPlanetSurfaceTransitionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString BodyStableKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	FString LandingContextStableKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Planet Surface|Transition")
	TArray<FAPSPlanetSurfaceTransitionFact> LatestCommittedFacts;

	bool TryGetLatestCommittedFact(
		EAPSPlanetSurfaceTransitionPhase Phase,
		FAPSPlanetSurfaceTransitionFact& OutFact) const;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnAPSPlanetSurfaceTransitionCommitted,
	const FAPSPlanetSurfaceTransitionFact&);

/**
 * Surface-owned transition authority. Consumers subscribe first and then query the
 * stateful snapshot; reads never consume a cursor and restore never replays history.
 */
UCLASS()
class APS_ALPHA_API UAPSPlanetSurfaceTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 PlacementContractVersion = 1;

	/** Builds the byte-stable Quest/Surface landing identity from the placement key. */
	UFUNCTION(BlueprintPure, Category = "APS|Planet Surface|Transition")
	static FString BuildLandingContextStableKey(
		const FString& BodyStableKey,
		int64 PlacementKey);

	/**
	 * Commits a new owner fact and broadcasts it once. An exact duplicate returns the
	 * existing fact without incrementing Sequence or broadcasting again.
	 */
	bool CommitTransition(
		const FAPSPlanetSurfaceTransitionCommitRequest& Request,
		FAPSPlanetSurfaceTransitionFact& OutFact);

	/** Side-effect-free, non-consuming per-phase reconciliation query. */
	bool GetTransitionSnapshot(
		const FString& BodyStableKey,
		const FString& LandingContextStableKey,
		FAPSPlanetSurfaceTransitionSnapshot& OutSnapshot) const;

	/** Side-effect-free query for one committed phase. */
	bool TryGetLatestCommittedFact(
		const FString& BodyStableKey,
		const FString& LandingContextStableKey,
		EAPSPlanetSurfaceTransitionPhase Phase,
		FAPSPlanetSurfaceTransitionFact& OutFact) const;

	/**
	 * Restores owner-authored facts without emitting multicast history. Identity fields
	 * are validated exactly and the next owner sequence advances past restored facts.
	 */
	bool RestoreTransitionSnapshot(const FAPSPlanetSurfaceTransitionSnapshot& Snapshot);

	FOnAPSPlanetSurfaceTransitionCommitted& OnTransitionCommitted()
	{
		return TransitionCommitted;
	}

private:
	static bool ValidateRequest(const FAPSPlanetSurfaceTransitionCommitRequest& Request);
	static bool ValidateLandingContextStableKey(
		const FString& BodyStableKey,
		const FString& LandingContextStableKey);
	static FString BuildStreamId(const FString& BodyStableKey);
	static FString BuildEventId(const FString& StreamId, int64 Sequence);
	static FString BuildSnapshotMapKey(
		const FString& BodyStableKey,
		const FString& LandingContextStableKey);
	static bool MatchesRequestPayload(
		const FAPSPlanetSurfaceTransitionFact& Fact,
		const FAPSPlanetSurfaceTransitionCommitRequest& Request);
	static void StoreLatestFact(
		FAPSPlanetSurfaceTransitionSnapshot& Snapshot,
		const FAPSPlanetSurfaceTransitionFact& Fact);

	TMap<FString, FAPSPlanetSurfaceTransitionSnapshot> SnapshotsByContext;
	TMap<FString, int64> LastSequenceByBody;
	FOnAPSPlanetSurfaceTransitionCommitted TransitionCommitted;
};
