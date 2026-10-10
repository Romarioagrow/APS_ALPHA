#include "APSMissions.h"

#include "APSInfrastructure.h"
#include "APSStarSystems.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "APSMissions"

namespace APSMissionsLocal
{
	using namespace APSInfrastructure;
	using APSMissions::EObjective;

	TMap<const UWorld*, FAPSMissionBoard*> GRegistry;

	/** How a template finds what it is about. */
	enum class ESubject : uint8
	{
		None,
		UnscannedSystem,
		ScannedSystem,
		SurveyedUnclaimedSystem,
		ClaimedSystem,
		UnsurveyedWorld,
		SurveyedWorld,
		DetectedAnomaly,
		LocatedAnomaly,
		Structure,
		Resource
	};

	struct FTemplate
	{
		const TCHAR* Id;
		EDepartment Department;
		FText Title;
		/** {0} is the subject's name. */
		FText Brief;
		EObjective Objective;
		ESubject Subject;
		int32 Count;
		TArray<FAmount> Reward;
		int32 Levels;
		/** Structure type (BuildStructure), resource name (ReachStock) or unlocked type. */
		const TCHAR* Detail;
		const TCHAR* Unlocks;
		const TCHAR* Next;
		bool bRepeatable;
	};

	FAmount M(const float Value) { return {EResource::Metals, Value}; }
	FAmount V(const float Value) { return {EResource::Volatiles, Value}; }
	FAmount E(const float Value) { return {EResource::Energy, Value}; }
	FAmount R(const float Value) { return {EResource::Research, Value}; }
	FAmount I(const float Value) { return {EResource::Influence, Value}; }

