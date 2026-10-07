#include "APSAncientsQuests.h"

#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"

#define LOCTEXT_NAMESPACE "APSAncients"

namespace APSAncientsQuestsLocal
{
	using namespace APSAncientsQuests;
	using APSAncients::EChain;
	using APSInfrastructure::EDepartment;
	using APSInfrastructure::EResource;
	using APSInfrastructure::FAmount;
	using APSMissions::EObjective;

	const FString TemplatePrefix(TEXT("Ancients."));

	FAmount Metals(const float Value) { return {EResource::Metals, Value}; }
	FAmount Volatiles(const float Value) { return {EResource::Volatiles, Value}; }
	FAmount Energy(const float Value) { return {EResource::Energy, Value}; }
	FAmount Research(const float Value) { return {EResource::Research, Value}; }
	FAmount Influence(const float Value) { return {EResource::Influence, Value}; }

	FStep MakeStep(const FText& Title, const FText& Brief, const FText& Done, const EObjective Objective, const uint16 Conditions,
		const TArray<FAmount>& Reward, const int32 Levels = 0, const TCHAR* Unlocks = nullptr)
	{
		FStep Step;
		Step.Title = Title;
		Step.Brief = Brief;
		Step.Done = Done;
		Step.Objective = Objective;
		Step.Conditions = Conditions;
		Step.Reward = Reward;
		Step.Levels = Levels;
		Step.Unlocks = Unlocks;
		return Step;
	}

