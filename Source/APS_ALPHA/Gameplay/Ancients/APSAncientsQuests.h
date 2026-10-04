#pragma once

#include "CoreMinimal.h"
#include "APSAncientsTypes.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"

/**
 * The Builders' quest chains (Docs/Design/ANCIENT_STRUCTURES.md, "Quest chains"). Every step of a chain is a department
 * mission on the existing board (APSMissions): it shows in DIVISIONS and in the HUD tracker, pays its reward in the
 * civilization's stocks, levels and unlocks, and is saved with the board. The board has no add of its own, so a step goes
 * on it through the board's public save path (CaptureSave, append, RestoreSave), keeping the tracked mission.
 *
 * Each step's mission has a subject of its own (ANCIENT:<site>:<step>), so only this runtime's notification completes it:
 * the ancients runtime watches the step's conditions (the pilot, the fleet, the surveys) and notifies the board when one
 * is met. Chain progress after a load comes back from the board's completed templates; nothing else is saved.
 */
namespace APSAncientsQuests
{
	/** What completes a step; a step lists several and any one of them does. */
	enum ECondition : uint16
	{
		/** The pilot (on foot or in any vehicle) within NearKm of the site. */
		PilotNear = 1 << 0,
		/** Any ship of the fleet holding at the site (a MOVE order to it). */
		FleetHold = 1 << 1,
		/** A science ship holding at the site for HoldSeconds. */
		ScienceHold = 1 << 2,
		/** The pilot within LingerKm of the site for LingerSeconds. */
		PilotLinger = 1 << 3,
		/** The pilot on foot within the site's on-foot reach. */
		OnFoot = 1 << 4,
		/** An exploration ship holding at the site for ExpeditionSeconds: its crew goes down. */
		Expedition = 1 << 5,
		/** The fleet surveyed the site's world (never the home planet: it is known from the start). */
		BodySurveyed = 1 << 6,
		/** A science ship studied the site's world (never the home planet). */
		BodyStudied = 1 << 7,
		/** The site's star system is surveyed: a survey ship, or the pilot's visit. */
		SystemSurveyed = 1 << 8,
		/** The pilot's ship within DockKm of an orbital site for DockSeconds (on foot does not apply in orbit). */
		Dock = 1 << 9
	};

	struct FStep
	{
		FText Title;
		/** The department's ask; {Site} {Body} {Where} {System} {Size} {Km} {Seconds} are filled per site. */
		FText Brief;
		/** What the step found, posted to the journal when it completes; {Story} is the kind's last line. */
		FText Done;
		/** Shown on the board and the tracker; never one a count-of-any mission listens to (APSMissionsLocal::Matches). */
		APSMissions::EObjective Objective{APSMissions::EObjective::LocateAnomaly};
		uint16 Conditions{0};
		float NearKm{25.0f};
		float LingerKm{3.0f};
		float LingerSeconds{20.0f};
		float HoldSeconds{30.0f};
		float ExpeditionSeconds{60.0f};
		float DockKm{3.0f};
		float DockSeconds{10.0f};
		TArray<APSInfrastructure::FAmount> Reward;
		int32 Levels{0};
		/** An infrastructure type the step unlocks (APSInfrastructureCatalog), or null. */
		const TCHAR* Unlocks{nullptr};
	};

	struct FChain
	{
		APSAncients::EChain Id{APSAncients::EChain::Echoes};
		/** In the mission templates (Ancients.<Key>.<site>.<step>); never renamed, saved missions carry it. */
		const TCHAR* Key{TEXT("")};
		APSInfrastructure::EDepartment Department{APSInfrastructure::EDepartment::Science};
		/** Posted when the chain starts. */
		FText Signal;
		TArray<FStep> Steps;
	};

	const FChain& ChainOf(APSAncients::EChain Id);
	/** "Ancients.Echoes.H_MONUMENT.1": the step's template on the board (steps count from 1 there). */
	FName TemplateOf(const APSAncients::FSiteSpec& Spec, int32 Step);
	/** "ANCIENT:H_MONUMENT:1": the subject only this runtime notifies. */
	FString SubjectOf(const APSAncients::FSiteSpec& Spec, int32 Step);
	bool IsAncientTemplate(FName Template);

	/** The board as the chains read it, refreshed whenever its revision moves. */
	struct FBoardView
	{
		uint32 Revision{0};
		/** Every completed template of ours, also the ones the board no longer lists. */
		TSet<FName> Completed;
		/** Our missions still on the board (offered or active), by template. */
		TMap<FName, FAPSMission> Open;

		/** Re-reads the board when its revision moved; true when it did. */
		bool Refresh(const FAPSMissionBoard& Board);
	};

	/** The step's mission for a site, its texts filled from Args. */
	FAPSMission MakeMission(const APSAncients::FSiteSpec& Spec, int32 Step, const FFormatNamedArguments& Args, const FText& SubjectName,
		double NowSeconds, APSMissions::EState State);
	/** Puts a mission on the board; tracks it when nothing else is tracked (and it is active). */
	void Inject(FAPSMissionBoard& Board, const FAPSMission& Mission, bool bTrackIfFree);
	/** Changes our missions in place (rewards after a load, a dropped step offered again); false when nothing changed. */
	bool Edit(FAPSMissionBoard& Board, TFunctionRef<bool(FAPSMission&)> Change);
}