	const TArray<FTemplate>& Templates()
	{
		using D = EDepartment;
		using O = EObjective;
		using S = ESubject;
		static const TArray<FTemplate> List = {
			// EXPLORATION: chart, visit, claim.
			{TEXT("ExploreProbe"), D::Exploration, LOCTEXT("ExploreProbe", "CHART THE NEIGHBOURHOOD"),
				LOCTEXT("ExploreProbeBrief", "Send a probe to {0}: we need to know what burns there and what circles it."),
				O::ScanSystem, S::UnscannedSystem, 1, {R(20), I(5)}, 0, nullptr, nullptr, TEXT("ExploreSurvey"), true},
			{TEXT("ExploreSurvey"), D::Exploration, LOCTEXT("ExploreSurvey", "FOOTPRINTS ON NEW GROUND"),
				LOCTEXT("ExploreSurveyBrief", "Chart {0} properly: a survey ship, or fly there yourself and look around."),
				O::SurveySystem, S::ScannedSystem, 1, {R(40), M(40)}, 1, nullptr, nullptr, TEXT("ExploreBeacon"), false},
			{TEXT("ExploreBeacon"), D::Exploration, LOCTEXT("ExploreBeacon", "LIGHT THE WAY"),
				LOCTEXT("ExploreBeaconBrief", "Raise a survey beacon in {0}: the system becomes ours and shows on every map."),
				O::BuildStructure, S::SurveyedUnclaimedSystem, 1, {I(20), M(60)}, 0, TEXT("SurveyBeacon"), nullptr, nullptr, true},
			{TEXT("ExploreVisit"), D::Exploration, LOCTEXT("ExploreVisit", "A CAPTAIN'S LOG FROM AFAR"),
				LOCTEXT("ExploreVisitBrief", "Fly to {0} yourself and stay a few seconds inside its sphere: the crews want the captain's own log."),
				O::VisitSystem, S::ScannedSystem, 1, {I(15), R(15)}, 0, nullptr, nullptr, nullptr, true},
			{TEXT("ExploreWorlds"), D::Exploration, LOCTEXT("ExploreWorlds", "SURVEYOR'S ROUND"),
				LOCTEXT("ExploreWorldsBrief", "Survey two more worlds; {0} first."),
				O::SurveyWorld, S::UnsurveyedWorld, 2, {R(15), M(30)}, 0, nullptr, nullptr, nullptr, true},
			// INDUSTRY: dig, power, refine, stockpile, the ring.
			{TEXT("IndustryMine"), D::Industry, LOCTEXT("IndustryMine", "BREAK GROUND"),
				LOCTEXT("IndustryMineBrief", "The smelters are idle: a mining outpost on {0} or any surveyed world."),
				O::BuildStructure, S::SurveyedWorld, 1, {M(80)}, 0, TEXT("MiningOutpost"), nullptr, TEXT("IndustryRefine"), false},
			{TEXT("IndustryRefine"), D::Industry, LOCTEXT("IndustryRefine", "REFINE IT"),
				LOCTEXT("IndustryRefineBrief", "Ore alone builds nothing: an orbital refinery over the mining world."),
				O::BuildStructure, S::None, 1, {M(50), R(20)}, 1, TEXT("OrbitalRefinery"), nullptr, nullptr, false},
			{TEXT("IndustryPower"), D::Industry, LOCTEXT("IndustryPower", "POWER FOR THE YARDS"),
				LOCTEXT("IndustryPowerBrief", "Two solar collectors: everything else runs on energy."),
				O::BuildStructure, S::None, 2, {E(60)}, 0, TEXT("SolarCollector"), nullptr, nullptr, false},
			{TEXT("IndustryStock"), D::Industry, LOCTEXT("IndustryStock", "FILL THE STORES"),
				LOCTEXT("IndustryStockBrief", "Six hundred metals in stock before we commit to anything big."),
				O::ReachStock, S::Resource, 600, {E(40)}, 1, TEXT("Metals"), TEXT("OrbitalRing"), TEXT("IndustryRing"), false},
			{TEXT("IndustryRing"), D::Industry, LOCTEXT("IndustryRing", "THE RING PROJECT"),
				LOCTEXT("IndustryRingBrief", "The orbital ring is unlocked: it hangs on a space elevator, so raise one on a studied world first, then lay the ring around it."),
				O::BuildStructure, S::None, 1, {I(40), R(60)}, 1, TEXT("OrbitalRing"), nullptr, nullptr, false},
			// Rio 03.10: the chain's last step, offered once five swarm rings circle one star.
			{TEXT("IndustrySphere"), D::Industry, LOCTEXT("IndustrySphere", "CLOSE THE SPHERE"),
				LOCTEXT("IndustrySphereBrief", "Five swarm rings circle the star: close them into a Dyson sphere."),
				O::BuildStructure, S::None, 1, {R(120), I(80)}, 1, TEXT("DysonSphere"), nullptr, nullptr, false},
			// SCIENCE: study, find, investigate, observe, count stars.
			{TEXT("ScienceStudy"), D::Science, LOCTEXT("ScienceStudy", "LOOK CLOSER"),
				LOCTEXT("ScienceStudyBrief", "A science ship should study {0}: the survey only scratched it."),
				O::StudyWorld, S::SurveyedWorld, 1, {R(30)}, 0, nullptr, nullptr, nullptr, true},
			{TEXT("ScienceLocate"), D::Science, LOCTEXT("ScienceLocate", "SOMETHING DOWN THERE"),
				LOCTEXT("ScienceLocateBrief", "The survey of {0} picked up an anomaly; a science ship's study pins down the site."),
				O::LocateAnomaly, S::DetectedAnomaly, 1, {R(30)}, 0, nullptr, nullptr, TEXT("ScienceInvestigate"), true},
			{TEXT("ScienceInvestigate"), D::Science, LOCTEXT("ScienceInvestigate", "BOOTS ON THE SITE"),
				LOCTEXT("ScienceInvestigateBrief", "The site on {0} is marked: send an expedition, or land and walk to the beacon yourself."),
				O::InvestigateAnomaly, S::LocatedAnomaly, 1, {R(60), I(10)}, 1, nullptr, nullptr, nullptr, true},
			{TEXT("ScienceObservatory"), D::Science, LOCTEXT("ScienceObservatory", "EYES ON THE SKY"),
				LOCTEXT("ScienceObservatoryBrief", "A deep observatory on an airless world hears what the probes miss."),
				O::BuildStructure, S::None, 1, {R(40)}, 0, TEXT("DeepObservatory"), nullptr, nullptr, false},
			{TEXT("ScienceCensus"), D::Science, LOCTEXT("ScienceCensus", "STELLAR CENSUS"),
				LOCTEXT("ScienceCensusBrief", "Scan three star systems; {0} is the nearest unknown one."),
				O::ScanSystem, S::UnscannedSystem, 3, {R(50)}, 0, nullptr, TEXT("DysonSwarm"), nullptr, false},
			// CIVIL AFFAIRS: homes, rights, government, diplomacy, voice.
			{TEXT("CivilHabitat"), D::CivilAffairs, LOCTEXT("CivilHabitat", "A PLACE TO LIVE"),
				LOCTEXT("CivilHabitatBrief", "The colony is crowded: an orbital habitat over a studied world."),
				O::BuildStructure, S::None, 1, {I(30)}, 0, TEXT("OrbitalHabitat"), nullptr, nullptr, false},
			{TEXT("CivilRights"), D::CivilAffairs, LOCTEXT("CivilRights", "RIGHTS FOR THE COLONISTS"),
				LOCTEXT("CivilRightsBrief", "The colonists ask for a civic forum: public services and a charter of rights."),
				O::BuildStructure, S::None, 1, {I(25)}, 1, TEXT("CivicForum"), nullptr, nullptr, false},
			{TEXT("CivilGovern"), D::CivilAffairs, LOCTEXT("CivilGovern", "GOVERN THE FRONTIER"),
				LOCTEXT("CivilGovernBrief", "Put an administration hub in {0}: a surveyed system needs a government."),
				O::BuildStructure, S::SurveyedUnclaimedSystem, 1, {I(40)}, 0, TEXT("AdministrationHub"), nullptr, nullptr, true},
			{TEXT("CivilEmbassy"), D::CivilAffairs, LOCTEXT("CivilEmbassy", "FIRST CONTACT PROTOCOL"),
				LOCTEXT("CivilEmbassyBrief", "We may not be alone out here: a diplomatic embassy, ready for whoever answers."),
				O::BuildStructure, S::None, 1, {I(40), R(20)}, 0, TEXT("DiplomaticEmbassy"), nullptr, nullptr, false},
			{TEXT("CivilVoice"), D::CivilAffairs, LOCTEXT("CivilVoice", "THE VOICE OF THE PEOPLE"),
				LOCTEXT("CivilVoiceBrief", "Two hundred influence: enough voice to raise a space elevator."),
				O::ReachStock, S::Resource, 200, {R(30)}, 0, TEXT("Influence"), TEXT("SpaceElevator"), nullptr, false},
			// MILITARY: guard, watch, patrol, garrison, hulls.
			{TEXT("MilitaryGuard"), D::Military, LOCTEXT("MilitaryGuard", "GUARD THE CRADLE"),
				LOCTEXT("MilitaryGuardBrief", "A defence platform over our worlds: nothing guards the cradle yet."),
				O::BuildStructure, S::None, 1, {M(40), I(10)}, 0, TEXT("DefencePlatform"), nullptr, nullptr, false},
			{TEXT("MilitaryPicket"), D::Military, LOCTEXT("MilitaryPicket", "WATCH THE EDGE"),
				LOCTEXT("MilitaryPicketBrief", "A sensor picket in {0}: whatever comes, we see it first."),
				O::BuildStructure, S::ScannedSystem, 1, {R(20)}, 0, TEXT("SensorPicket"), nullptr, nullptr, true},
			{TEXT("MilitaryPatrol"), D::Military, LOCTEXT("MilitaryPatrol", "SHOW THE FLAG"),
				LOCTEXT("MilitaryPatrolBrief", "Patrol {0} in person: the beacon is lit, now the flag must be seen."),
				O::VisitSystem, S::ClaimedSystem, 1, {I(20)}, 0, nullptr, nullptr, nullptr, true},
			{TEXT("MilitaryGarrison"), D::Military, LOCTEXT("MilitaryGarrison", "GARRISON"),
				LOCTEXT("MilitaryGarrisonBrief", "Troops and patrol craft stationed on a world of ours."),
				O::BuildStructure, S::None, 1, {M(30)}, 1, TEXT("Garrison"), nullptr, nullptr, false},
			{TEXT("MilitaryHull"), D::Military, LOCTEXT("MilitaryHull", "A NEW HULL FOR THE LINE"),
				LOCTEXT("MilitaryHullBrief", "Launch a ship from a shipyard: the line is thin."),
				O::LaunchShip, S::None, 1, {M(60)}, 0, nullptr, nullptr, nullptr, true},
			// FLEET COMMAND: hulls, docks, supply, long hauls, the gate.
			{TEXT("FleetHulls"), D::FleetCommand, LOCTEXT("FleetHulls", "NEW HULLS"),
				LOCTEXT("FleetHullsBrief", "Two new ships off the slipways."),
				O::LaunchShip, S::None, 2, {M(80)}, 1, nullptr, nullptr, nullptr, false},
			{TEXT("FleetDock"), D::FleetCommand, LOCTEXT("FleetDock", "KEEP THEM FLYING"),
				LOCTEXT("FleetDockBrief", "A repair dock: ships come back from the frontier worn."),
				O::BuildStructure, S::None, 1, {E(40)}, 0, TEXT("RepairDock"), nullptr, nullptr, false},
			{TEXT("FleetDepot"), D::FleetCommand, LOCTEXT("FleetDepot", "SUPPLY LINES"),
				LOCTEXT("FleetDepotBrief", "A supply depot: fuel and spares where the fleet needs them."),
				O::BuildStructure, S::None, 1, {V(40)}, 0, TEXT("SupplyDepot"), nullptr, nullptr, false},
			{TEXT("FleetLongHaul"), D::FleetCommand, LOCTEXT("FleetLongHaul", "THE LONG HAUL"),
				LOCTEXT("FleetLongHaulBrief", "Chart {0} with a ship: the fleet must learn the way between the stars."),
				O::SurveySystem, S::ScannedSystem, 1, {I(15), E(30)}, 0, nullptr, nullptr, nullptr, true},
			{TEXT("FleetGate"), D::FleetCommand, LOCTEXT("FleetGate", "FLEET REVIEW"),
				LOCTEXT("FleetGateBrief", "Four hundred energy in reserve: the admiralty will approve a jump gate."),
				O::ReachStock, S::Resource, 400, {I(20)}, 0, TEXT("Energy"), TEXT("JumpGate"), nullptr, false},
			// TRANSPORT: hubs, relays, the network, drivers, corridors.
			{TEXT("TransportHub"), D::Transport, LOCTEXT("TransportHub", "CARGO FLOWS"),
				LOCTEXT("TransportHubBrief", "A cargo hub: goods must move between our worlds."),
				O::BuildStructure, S::None, 1, {M(50), E(20)}, 0, TEXT("CargoHub"), nullptr, nullptr, false},
			{TEXT("TransportRelay"), D::Transport, LOCTEXT("TransportRelay", "LINK THE STARS"),
				LOCTEXT("TransportRelayBrief", "A hyperspace relay in {0}: our network reaches out from there."),
				O::BuildStructure, S::SurveyedUnclaimedSystem, 1, {I(30)}, 0, TEXT("HyperspaceRelay"), nullptr, TEXT("TransportNetwork"), true},
			{TEXT("TransportNetwork"), D::Transport, LOCTEXT("TransportNetwork", "THE NETWORK"),
				LOCTEXT("TransportNetworkBrief", "Two links in the relay network between claimed systems."),
				O::LinkSystems, S::None, 2, {M(100), R(40)}, 1, nullptr, nullptr, nullptr, false},
			{TEXT("TransportDriver"), D::Transport, LOCTEXT("TransportDriver", "THROW IT TO ORBIT"),
				LOCTEXT("TransportDriverBrief", "A mass driver next to a mining outpost: the ore goes up without rockets."),
				O::BuildStructure, S::None, 1, {M(60)}, 0, TEXT("MassDriver"), nullptr, nullptr, false},
			{TEXT("TransportCorridor"), D::Transport, LOCTEXT("TransportCorridor", "CLAIM THE CORRIDOR"),
				LOCTEXT("TransportCorridorBrief", "Claim two more star systems; {0} lies on the way."),
				O::ClaimSystem, S::SurveyedUnclaimedSystem, 2, {I(50)}, 0, nullptr, nullptr, nullptr, false},
			// Rio 03.10: the hubs, the system's logistics.
			{TEXT("TransportSpaceHub"), D::Transport, LOCTEXT("TransportSpaceHub", "A HUB FOR THE SYSTEM"),
				LOCTEXT("TransportSpaceHubBrief", "Raise a space hub over a world one of our stations orbits: docks, depots and crews for the whole system."),
				O::BuildStructure, S::None, 1, {I(40), E(60)}, 1, TEXT("SpaceHub"), nullptr, TEXT("TransportGrandHub"), false},
			{TEXT("TransportGrandHub"), D::Transport, LOCTEXT("TransportGrandHub", "THE GRAND HUB"),
				LOCTEXT("TransportGrandHubBrief", "Grow the system's port: a grand hub over the world that holds the space hub."),
				O::BuildStructure, S::None, 1, {I(80), R(40)}, 1, TEXT("GrandHub"), nullptr, nullptr, false},
		};
		return List;
	}

