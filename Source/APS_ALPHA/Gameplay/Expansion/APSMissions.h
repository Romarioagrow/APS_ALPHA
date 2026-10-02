#pragma once

#include "CoreMinimal.h"
#include "APSInfrastructureCatalog.h"

class UWorld;

/**
 * Department missions (Rio 02.10: "each department has its own missions, and varied ones: not only bring something,
 * but check, do, solve, find, install, fly, learn; they come from the departments as options"). Every division keeps
 * a few offers drawn from templates that fit what the civilization knows and holds; the player accepts one, the
 * runtime's events (surveys, scans, visits, builds, anomalies, launches, stocks) advance it, and completing it pays
 * the department: stocks, earned levels, an unlocked structure type, or the next step of a chain.
 *
 * Plain C++ owned by UAPSFleetCommandSubsystem; reached through APSMissionsFind(World).
 */
namespace APSMissions
{
	/** What a mission asks. A Subject narrows it (a body key, a star system id, a structure type, a resource). */
	enum class EObjective : uint8
	{
		SurveyWorld,
		StudyWorld,
		ScanSystem,
		SurveySystem,
		ClaimSystem,
		VisitSystem,
		LandOnWorld,
		BuildStructure,
		LocateAnomaly,
		InvestigateAnomaly,
		LaunchShip,
		ReachStock,
		LinkSystems,
		Count
	};

	enum class EState : uint8
	{
		Offered,
		Active,
		Completed,
		Expired
	};

	APS_ALPHA_API FText ObjectiveName(EObjective Objective);
	APS_ALPHA_API FText StateName(EState State);
}

struct APS_ALPHA_API FAPSMission
{
	FGuid Id;
	APSInfrastructure::EDepartment Department{APSInfrastructure::EDepartment::Exploration};
	/** The template it came from (chains and repeats refer to it). */
	FName Template;
	FText Title;
	/** The department's ask, in its own voice. */
	FText Brief;
	APSMissions::EObjective Objective{APSMissions::EObjective::SurveyWorld};
	/** What exactly: a body key, a star system id (digits), a structure type, a resource name; empty for any. */
	FString Subject;
	FText SubjectName;
	int32 Count{1};
	int32 Progress{0};
	TArray<APSInfrastructure::FAmount> Reward;
	/** Earned levels for the department on completion. */
	int32 RewardLevels{0};
	/** A structure type this unlocks (NAME_None: none). */
	FName Unlocks;
	/** The chain's next template, offered on completion (NAME_None: none). */
	FName Next;
	APSMissions::EState State{APSMissions::EState::Offered};
	double OfferedSeconds{0.0};
};

struct APS_ALPHA_API FAPSMissionSaveData
{
	TArray<FAPSMission> Missions;
	TArray<FName> Unlocked;
	/** Earned levels per department (EDepartment order). */
	TArray<int32> EarnedLevels;
	TArray<FName> CompletedTemplates;

	friend FArchive& operator<<(FArchive& Ar, FAPSMissionSaveData& Data);
};

class APS_ALPHA_API FAPSMissionBoard
{
public:
	explicit FAPSMissionBoard(UWorld* InWorld);
	~FAPSMissionBoard();

	/** Keeps a few offers per department from the templates that fit the civilization's state; expires old offers. */
	void Tick(float DeltaSeconds);

	const TArray<FAPSMission>& GetMissions() const { return Missions; }
	void GetFor(APSInfrastructure::EDepartment Department, TArray<const FAPSMission*>& OutMissions) const;
	const FAPSMission* Find(const FGuid& Id) const;
	bool Accept(const FGuid& Id);
	bool Decline(const FGuid& Id);
	/** The mission the HUD follows (the last accepted), or null. */
	const FAPSMission* GetTracked() const;
	void SetTracked(const FGuid& Id);

	/** Events from the runtime advance every active mission whose objective (and subject, when set) matches. */
	void Notify(APSMissions::EObjective Objective, const FString& Subject, int32 Amount = 1);

	/** Structure types unlocked by missions (catalogue types marked as needing it). */
	bool IsUnlocked(FName Type) const { return Unlocked.Contains(Type); }
	/** Levels the department earned through missions (added to its level like work growth). */
	int32 GetEarnedLevels(APSInfrastructure::EDepartment Department) const;

	uint32 GetRevision() const { return Revision; }
	void CaptureSave(FAPSMissionSaveData& OutData) const;
	void RestoreSave(FAPSMissionSaveData&& Data);

private:
	TWeakObjectPtr<UWorld> World;
	TArray<FAPSMission> Missions;
	TSet<FName> Unlocked;
	TArray<FName> CompletedTemplates;
	int32 EarnedLevels[static_cast<int32>(APSInfrastructure::EDepartment::Count)]{};
	FGuid Tracked;
	float OfferClock{0.0f};
	uint32 Revision{1};
};

APS_ALPHA_API FAPSMissionBoard* APSMissionsFind(const UWorld* World);
APS_ALPHA_API void APSMissionsRegister(const UWorld* World, FAPSMissionBoard* Board);
/** Safe from anywhere: forwards an event to the world's board when there is one. */
APS_ALPHA_API void APSMissionsNotify(const UWorld* World, APSMissions::EObjective Objective, const FString& Subject,
	int32 Amount = 1);