	/**
	 * The chains' texts and rules. Rewards follow the department missions' scale (APSMissions: 20-120 a step); each chain's
	 * last step pays a level and unlocks one of the megastructures that otherwise need a mission (APSInfrastructureCatalog).
	 */
	TArray<FChain> BuildChains()
	{
		TArray<FChain> Chains;
		Chains.SetNum(static_cast<int32>(EChain::Count));

		// The monument on the home planet (Science).
		{
			FChain& Chain = Chains[static_cast<int32>(EChain::Echoes)];
			Chain.Id = EChain::Echoes;
			Chain.Key = TEXT("Echoes");
			Chain.Department = EDepartment::Science;
			Chain.Signal = LOCTEXT("EchoesSignal",
				"SIGNAL — {Body}: the colony's deep radar draws {Size} of straight edges at {Where}, too regular for rock. The colony map marks it as an anomaly site (UNKNOWN STRUCTURE): set a course there.");
			FStep& Locate = Chain.Steps.Add_GetRef(MakeStep(LOCTEXT("EchoesLocate", "A SHAPE ON THE HORIZON"),
				LOCTEXT("EchoesLocateBrief", "Something built stands on {Body} at {Where}. Fly within {NearKm} km of it, or send any ship to the marked site."),
				LOCTEXT("EchoesLocateDone", "{Site} on {Body}: {Description} No record of ours mentions it. Its faces carry glyphs."),
				EObjective::LocateAnomaly, PilotNear | FleetHold, {Research(20)}));
			Locate.NearKm = 25.0f;
			FStep& Study = Chain.Steps.Add_GetRef(MakeStep(LOCTEXT("EchoesStudy", "READ THE STONE"),
				LOCTEXT("EchoesStudyBrief", "A science ship holding over {Site} for {HoldSeconds} s reads its glyphs; or stay within {LingerKm} km of it yourself for {LingerSeconds} s."),
				LOCTEXT("EchoesStudyDone", "The glyphs of {Site} count in sevens. Beside a map of this system one sign repeats: the Builders knew our sun."),
				EObjective::StudyWorld, ScienceHold | PilotLinger, {Research(60)}));
			Study.LingerKm = 3.0f;
			Chain.Steps.Add(MakeStep(LOCTEXT("EchoesContact", "AT THE FOOT OF IT"),
				LOCTEXT("EchoesContactBrief", "{Reach}, or send an expedition: an exploration ship holding over it for {ExpeditionSeconds} s puts a crew down."),
				LOCTEXT("EchoesContactDone", "{Story} What the Builders knew of lifting stone to orbit is ours now: SPACE ELEVATOR unlocked."),
				EObjective::InvestigateAnomaly, OnFoot | Expedition | Dock, {Research(80), Influence(40)}, 1, TEXT("SpaceElevator")));
		}

		// The derelict in the home planet's high orbit (Industry).
		{
			FChain& Chain = Chains[static_cast<int32>(EChain::QuietHull)];
			Chain.Id = EChain::QuietHull;
			Chain.Key = TEXT("QuietHull");
			Chain.Department = EDepartment::Industry;
			Chain.Signal = LOCTEXT("HullSignal",
				"SIGNAL — long-range optics caught a hull in high orbit of {Body}: {Size} long, no transponder, no heat. The colony map marks it as an anomaly site.");
			FStep& Locate = Chain.Steps.Add_GetRef(MakeStep(LOCTEXT("HullLocate", "A COLD RETURN"),
				LOCTEXT("HullLocateBrief", "Fly within {NearKm} km of the hull over {Body}, or send any ship to it."),
				LOCTEXT("HullLocateDone", "{Site}: {Description} Its alloy matches no ore in this system."),
				EObjective::LocateAnomaly, PilotNear | FleetHold, {Metals(40)}));
			Locate.NearKm = 50.0f;
			FStep& Scan = Chain.Steps.Add_GetRef(MakeStep(LOCTEXT("HullScan", "SCAN THE HULL"),
				LOCTEXT("HullScanBrief", "A science ship holding by {Site} for {HoldSeconds} s scans it through; or stay within {LingerKm} km of it yourself for {LingerSeconds} s."),
				LOCTEXT("HullScanDone", "The hull was grown, not welded: one crystal of metal. Its rings were segments of something far larger."),
				EObjective::StudyWorld, ScienceHold | PilotLinger, {Research(40)}));
			Scan.LingerKm = 5.0f;
			Chain.Steps.Add(MakeStep(LOCTEXT("HullBoard", "BOARD IT"),
				LOCTEXT("HullBoardBrief", "{Reach} while the crew goes over, or send an expedition: an exploration ship holding by it for {ExpeditionSeconds} s."),
				LOCTEXT("HullBoardDone", "{Story} The salvage crews strip its vaults of alloy and fuel, and its ring segments teach the yards to close a ring round a world: ORBITAL RING unlocked."),
				EObjective::InvestigateAnomaly, Dock | Expedition, {Metals(200), Volatiles(80), Energy(40)}, 1, TEXT("OrbitalRing")));
		}

		// The small circle on a home moon (Exploration).
		{
			FChain& Chain = Chains[static_cast<int32>(EChain::Circle)];
			Chain.Id = EChain::Circle;
			Chain.Key = TEXT("Circle");
			Chain.Department = EDepartment::Exploration;
			Chain.Signal = LOCTEXT("CircleSignal",
				"The Builders' glyphs point to a smaller work on {Body}: a circle of standing stones, small enough to walk into. The colony map marks it once {Body}'s surface is in reach.");
			Chain.Steps.Add(MakeStep(LOCTEXT("CircleContact", "STONES IN A RING"),
				LOCTEXT("CircleContactBrief", "{Reach}, send an expedition (an exploration ship holding over it for {ExpeditionSeconds} s), or have a science ship study {Body}."),
				LOCTEXT("CircleContactDone", "{Story}"),
				EObjective::InvestigateAnomaly, OnFoot | Expedition | BodyStudied, {Research(40), Influence(20)}));
		}

		// A chance site on another world of the start system (Exploration).
		{
			FChain& Chain = Chains[static_cast<int32>(EChain::LostWorks)];
			Chain.Id = EChain::LostWorks;
			Chain.Key = TEXT("LostWorks");
			Chain.Department = EDepartment::Exploration;
			Chain.Signal = LOCTEXT("LostSignal",
				"Something built stands on {Body}: {Size} of it. The colony map marks it once {Body}'s surface is in reach.");
			Chain.Steps.Add(MakeStep(LOCTEXT("LostLocate", "LOST WORKS"),
				LOCTEXT("LostLocateBrief", "Fly within {NearKm} km of {Site} on {Body}, send any ship to it, or survey {Body}."),
				LOCTEXT("LostLocateDone", "{Site} on {Body}: {Description}"),
				EObjective::LocateAnomaly, PilotNear | FleetHold | BodySurveyed, {Research(20)}));
			Chain.Steps.Add(MakeStep(LOCTEXT("LostContact", "WHAT STANDS THERE"),
				LOCTEXT("LostContactBrief", "{Reach}, send an expedition (an exploration ship holding over it for {ExpeditionSeconds} s), or have a science ship study {Body}."),
				LOCTEXT("LostContactDone", "{Story}"),
				EObjective::InvestigateAnomaly, OnFoot | Expedition | BodyStudied | ScienceHold | Dock, {Research(50), Metals(50)}));
		}

		// A site in a nearby star system, along the Builders' charts (Exploration).
		{
			FChain& Chain = Chains[static_cast<int32>(EChain::Road)];
			Chain.Id = EChain::Road;
			Chain.Key = TEXT("Road");
			Chain.Department = EDepartment::Exploration;
			Chain.Signal = LOCTEXT("RoadSignal",
				"THE BUILDERS' CHART — one of their glyphs circles {System}. The star is marked on every map now.");
			Chain.Steps.Add(MakeStep(LOCTEXT("RoadChart", "THE STAR ON THE CHART"),
				LOCTEXT("RoadChartBrief", "Chart {System}: a probe and a survey of the system, or fly there yourself and stay a few seconds inside it."),
				LOCTEXT("RoadChartDone", "{System} is charted. One of its worlds answers our ping the way the Builders' stone did."),
				EObjective::SurveySystem, SystemSurveyed, {Research(30), Influence(10)}));
			Chain.Steps.Add(MakeStep(LOCTEXT("RoadLocate", "THE NEXT SITE"),
				LOCTEXT("RoadLocateBrief", "Find the Builders' site in {System}: fly within {NearKm} km of it, send any ship to it, or survey the world it stands on."),
				LOCTEXT("RoadLocateDone", "{Site} on {Body}: {Description}"),
				EObjective::LocateAnomaly, PilotNear | FleetHold | BodySurveyed, {Research(40)}));
			Chain.Steps.Add(MakeStep(LOCTEXT("RoadContact", "WHAT THEY LEFT"),
				LOCTEXT("RoadContactBrief", "{Reach}, send an expedition (an exploration ship holding over it for {ExpeditionSeconds} s), or have a science ship study {Body}."),
				LOCTEXT("RoadContactDone", "{Story} Its makers walked between the stars through gates: JUMP GATE unlocked. {Next}"),
				EObjective::InvestigateAnomaly, OnFoot | Expedition | BodyStudied | ScienceHold | Dock, {Research(120), Influence(40)}, 1,
				TEXT("JumpGate")));
		}
		return Chains;
	}
}