	const FTemplate* FindTemplate(const FName Id)
	{
		return Templates().FindByPredicate([Id](const FTemplate& Each) { return Id == FName(Each.Id); });
	}

	constexpr int32 OffersPerDepartment = 2;
	constexpr int32 DepartmentCount = static_cast<int32>(EDepartment::Count);

	/** Picks what a template is about from the world now; false when nothing fits (the template waits). */
	bool PickSubject(UWorld* World, const FTemplate& Template, FString& OutSubject, FText& OutName)
	{
		OutSubject.Reset();
		OutName = FText::GetEmpty();
		const FAPSStarSystems* Stars = APSStarSystemsFind(World);
		const FAPSFleetCommand* Fleet = APSFleetFind(World);
		const auto System = [&](const TFunctionRef<bool(const FAPSStarSystemInfo&, const FAPSStarSystemState&)> Fits)
		{
			if (!Stars || !Stars->GetHome()) return false;
			TArray<int32> Candidates;
			Stars->FindNearest(Stars->GetHome()->Location, 80, Candidates);
			for (const int32 Index : Candidates)
			{
				const FAPSStarSystemInfo* Info = Stars->Get(Index);
				if (!Info || Info->bHome) continue;
				if (Fits(*Info, Stars->GetState(Info->Id)))
				{
					OutSubject = Info->Id.ToString(EGuidFormats::Digits);
					OutName = FText::FromString(Info->Name);
					return true;
				}
			}
			return false;
		};
		const auto World_ = [&](const TFunctionRef<bool(const FAPSFleetBodyRecord*, APSFleet::ESurvey)> Fits)
		{
			if (!Fleet || !World) return false;
			for (TActorIterator<APlanetaryBody> It(World); It; ++It)
			{
				if (!IsValid(*It)) continue;
				if (Fits(Fleet->FindBody(*It), Fleet->GetSurvey(*It)))
				{
					OutSubject = FAPSFleetCommand::KeyOf(*It);
					OutName = FText::FromString(It->AstroName.ToString().ToUpper());
					return true;
				}
			}
			return false;
		};
		switch (Template.Subject)
		{
		case ESubject::None:
			if (Template.Objective == EObjective::BuildStructure && Template.Detail) OutSubject = Template.Detail;
			return true;
		case ESubject::Resource:
			OutSubject = Template.Detail ? Template.Detail : TEXT("");
			return true;
		case ESubject::Structure:
			OutSubject = Template.Detail ? Template.Detail : TEXT("");
			return true;
		case ESubject::UnscannedSystem:
			return System([](const FAPSStarSystemInfo&, const FAPSStarSystemState& State)
				{ return State.Knowledge == APSStars::EKnowledge::Catalogued; });
		case ESubject::ScannedSystem:
			return System([](const FAPSStarSystemInfo&, const FAPSStarSystemState& State)
				{ return State.Knowledge == APSStars::EKnowledge::Scanned; });
		case ESubject::SurveyedUnclaimedSystem:
			return System([](const FAPSStarSystemInfo&, const FAPSStarSystemState& State)
				{ return State.Knowledge == APSStars::EKnowledge::Surveyed && !State.bClaimed; });
		case ESubject::ClaimedSystem:
			return System([](const FAPSStarSystemInfo&, const FAPSStarSystemState& State) { return State.bClaimed; });
		case ESubject::UnsurveyedWorld:
			return World_([](const FAPSFleetBodyRecord*, const APSFleet::ESurvey Survey) { return Survey == APSFleet::ESurvey::Unknown; });
		case ESubject::SurveyedWorld:
			return World_([](const FAPSFleetBodyRecord*, const APSFleet::ESurvey Survey) { return Survey == APSFleet::ESurvey::Surveyed; });
		case ESubject::DetectedAnomaly:
			return World_([](const FAPSFleetBodyRecord* Record, APSFleet::ESurvey)
				{ return Record && Record->bHasAnomaly && Record->Anomaly == APSFleet::EAnomalyState::Detected; });
		case ESubject::LocatedAnomaly:
			return World_([](const FAPSFleetBodyRecord* Record, APSFleet::ESurvey)
				{ return Record && Record->bHasAnomaly && Record->Anomaly == APSFleet::EAnomalyState::Located; });
		default:
			return false;
		}
	}

