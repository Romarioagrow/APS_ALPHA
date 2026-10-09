#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "APSAncientsQuests.h"
#include "APSAncientsTypes.h"

class AActor;
class AAstroGenerator;
class APawn;
class APlanetaryBody;
class FAPSMissionBoard;
class UWorld;
struct FAPSAncientShape;
struct FAPSObjectAction;

/**
 * The world's ancient sites (Rio 02.10; Docs/Design/ANCIENT_STRUCTURES.md). Plain C++, ticked with its own world as a
 * tickable game object (so it pauses with the game) and created for every game and PIE world by world delegates that
 * register themselves when the engine has started: no class, subsystem or shared file had to change for it.
 *
 * Four times a second it binds the sites to their worlds (the start system's at once, a nearby system's once that system
 * stands materialized), resolves each surface site's place when that world's WorldScape surface is loaded (the first of
 * a seeded list of candidates with dry, gentle ground under the whole footprint), builds one site per tick (procedural
 * stone and glow meshes sunk into the ground, attached to the body), and runs the quest chains on the mission board.
 * Console: aps.Ancients.List / Teleport / Advance / StartAll / Rebuild; log lines start with [APS.Ancients].
 */
class APS_ALPHA_API FAPSAncients final : public FTickableGameObject
{
public:
	struct FSite
	{
		enum class EStage : uint8
		{
			/** A nearby system's site before that system's worlds stand. */
			Unbound,
			/** Bound to its world; waits for the world's surface (or builds at once in orbit). */
			Waiting,
			/** Trying candidate places on the loaded surface, a few per tick. */
			Resolving,
			Built
		};

		APSAncients::FSiteSpec Spec;
		EStage Stage{EStage::Unbound};
		/** The planet or moon it stands on or orbits. */
		TWeakObjectPtr<AActor> Body;
		FText BodyName;
		FText SystemName;
		/** What the shape measures (from the seed alone before it is built, from the build after). */
		APSAncients::FMetrics Metrics;
		/** The resolved place: the centre's body-local direction, kept while the body stands so a rebuild skips the search. */
		bool bResolved{false};
		FVector LocalUp{FVector::UpVector};
		int32 Candidate{INDEX_NONE};
		int32 NextCandidate{0};
		int32 BestCandidate{INDEX_NONE};
		double BestScore{TNumericLimits<double>::Max()};
		TWeakObjectPtr<AActor> Actor;
		/** Nearby systems: the planets seen standing and since when, so the world is chosen once they all stand. */
		int32 SeenPlanets{INDEX_NONE};
		double PlanetsSince{0.0};

		/** Quest: the first step not completed (-1 before the chain started), and this session's view of it. */
		int32 Step{-1};
		bool bSynced{false};
		bool bStarted{false};
		bool bOpenSeen{false};
		bool bNotified{false};
		bool bForced{false};
		double ReofferAt{0.0};
		/** Steps of this chain the player dropped this session: each next offer waits longer. */
		int32 Drops{0};
		float ScienceSeconds{0.0f};
		float ExpeditionSeconds{0.0f};
		float LingerSeconds{0.0f};
		float DockSeconds{0.0f};
	};

	explicit FAPSAncients(UWorld* InWorld);
	virtual ~FAPSAncients() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	const TArray<FSite>& GetSites() const { return Sites; }
	const FSite* FindByActor(const AActor* Actor) const;
	/** The site's centre on the ground (in orbit: the hull's centre), world space; false before it is built. */
	bool GetCentre(const FSite& Site, FVector& OutCentre) const;

	/** Object actions (APSObjectActions provider): what the site is, and the science and expedition orders. */
	void GatherActions(const FSite& Site, AActor* Object, TArray<FAPSObjectAction>& OutActions) const;

	/**
	 * Rio 09.10 (the quest chains read as one story): the site's one name, as its marker, the navigation list, the HUD
	 * chip and the JOURNAL's chain page show it: "UNKNOWN STRUCTURE · RESO" until it is identified, then
	 * "THE SPIRE FOREST · RESO" (aps.Quests.SiteNames; 0: the former names).
	 */
	FText DisplayName(const FSite& Site) const;
	/** The texts' arguments for a step of the site's chain as the chain fills them (Step < 0: the chain's signal). */
	FFormatNamedArguments TextArgs(const FSite& Site, int32 Step) const;
	/** The site and its step (from 0) whose mission template this is; null for any other template. */
	const FSite* FindByTemplate(FName Template, int32& OutStep) const;

	/** Console. */
	void LogSites() const;
	bool Teleport(int32 Index);
	bool Advance(int32 Index);
	void StartAll();
	void Rebuild();
	/** A site by its index or (part of) its id; INDEX_NONE when none. */
	int32 FindIndex(const FString& Text) const;

private:
	AAstroGenerator* FindGenerator();
	void BuildHomeSites(AAstroGenerator& Generator);
	void ScanLostWorks(AAstroGenerator& Generator);
	void BuildNearbySites();
	FSite& AddSite(const APSAncients::FSiteSpec& Spec, AActor* Body);

	void UpdateSites();
	void BindNearby(FSite& Site);
	/** Tries the next few candidate places; true when it spent this tick's budget. */
	bool ResolveSome(FSite& Site);
	bool BuildSurface(FSite& Site);
	bool BuildOrbital(FSite& Site);
	AActor* SpawnSiteActor(FSite& Site, const FAPSAncientShape& Shape, const FVector& Centre, const FQuat& Rotation);
	void DestroyActor(FSite& Site);
	void NameSite(const FSite& Site) const;

	void UpdateQuests(float DeltaSeconds);
	/** A load brings our missions back without their rewards (the board fills rewards from its own templates). */
	void HydrateRewards(FAPSMissionBoard& Board);
	void UpdateChain(FSite& Site, FAPSMissionBoard& Board, float DeltaSeconds);
	bool StartConditionMet(const FSite& Site) const;
	bool StepMet(FSite& Site, const APSAncientsQuests::FStep& Step, float DeltaSeconds);
	int32 CompletedSteps(const FSite& Site) const;
	bool IsChainDone(APSAncients::EChain Chain, const FString& SiteId = FString()) const;
	void OnStepsCompleted(FSite& Site, int32 From, int32 To);
	void StartChain(FSite& Site, FAPSMissionBoard& Board);
	void PutStep(FSite& Site, FAPSMissionBoard& Board, APSMissions::EState State);
	FFormatNamedArguments ArgsFor(const FSite& Site, const APSAncientsQuests::FStep* Step) const;
	FText SubjectNameFor(const FSite& Site, int32 Step) const;
	/** Whether the site is identified while Step is its current step (the chains name a site from its first survey). */
	bool IsKnownAt(const FSite& Site, int32 Step) const;
	void Post(const FText& Text) const;
	bool QuestsAllowed() const;
	double Now() const;
	APawn* Pilot() const;

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAstroGenerator> Generator;
	TArray<FSite> Sites;
	TSet<FString> RolledBodies;
	int32 WorldSeed{0};
	bool bHomeBuilt{false};
	bool bNearbyBuilt{false};
	bool bStartAll{false};
	FName HomeNameSeen;
	double HomeNameSince{0.0};
	float LogicClock{0.0f};
	float LostClock{0.0f};
	/** Quests: open once the load (if any) has been applied; the board as last read. */
	bool bQuestsOpen{false};
	double QuestsOpenedAt{0.0};
	APSAncientsQuests::FBoardView BoardView;
	bool bHydrated{false};
};

/** The world's ancient sites, or null (the menu, the authored map, a world before it starts). */
APS_ALPHA_API FAPSAncients* APSAncientsFind(const UWorld* World);