const APSAncientsQuests::FChain& APSAncientsQuests::ChainOf(const APSAncients::EChain Id)
{
	static const TArray<FChain> Chains = APSAncientsQuestsLocal::BuildChains();
	return Chains[FMath::Clamp(static_cast<int32>(Id), 0, Chains.Num() - 1)];
}

FName APSAncientsQuests::TemplateOf(const APSAncients::FSiteSpec& Spec, const int32 Step)
{
	return FName(*FString::Printf(TEXT("%s%s.%s.%d"), *APSAncientsQuestsLocal::TemplatePrefix, ChainOf(Spec.Chain).Key, *Spec.Id,
		Step + 1));
}

FString APSAncientsQuests::SubjectOf(const APSAncients::FSiteSpec& Spec, const int32 Step)
{
	return FString::Printf(TEXT("ANCIENT:%s:%d"), *Spec.Id, Step + 1);
}

bool APSAncientsQuests::IsAncientTemplate(const FName Template)
{
	return Template.ToString().StartsWith(APSAncientsQuestsLocal::TemplatePrefix);
}

bool APSAncientsQuests::FBoardView::Refresh(const FAPSMissionBoard& Board)
{
	if (Board.GetRevision() == Revision)
	{
		return false;
	}
	Revision = Board.GetRevision();
	FAPSMissionSaveData Data;
	Board.CaptureSave(Data);
	Completed.Reset();
	Open.Reset();
	for (const FName& Template : Data.CompletedTemplates)
	{
		if (IsAncientTemplate(Template))
		{
			Completed.Add(Template);
		}
	}
	for (const FAPSMission& Mission : Data.Missions)
	{
		if (!IsAncientTemplate(Mission.Template))
		{
			continue;
		}
		if (Mission.State == APSMissions::EState::Completed)
		{
			Completed.Add(Mission.Template);
		}
		else if (Mission.State == APSMissions::EState::Offered || Mission.State == APSMissions::EState::Active)
		{
			Open.Add(Mission.Template, Mission);
		}
	}
	return true;
}