	/**
	 * Rio 03.10: the step before a chain type stands somewhere (a space hub before the grand hub, an elevator before the
	 * ring, five swarm rings at one star before the sphere); true for a type that stands on no step.
	 */
	bool ChainStepBeforeStands(const FAPSInfrastructure* Infrastructure, const APSInfrastructure::FType& Type)
	{
		const FName Before = !Type.RequiresAtSite.IsNone() ? Type.RequiresAtSite : Type.RequiresInSystem;
		if (Before.IsNone())
		{
			return true;
		}
		if (!Infrastructure)
		{
			return false;
		}
		const int32 Wanted = Type.RequiresAtSite.IsNone() ? 1 : FMath::Max(Type.RequiresAtSiteCount, 1);
		TMap<FString, int32> PerPlace;
		for (const FAPSBuiltStructure& Built : Infrastructure->GetStructures())
		{
			if (Built.Type == Before && ++PerPlace.FindOrAdd(Built.SiteKey) >= Wanted)
			{
				return true;
			}
		}
		return false;
	}

	/** Which subject a BuildStructure mission compares against: the structure type, not where it is built (any place). */
	bool Matches(const FAPSMission& Mission, const EObjective Objective, const FString& Subject)
	{
		if (Mission.State != APSMissions::EState::Active || Mission.Objective != Objective) return false;
		if (Objective == EObjective::BuildStructure)
		{
			const FTemplate* Template = FindTemplate(Mission.Template);
			return !Template || !Template->Detail || Subject == Template->Detail;
		}
		// Repeat-style counts (survey two worlds, scan three systems) take any subject; single targets their own.
		return Mission.Count > 1 || Mission.Subject.IsEmpty() || Mission.Subject == Subject;
	}
}

FText APSMissions::ObjectiveName(const EObjective Objective)
{
	switch (Objective)
	{
	case EObjective::SurveyWorld: return LOCTEXT("ObjSurveyWorld", "SURVEY A WORLD");
	case EObjective::StudyWorld: return LOCTEXT("ObjStudyWorld", "STUDY A WORLD");
	case EObjective::ScanSystem: return LOCTEXT("ObjScanSystem", "SCAN A STAR SYSTEM");
	case EObjective::SurveySystem: return LOCTEXT("ObjSurveySystem", "SURVEY A STAR SYSTEM");
	case EObjective::ClaimSystem: return LOCTEXT("ObjClaimSystem", "CLAIM A STAR SYSTEM");
	case EObjective::VisitSystem: return LOCTEXT("ObjVisitSystem", "VISIT IN PERSON");
	case EObjective::LandOnWorld: return LOCTEXT("ObjLand", "LAND ON A WORLD");
	case EObjective::BuildStructure: return LOCTEXT("ObjBuild", "BUILD");
	case EObjective::LocateAnomaly: return LOCTEXT("ObjLocate", "LOCATE AN ANOMALY");
	case EObjective::InvestigateAnomaly: return LOCTEXT("ObjInvestigate", "INVESTIGATE AN ANOMALY");
	case EObjective::LaunchShip: return LOCTEXT("ObjLaunch", "LAUNCH SHIPS");
	case EObjective::ReachStock: return LOCTEXT("ObjStock", "REACH A STOCK");
	case EObjective::LinkSystems: return LOCTEXT("ObjLink", "LINK THE NETWORK");
	default: return FText::GetEmpty();
	}
}

FText APSMissions::StateName(const EState State)
{
	switch (State)
	{
	case EState::Active: return LOCTEXT("Active", "ACTIVE");
	case EState::Completed: return LOCTEXT("Completed", "COMPLETED");
	case EState::Expired: return LOCTEXT("Expired", "EXPIRED");
	default: return LOCTEXT("Offered", "OFFERED");
	}
}