FAPSMission APSAncientsQuests::MakeMission(const APSAncients::FSiteSpec& Spec, const int32 Step, const FFormatNamedArguments& Args,
	const FText& SubjectName, const double NowSeconds, const APSMissions::EState State)
{
	const FChain& Chain = ChainOf(Spec.Chain);
	const FStep& Definition = Chain.Steps[FMath::Clamp(Step, 0, Chain.Steps.Num() - 1)];
	FAPSMission Mission;
	Mission.Id = FGuid::NewGuid();
	Mission.Department = Chain.Department;
	Mission.Template = TemplateOf(Spec, Step);
	Mission.Title = Definition.Title;
	Mission.Brief = FText::Format(Definition.Brief, Args);
	Mission.Objective = Definition.Objective;
	Mission.Subject = SubjectOf(Spec, Step);
	Mission.SubjectName = SubjectName;
	Mission.Count = 1;
	Mission.Progress = 0;
	Mission.Reward = Definition.Reward;
	Mission.RewardLevels = Definition.Levels;
	Mission.Unlocks = Definition.Unlocks ? FName(Definition.Unlocks) : NAME_None;
	// The chain goes on through this runtime, never through the board's own template chains.
	Mission.Next = NAME_None;
	Mission.State = State;
	Mission.OfferedSeconds = NowSeconds;
	return Mission;
}

void APSAncientsQuests::Inject(FAPSMissionBoard& Board, const FAPSMission& Mission, const bool bTrackIfFree)
{
	// RestoreSave tracks the last active mission and rebuilds the list (the screens rebuild on the revision it moves).
	const FAPSMission* Tracked = Board.GetTracked();
	const FGuid KeepTracked = Tracked ? Tracked->Id : FGuid();
	FAPSMissionSaveData Data;
	Board.CaptureSave(Data);
	Data.Missions.Add(Mission);
	Board.RestoreSave(MoveTemp(Data));
	const bool bTrackNew = !KeepTracked.IsValid() && bTrackIfFree && Mission.State == APSMissions::EState::Active;
	Board.SetTracked(bTrackNew ? Mission.Id : KeepTracked);
}

bool APSAncientsQuests::Edit(FAPSMissionBoard& Board, const TFunctionRef<bool(FAPSMission&)> Change)
{
	FAPSMissionSaveData Data;
	Board.CaptureSave(Data);
	bool bChanged = false;
	for (FAPSMission& Mission : Data.Missions)
	{
		if (IsAncientTemplate(Mission.Template) && Change(Mission))
		{
			bChanged = true;
		}
	}
	if (!bChanged)
	{
		return false;
	}
	const FAPSMission* Tracked = Board.GetTracked();
	const FGuid KeepTracked = Tracked ? Tracked->Id : FGuid();
	Board.RestoreSave(MoveTemp(Data));
	Board.SetTracked(KeepTracked);
	return true;
}

#undef LOCTEXT_NAMESPACE