FArchive& operator<<(FArchive& Ar, FAPSMissionSaveData& Data)
{
	uint8 Version = 1;
	Ar << Version;
	if (Version < 1) return Ar;
	int32 Count = Data.Missions.Num();
	Ar << Count;
	if (Ar.IsLoading()) Data.Missions.SetNum(FMath::Clamp(Count, 0, 1000));
	for (FAPSMission& Mission : Data.Missions)
	{
		FString Template = Mission.Template.ToString();
		FString Title = Mission.Title.ToString();
		FString Brief = Mission.Brief.ToString();
		FString SubjectName = Mission.SubjectName.ToString();
		uint8 Department = static_cast<uint8>(Mission.Department);
		uint8 Objective = static_cast<uint8>(Mission.Objective);
		uint8 State = static_cast<uint8>(Mission.State);
		Ar << Mission.Id << Template << Title << Brief << SubjectName << Department << Objective << State;
		Ar << Mission.Subject << Mission.Count << Mission.Progress << Mission.OfferedSeconds;
		if (Ar.IsLoading())
		{
			Mission.Template = FName(*Template);
			Mission.Title = FText::FromString(Title);
			Mission.Brief = FText::FromString(Brief);
			Mission.SubjectName = FText::FromString(SubjectName);
			Mission.Department = static_cast<APSInfrastructure::EDepartment>(FMath::Min<uint8>(Department,
				static_cast<uint8>(APSInfrastructure::EDepartment::Count) - 1));
			Mission.Objective = static_cast<APSMissions::EObjective>(FMath::Min<uint8>(Objective,
				static_cast<uint8>(APSMissions::EObjective::Count) - 1));
			Mission.State = static_cast<APSMissions::EState>(FMath::Min<uint8>(State, static_cast<uint8>(APSMissions::EState::Expired)));
			// Rewards and chains come from the template, so a load keeps them current.
			if (const APSMissionsLocal::FTemplate* Found = APSMissionsLocal::FindTemplate(Mission.Template))
			{
				Mission.Reward = Found->Reward;
				Mission.RewardLevels = Found->Levels;
				Mission.Unlocks = Found->Unlocks ? FName(Found->Unlocks) : NAME_None;
				Mission.Next = Found->Next ? FName(Found->Next) : NAME_None;
			}
		}
	}
	TArray<FString> Unlocked, Completed;
	if (Ar.IsSaving())
	{
		for (const FName& Name : Data.Unlocked) Unlocked.Add(Name.ToString());
		for (const FName& Name : Data.CompletedTemplates) Completed.Add(Name.ToString());
	}
	Ar << Unlocked << Completed << Data.EarnedLevels;
	if (Ar.IsLoading())
	{
		Data.Unlocked.Reset();
		Data.CompletedTemplates.Reset();
		for (const FString& Name : Unlocked) Data.Unlocked.Add(FName(*Name));
		for (const FString& Name : Completed) Data.CompletedTemplates.Add(FName(*Name));
	}
	return Ar;
}

FAPSMissionBoard* APSMissionsFind(const UWorld* World)
{
	return World ? APSMissionsLocal::GRegistry.FindRef(World) : nullptr;
}

void APSMissionsRegister(const UWorld* World, FAPSMissionBoard* Board)
{
	if (!World) return;
	if (Board) APSMissionsLocal::GRegistry.Add(World, Board);
	else APSMissionsLocal::GRegistry.Remove(World);
}

void APSMissionsNotify(const UWorld* World, const APSMissions::EObjective Objective, const FString& Subject, const int32 Amount)
{
	if (FAPSMissionBoard* Board = APSMissionsFind(World)) Board->Notify(Objective, Subject, Amount);
}

FAPSMissionBoard::FAPSMissionBoard(UWorld* InWorld)
	: World(InWorld)
{
}

FAPSMissionBoard::~FAPSMissionBoard() = default;

void FAPSMissionBoard::Tick(const float DeltaSeconds)
{
	using namespace APSMissionsLocal;
	OfferClock -= DeltaSeconds;
	if (OfferClock > 0.0f) return;
	OfferClock = 3.0f;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld) return;
	// Missions that watch a state rather than an event: stocks and the relay network.
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld);
	const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	for (FAPSMission& Mission : Missions)
	{
		if (Mission.State != APSMissions::EState::Active) continue;
		int32 Now = -1;
		if (Mission.Objective == EObjective::ReachStock && Infrastructure)
		{
			for (int32 Resource = 0; Resource < static_cast<int32>(EResource::Count); ++Resource)
			{
				if (ResourceName(static_cast<EResource>(Resource)).ToString().Equals(Mission.Subject, ESearchCase::IgnoreCase))
				{
					Now = FMath::FloorToInt(Infrastructure->GetStock(static_cast<EResource>(Resource)));
				}
			}
		}
		else if (Mission.Objective == EObjective::LinkSystems && Stars)
		{
			TArray<TPair<int32, int32>> Links;
			Stars->GetNetwork(Links);
			Now = Links.Num();
		}
		if (Now >= 0 && Now != Mission.Progress)
		{
			Mission.Progress = FMath::Min(Now, Mission.Count);
			++Revision;
		}
	}
	// Complete what reached its count (events may also have moved it).
	for (FAPSMission& Mission : Missions)
	{
		if (Mission.State == APSMissions::EState::Active && Mission.Progress >= Mission.Count)
		{
			Mission.State = APSMissions::EState::Completed;
			for (const FAmount& Amount : Mission.Reward)
			{
				if (FAPSInfrastructure* Runtime = APSInfrastructureFind(LiveWorld)) Runtime->AddStock(Amount.Resource, Amount.Value);
			}
			EarnedLevels[static_cast<int32>(Mission.Department)] += Mission.RewardLevels;
			if (!Mission.Unlocks.IsNone()) Unlocked.Add(Mission.Unlocks);
			CompletedTemplates.AddUnique(Mission.Template);
			++Revision;
			const APSInfrastructure::FType* Unlock = Mission.Unlocks.IsNone() ? nullptr : APSInfrastructure::Find(Mission.Unlocks);
			UAPSCivilizationJournalSubsystem::Post(LiveWorld, TEXT("Missions"), FText::Format(
				LOCTEXT("Completed", "{0} — {1} completed. Reward: {2}{3}{4}."), DepartmentName(Mission.Department), Mission.Title,
				DescribeAmounts(Mission.Reward),
				Mission.RewardLevels > 0 ? FText::Format(LOCTEXT("RewardLevel", ", +{0} level"), FText::AsNumber(Mission.RewardLevels)) : FText::GetEmpty(),
				Unlock ? FText::Format(LOCTEXT("RewardUnlock", ", unlocks {0}"), Unlock->Name) : FText::GetEmpty()));
			if (!Mission.Next.IsNone())
			{
				// The chain goes on at once, active: the department already has the crew on it.
				if (const FTemplate* Next = FindTemplate(Mission.Next))
				{
					FString Subject;
					FText SubjectName;
					if (PickSubject(LiveWorld, *Next, Subject, SubjectName))
					{
						FAPSMission& Follow = Missions.AddDefaulted_GetRef();
						Follow.Id = FGuid::NewGuid();
						Follow.Department = Next->Department;
						Follow.Template = FName(Next->Id);
						Follow.Title = Next->Title;
						Follow.Brief = FText::Format(Next->Brief, SubjectName.IsEmpty() ? LOCTEXT("Anywhere", "a place of our choosing") : SubjectName);
						Follow.Objective = Next->Objective;
						Follow.Subject = Subject;
						Follow.SubjectName = SubjectName;
						Follow.Count = Next->Count;
						Follow.Reward = Next->Reward;
						Follow.RewardLevels = Next->Levels;
						Follow.Unlocks = Next->Unlocks ? FName(Next->Unlocks) : NAME_None;
						Follow.Next = Next->Next ? FName(Next->Next) : NAME_None;
						Follow.State = APSMissions::EState::Active;
						Follow.OfferedSeconds = LiveWorld->GetTimeSeconds();
						Tracked = Follow.Id;
						UAPSCivilizationJournalSubsystem::Post(LiveWorld, TEXT("Missions"), FText::Format(
							LOCTEXT("ChainNext", "{0} next: {1}. {2}"), DepartmentName(Follow.Department), Follow.Title, Follow.Brief));
					}
				}
			}
			break; // the array may have grown; the rest completes on the next pass
		}
	}
	// Keep a couple of offers per department from the templates that fit what is known now.
	for (int32 Department = 0; Department < DepartmentCount; ++Department)
	{
		int32 Offered = 0;
		for (const FAPSMission& Mission : Missions)
		{
			Offered += static_cast<int32>(Mission.Department) == Department && Mission.State == APSMissions::EState::Offered ? 1 : 0;
		}
		for (const FTemplate& Template : Templates())
		{
			if (Offered >= OffersPerDepartment) break;
			if (static_cast<int32>(Template.Department) != Department) continue;
			const FName Id(Template.Id);
			if (!Template.bRepeatable && CompletedTemplates.Contains(Id)) continue;
			if (Missions.ContainsByPredicate([Id](const FAPSMission& Mission)
				{ return Mission.Template == Id && (Mission.State == APSMissions::EState::Offered || Mission.State == APSMissions::EState::Active); }))
			{
				continue;
			}
			// Structures beyond the department's level or still locked wait until they can be built at all; a hub's or a
			// megastructure's chain step (Rio 03.10) until the step before it stands.
			if (Template.Objective == EObjective::BuildStructure && Template.Detail)
			{
				const APSInfrastructure::FType* Type = APSInfrastructure::Find(FName(Template.Detail));
				if (!Type || (Type->bNeedsUnlock && !Unlocked.Contains(Type->Id))
					|| FAPSInfrastructure::DepartmentLevel(LiveWorld, Type->Department) < Type->RequiredLevel
					|| (Type->bFleetOnly && !ChainStepBeforeStands(Infrastructure, *Type)))
				{
					continue;
				}
			}
			FString Subject;
			FText SubjectName;
			if (!PickSubject(LiveWorld, Template, Subject, SubjectName)) continue;
			FAPSMission& Mission = Missions.AddDefaulted_GetRef();
			Mission.Id = FGuid::NewGuid();
			Mission.Department = Template.Department;
			Mission.Template = Id;
			Mission.Title = Template.Title;
			Mission.Brief = FText::Format(Template.Brief, SubjectName.IsEmpty() ? LOCTEXT("Anywhere", "a place of our choosing") : SubjectName);
			Mission.Objective = Template.Objective;
			Mission.Subject = Template.Objective == EObjective::BuildStructure || Template.Count > 1 ? (Template.Detail ? Template.Detail : TEXT("")) : Subject;
			if (Template.Objective == EObjective::ReachStock) Mission.Subject = Template.Detail ? Template.Detail : TEXT("");
			Mission.SubjectName = SubjectName;
			Mission.Count = Template.Count;
			Mission.Reward = Template.Reward;
			Mission.RewardLevels = Template.Levels;
			Mission.Unlocks = Template.Unlocks ? FName(Template.Unlocks) : NAME_None;
			Mission.Next = Template.Next ? FName(Template.Next) : NAME_None;
			Mission.State = APSMissions::EState::Offered;
			Mission.OfferedSeconds = LiveWorld->GetTimeSeconds();
			++Offered;
			++Revision;
		}
	}
	// The board keeps the last completed missions for the record; older ones go.
	int32 Done = 0;
	for (int32 Index = Missions.Num() - 1; Index >= 0; --Index)
	{
		if (Missions[Index].State == APSMissions::EState::Completed && ++Done > 20) Missions.RemoveAt(Index);
	}
}

void FAPSMissionBoard::GetFor(const APSInfrastructure::EDepartment Department, TArray<const FAPSMission*>& OutMissions) const
{
	OutMissions.Reset();
	for (const FAPSMission& Mission : Missions)
	{
		if (Mission.Department == Department) OutMissions.Add(&Mission);
	}
}

const FAPSMission* FAPSMissionBoard::Find(const FGuid& Id) const
{
	return Missions.FindByPredicate([&Id](const FAPSMission& Mission) { return Mission.Id == Id; });
}

bool FAPSMissionBoard::Accept(const FGuid& Id)
{
	FAPSMission* Mission = Missions.FindByPredicate([&Id](const FAPSMission& Each) { return Each.Id == Id; });
	if (!Mission || Mission->State != APSMissions::EState::Offered) return false;
	Mission->State = APSMissions::EState::Active;
	Tracked = Id;
	++Revision;
	UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Missions"), FText::Format(
		LOCTEXT("Accepted", "{0} — accepted: {1}. {2}"), APSInfrastructure::DepartmentName(Mission->Department), Mission->Title,
		Mission->Brief));
	return true;
}

bool FAPSMissionBoard::Decline(const FGuid& Id)
{
	const int32 Index = Missions.IndexOfByPredicate([&Id](const FAPSMission& Each) { return Each.Id == Id; });
	if (Index == INDEX_NONE || Missions[Index].State == APSMissions::EState::Completed) return false;
	Missions.RemoveAt(Index);
	if (Tracked == Id) Tracked.Invalidate();
	++Revision;
	return true;
}

const FAPSMission* FAPSMissionBoard::GetTracked() const
{
	const FAPSMission* Mission = Find(Tracked);
	return Mission && Mission->State == APSMissions::EState::Active ? Mission : nullptr;
}

void FAPSMissionBoard::SetTracked(const FGuid& Id)
{
	Tracked = Id;
	++Revision;
}

void FAPSMissionBoard::Notify(const APSMissions::EObjective Objective, const FString& Subject, const int32 Amount)
{
	bool bChanged = false;
	for (FAPSMission& Mission : Missions)
	{
		if (APSMissionsLocal::Matches(Mission, Objective, Subject))
		{
			Mission.Progress = FMath::Min(Mission.Count, Mission.Progress + FMath::Max(Amount, 0));
			bChanged = true;
		}
	}
	if (bChanged)
	{
		++Revision;
		OfferClock = 0.0f; // complete and pay at once
	}
}

int32 FAPSMissionBoard::GetEarnedLevels(const APSInfrastructure::EDepartment Department) const
{
	const int32 Index = static_cast<int32>(Department);
	return Index >= 0 && Index < APSMissionsLocal::DepartmentCount ? EarnedLevels[Index] : 0;
}

void FAPSMissionBoard::Unlock(const FName Type)
{
	if (Type.IsNone() || Unlocked.Contains(Type))
	{
		return;
	}
	Unlocked.Add(Type);
	++Revision;
	UE_LOG(LogTemp, Log, TEXT("[APS.Missions] unlocked %s"), *Type.ToString());
}

void FAPSMissionBoard::CaptureSave(FAPSMissionSaveData& OutData) const
{
	OutData = FAPSMissionSaveData();
	OutData.Missions = Missions;
	OutData.Unlocked = Unlocked.Array();
	OutData.CompletedTemplates = CompletedTemplates;
	for (int32 Index = 0; Index < APSMissionsLocal::DepartmentCount; ++Index) OutData.EarnedLevels.Add(EarnedLevels[Index]);
}

void FAPSMissionBoard::RestoreSave(FAPSMissionSaveData&& Data)
{
	Missions = MoveTemp(Data.Missions);
	Unlocked = TSet<FName>(Data.Unlocked);
	CompletedTemplates = MoveTemp(Data.CompletedTemplates);
	for (int32 Index = 0; Index < APSMissionsLocal::DepartmentCount && Index < Data.EarnedLevels.Num(); ++Index)
	{
		EarnedLevels[Index] = Data.EarnedLevels[Index];
	}
	for (const FAPSMission& Mission : Missions)
	{
		if (Mission.State == APSMissions::EState::Active) Tracked = Mission.Id;
	}
	++Revision;
}

namespace APSMissionsLocal
{
	FAutoConsoleCommandWithWorld ListCommand(TEXT("aps.Missions.List"), TEXT("Logs every department's missions."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld)
		{
			const FAPSMissionBoard* Board = APSMissionsFind(InWorld);
			if (!Board) return;
			for (const FAPSMission& Mission : Board->GetMissions())
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Missions] %-13s %-9s %s (%d/%d) %s"), *DepartmentName(Mission.Department).ToString(),
					*APSMissions::StateName(Mission.State).ToString(), *Mission.Title.ToString(), Mission.Progress, Mission.Count,
					*Mission.Brief.ToString());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs AcceptCommand(TEXT("aps.Missions.Accept"),
		TEXT("Accepts the first offered mission whose title contains the text: aps.Missions.Accept <text>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* InWorld)
		{
			FAPSMissionBoard* Board = APSMissionsFind(InWorld);
			if (!Board || Args.IsEmpty()) return;
			for (const FAPSMission& Mission : Board->GetMissions())
			{
				if (Mission.State == APSMissions::EState::Offered && Mission.Title.ToString().Contains(Args[0], ESearchCase::IgnoreCase))
				{
					Board->Accept(Mission.Id);
					return;
				}
			}
		}));
}

#undef LOCTEXT_NAMESPACE
