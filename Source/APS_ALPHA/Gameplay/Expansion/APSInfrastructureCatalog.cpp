#include "APSInfrastructureCatalog.h"

#include "APS_ALPHA/Gameplay/Origins/APSOrigins.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"

#define LOCTEXT_NAMESPACE "APSInfrastructure"

namespace APSInfrastructureCatalogLocal
{
	using namespace APSInfrastructure;

	FAmount M(const float Value) { return {EResource::Metals, Value}; }
	FAmount V(const float Value) { return {EResource::Volatiles, Value}; }
	FAmount E(const float Value) { return {EResource::Energy, Value}; }
	FAmount R(const float Value) { return {EResource::Research, Value}; }
	FAmount I(const float Value) { return {EResource::Influence, Value}; }

	FType Make(const TCHAR* Id, const FText& Name, const FText& Role, const EDepartment Department, const ECategory Category,
		const EPlacement Placement, const uint8 Knowledge, const int32 Level, const float Seconds,
		TArray<FAmount> Cost, TArray<FAmount> Yield, const EVisual Visual)
	{
		FType Type;
		Type.Id = FName(Id);
		Type.Name = Name;
		Type.Role = Role;
		Type.Department = Department;
		Type.Category = Category;
		Type.Placement = Placement;
		Type.RequiredKnowledge = Knowledge;
		Type.RequiredLevel = Level;
		Type.BuildSeconds = Seconds;
		Type.Cost = MoveTemp(Cost);
		Type.Yield = MoveTemp(Yield);
		Type.Visual = Visual;
		return Type;
	}

	TArray<FType> Build()
	{
		using D = EDepartment;
		using C = ECategory;
		using P = EPlacement;
		using L = EVisual;
		TArray<FType> Types;

		// EXPLORATION: claim the stars, see further, survey faster.
		{
			FType T = Make(TEXT("SurveyBeacon"), LOCTEXT("SurveyBeacon", "SURVEY BEACON"),
				LOCTEXT("SurveyBeaconRole", "Marks a star system as the civilization's and lights a beacon on every map."),
				D::Exploration, C::Relay, P::StarSystem, 1, 0, 40.0f, {M(40), E(20)}, {I(1)}, L::Beacon);
			T.bClaims = true;
			T.RelayReachAu = 1.5f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("DeepSpaceScanner"), LOCTEXT("DeepSpaceScanner", "DEEP-SPACE SCANNER"),
				LOCTEXT("DeepSpaceScannerRole", "Scans the star systems within 2.5 AU: their stars and how many worlds they hold."),
				D::Exploration, C::Station, P::StarSystem, 2, 1, 60.0f, {M(80), E(40)}, {R(2)}, L::Station);
			T.ScanReachAu = 2.5f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("SurveyOutpost"), LOCTEXT("SurveyOutpost", "SURVEY OUTPOST"),
				LOCTEXT("SurveyOutpostRole", "A crewed post over a world: surveys and studies there go half again as fast."),
				D::Exploration, C::Outpost, P::Orbit, 1, 0, 35.0f, {M(30)}, {R(1)}, L::Outpost);
			T.LocalWorkSpeed = 0.5f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("ProbeBay"), LOCTEXT("ProbeBay", "PROBE BAY"),
				LOCTEXT("ProbeBayRole", "Builds survey probes in orbit: every survey of the civilization 10% faster."),
				D::Exploration, C::Station, P::Orbit, 1, 1, 50.0f, {M(60), E(20)}, {}, L::Station);
			T.SurveySpeed = 0.10f;
			Types.Add(T);
		}

		// INDUSTRY: metals, volatiles, energy, faster construction.
		{
			FType T = Make(TEXT("MiningOutpost"), LOCTEXT("MiningOutpost", "MINING OUTPOST"),
				LOCTEXT("MiningOutpostRole", "Extracts metals from the crust."),
				D::Industry, C::Outpost, P::Surface, 1, 0, 50.0f, {M(40), E(10)}, {M(6)}, L::Outpost);
			T.LimitPerSite = 3;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("GasHarvester"), LOCTEXT("GasHarvester", "GAS HARVESTER"),
				LOCTEXT("GasHarvesterRole", "Skims volatiles from a giant's upper atmosphere."),
				D::Industry, C::Station, P::Orbit, 1, 0, 60.0f, {M(60)}, {V(6)}, L::Station);
			T.bGiantOnly = true;
			T.LimitPerSite = 2;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("OrbitalRefinery"), LOCTEXT("OrbitalRefinery", "ORBITAL REFINERY"),
				LOCTEXT("OrbitalRefineryRole", "Turns the world's ore into alloys and fuel."),
				D::Industry, C::Station, P::Orbit, 1, 1, 80.0f, {M(80), E(30)}, {M(3), E(2)}, L::Station);
			T.RequiresAtSite = FName(TEXT("MiningOutpost"));
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("FabricationYard"), LOCTEXT("FabricationYard", "FABRICATION YARD"),
				LOCTEXT("FabricationYardRole", "Prefabricates modules: every construction of the civilization 15% faster."),
				D::Industry, C::Station, P::Orbit, 1, 2, 90.0f, {M(120), E(40)}, {}, L::Shipyard);
			T.BuildSpeed = 0.15f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("SolarCollector"), LOCTEXT("SolarCollector", "SOLAR COLLECTOR"),
				LOCTEXT("SolarCollectorRole", "Power from the star for everything else."),
				D::Industry, C::Station, P::Orbit, 0, 0, 40.0f, {M(30)}, {E(5)}, L::Station);
			T.LimitPerSite = 4;
			Types.Add(T);
		}

		// SCIENCE: research, observation, anomalies.
		{
			FType T = Make(TEXT("ResearchStation"), LOCTEXT("ResearchStation", "RESEARCH STATION"),
				LOCTEXT("ResearchStationRole", "Laboratories in orbit: research, and studies here a third faster."),
				D::Science, C::Station, P::Orbit, 2, 0, 70.0f, {M(70), E(20)}, {R(5)}, L::Station);
			T.LocalWorkSpeed = 0.3f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("DeepObservatory"), LOCTEXT("DeepObservatory", "DEEP OBSERVATORY"),
				LOCTEXT("DeepObservatoryRole", "A telescope array on the ground: scans the stars within 4 AU and hears deep-space signals."),
				D::Science, C::Outpost, P::Surface, 1, 2, 90.0f, {M(90), E(30)}, {R(3)}, L::Outpost);
			T.ScanReachAu = 4.0f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("XenologyLab"), LOCTEXT("XenologyLab", "XENOLOGY LAB"),
				LOCTEXT("XenologyLabRole", "Studies anomalies and what they leave: research from every find."),
				D::Science, C::Station, P::Orbit, 2, 3, 100.0f, {M(100), R(40)}, {R(4)}, L::Station);
			Types.Add(T);
		}

		// CIVIL AFFAIRS: people, government, diplomacy, rights.
		{
			FType T = Make(TEXT("OrbitalHabitat"), LOCTEXT("OrbitalHabitat", "ORBITAL HABITAT"),
				LOCTEXT("OrbitalHabitatRole", "Homes in orbit: the civilization grows and gains influence."),
				D::CivilAffairs, C::Station, P::Orbit, 2, 0, 80.0f, {M(90), E(30)}, {I(3)}, L::Station);
			T.LimitPerSite = 2;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("AdministrationHub"), LOCTEXT("AdministrationHub", "ADMINISTRATION HUB"),
				LOCTEXT("AdministrationHubRole", "Governs a star system: claims it and carries the network 2 AU."),
				D::CivilAffairs, C::Relay, P::StarSystem, 2, 1, 90.0f, {M(80), I(30)}, {I(2)}, L::Headquarters);
			T.bClaims = true;
			T.RelayReachAu = 2.0f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("DiplomaticEmbassy"), LOCTEXT("DiplomaticEmbassy", "DIPLOMATIC EMBASSY"),
				LOCTEXT("DiplomaticEmbassyRole", "A neutral meeting place for first contact and treaties."),
				D::CivilAffairs, C::Station, P::Orbit, 2, 2, 100.0f, {M(60), I(40)}, {I(4)}, L::Headquarters);
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("CivicForum"), LOCTEXT("CivicForum", "CIVIC FORUM"),
				LOCTEXT("CivicForumRole", "Civil rights and public services: crews everywhere work 5% faster."),
				D::CivilAffairs, C::Outpost, P::Surface, 2, 1, 70.0f, {M(50), I(20)}, {I(2)}, L::Outpost);
			T.SurveySpeed = 0.05f;
			T.BuildSpeed = 0.05f;
			Types.Add(T);
		}

		// MILITARY: guard worlds and the edges of systems.
		{
			FType T = Make(TEXT("DefencePlatform"), LOCTEXT("DefencePlatform", "DEFENCE PLATFORM"),
				LOCTEXT("DefencePlatformRole", "Guards a world's orbit."),
				D::Military, C::Station, P::Orbit, 1, 0, 60.0f, {M(80), E(20)}, {}, L::Station);
			T.LimitPerSite = 3;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("SensorPicket"), LOCTEXT("SensorPicket", "SENSOR PICKET"),
				LOCTEXT("SensorPicketRole", "Watches a star system's edge: early warning, scans 1.5 AU around."),
				D::Military, C::Relay, P::StarSystem, 1, 0, 45.0f, {M(40), E(10)}, {}, L::Beacon);
			T.ScanReachAu = 1.5f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("Garrison"), LOCTEXT("Garrison", "GARRISON"),
				LOCTEXT("GarrisonRole", "Troops and patrol craft on a world."),
				D::Military, C::Outpost, P::Surface, 1, 2, 80.0f, {M(70), I(10)}, {}, L::Outpost);
			Types.Add(T);
		}

		// FLEET COMMAND: keep the fleet flying.
		{
			// Rio 07-09.10, ORIGIN ladder (T-07): the first yard, on the ground, where the colony's first hull is laid down (a
			// shipyard of the fleet: GetShipyards lists it). The monument's LAUNCH opens it; elsewhere just a surface yard.
			FType T = Make(TEXT("LaunchYard"), LOCTEXT("LaunchYard", "LAUNCH YARD"),
				LOCTEXT("LaunchYardRole", "A yard on the ground that lays down the colony's first hulls: ships are ordered here."),
				D::Industry, C::Outpost, P::Surface, 0, 0, 60.0f, {M(120), E(40)}, {}, L::Shipyard);
			T.RequiresToken = APSProgressionTokens::Launch();
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("RepairDock"), LOCTEXT("RepairDock", "REPAIR DOCK"),
				LOCTEXT("RepairDockRole", "Repairs and refits: work at this world a quarter faster, the fleet 5% faster."),
				D::FleetCommand, C::Station, P::Orbit, 1, 0, 70.0f, {M(90), E(30)}, {}, L::Shipyard);
			T.LocalWorkSpeed = 0.25f;
			T.ShipSpeed = 0.05f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("SupplyDepot"), LOCTEXT("SupplyDepot", "SUPPLY DEPOT"),
				LOCTEXT("SupplyDepotRole", "Fuel and spares: the whole fleet flies 5% faster."),
				D::FleetCommand, C::Station, P::Orbit, 1, 0, 50.0f, {M(50), V(20)}, {}, L::Station);
			T.ShipSpeed = 0.05f;
			T.LimitPerSite = 2;
			Types.Add(T);
		}

		// TRANSPORT: move goods, link systems.
		{
			FType T = Make(TEXT("CargoHub"), LOCTEXT("CargoHub", "CARGO HUB"),
				LOCTEXT("CargoHubRole", "Moves goods between the worlds of a system: metals and energy, construction 5% faster."),
				D::Transport, C::Transport, P::Orbit, 1, 1, 70.0f, {M(80), E(20)}, {M(2), E(1)}, L::Station);
			T.BuildSpeed = 0.05f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("HyperspaceRelay"), LOCTEXT("HyperspaceRelay", "HYPERSPACE RELAY"),
				LOCTEXT("HyperspaceRelayRole", "Links star systems: claims the system and carries the network 4 AU."),
				D::Transport, C::Relay, P::StarSystem, 2, 1, 90.0f, {M(100), E(60)}, {}, L::Beacon);
			T.bClaims = true;
			T.RelayReachAu = 4.0f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("MassDriver"), LOCTEXT("MassDriver", "MASS DRIVER"),
				LOCTEXT("MassDriverRole", "Throws cargo from the surface to orbit: more metals from this world."),
				D::Transport, C::Transport, P::Surface, 1, 2, 80.0f, {M(90), E(40)}, {M(3)}, L::Outpost);
			T.RequiresAtSite = FName(TEXT("MiningOutpost"));
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("JumpGate"), LOCTEXT("JumpGate", "JUMP GATE"),
				LOCTEXT("JumpGateRole", "A gate to the other gates: the network reaches 8 AU and the fleet flies 15% faster."),
				D::Transport, C::Megastructure, P::StarSystem, 2, 3, 300.0f, {M(600), E(400), R(200)}, {}, L::Headquarters);
			T.bClaims = true;
			T.RelayReachAu = 8.0f;
			T.ShipSpeed = 0.15f;
			T.bMegastructure = true;
			T.bNeedsUnlock = true;
			T.VisualScale = 3.0f;
			Types.Add(T);
		}

		// HUBS (Rio 03.10: "space hubs: not the stations we usually build, huge, enormous ones", the hand-made level's own
		// hub stations at their authored size): a high orbit of a world, logistics for the whole system, berths at the world.
		{
			FType T = Make(TEXT("SpaceHub"), LOCTEXT("SpaceHub", "SPACE HUB"),
				LOCTEXT("SpaceHubRole", "An 11 km hub over a world: docks, depots and crews for the whole system. Work in the system 20% faster, two more berths for stations at its world."),
				D::Transport, C::Hub, P::HighOrbit, 1, 2, 240.0f, {M(400), E(150), I(40)}, {M(4), E(2), I(3)}, L::SpaceHub);
			T.bNeedsStationHere = true;
			T.bFleetOnly = true;
			T.BuildSpeed = 0.10f;
			T.ShipSpeed = 0.05f;
			T.LocalWorkSpeed = 0.5f;
			T.SystemWorkSpeed = 0.2f;
			T.HubBerths = 2;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("GrandHub"), LOCTEXT("GrandHub", "GRAND HUB"),
				LOCTEXT("GrandHubRole", "A 67 km hub of hubs, the system's port and arsenal: the fleet 10% faster, work in the system 30% faster, three more berths."),
				D::Transport, C::Hub, P::HighOrbit, 2, 3, 420.0f, {M(900), E(400), R(60), I(120)}, {M(8), E(6), I(6)}, L::GrandHub);
			T.RequiresAtSite = FName(TEXT("SpaceHub"));
			T.bFleetOnly = true;
			T.BuildSpeed = 0.10f;
			T.ShipSpeed = 0.10f;
			T.SystemWorkSpeed = 0.3f;
			T.HubBerths = 3;
			Types.Add(T);
		}

		// MEGASTRUCTURES (Rio 03.10: a chain, as on the hand-made level: a station over the world > SPACE ELEVATOR >
		// ORBITAL RING > DYSON SWARM around the star > DYSON SPHERE; each step opens the next).
		{
			FType T = Make(TEXT("SpaceElevator"), LOCTEXT("SpaceElevator", "SPACE ELEVATOR"),
				LOCTEXT("SpaceElevatorRole", "A tower on the equator and a tether up to a counterweight in stationary orbit: cargo to orbit without rockets, work at the world 30% faster."),
				D::Industry, C::Megastructure, P::AroundWorld, 2, 3, 360.0f, {M(600), E(250), I(50)}, {M(10), I(2)}, L::SpaceElevator);
			T.bNeedsStationHere = true;
			T.bNeedsGround = true;
			T.bFleetOnly = true;
			T.LocalWorkSpeed = 0.3f;
			T.bMegastructure = true;
			T.bNeedsUnlock = true;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("OrbitalRing"), LOCTEXT("OrbitalRing", "ORBITAL RING"),
				LOCTEXT("OrbitalRingRole", "A ring around the world's equator, hung on the elevator: industry, docks and homes; construction 20% faster."),
				D::Industry, C::Megastructure, P::AroundWorld, 2, 3, 480.0f, {M(1000), E(400), R(80)}, {M(20), I(5)}, L::OrbitalRing);
			T.RequiresAtSite = FName(TEXT("SpaceElevator"));
			T.bFleetOnly = true;
			T.BuildSpeed = 0.20f;
			T.bMegastructure = true;
			T.bNeedsUnlock = true;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("DysonSwarm"), LOCTEXT("DysonSwarm", "DYSON SWARM SEGMENT"),
				LOCTEXT("DysonSwarmRole", "A ring of collectors around the star, built from the ring's yards: vast energy."),
				D::Industry, C::Megastructure, P::StarSystem, 2, 3, 420.0f, {M(900), R(150)}, {E(60)}, L::DysonSwarm);
			T.RequiresInSystem = FName(TEXT("OrbitalRing"));
			T.bFleetOnly = true;
			T.bMegastructure = true;
			T.bNeedsUnlock = true;
			T.LimitPerSite = 5;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("DysonSphere"), LOCTEXT("DysonSphere", "DYSON SPHERE"),
				LOCTEXT("DysonSphereRole", "The five swarm rings closed into a shell around the star: energy beyond counting, construction 25% and the fleet 10% faster."),
				D::Industry, C::Megastructure, P::StarSystem, 2, 3, 900.0f, {M(3000), E(800), R(400), I(100)}, {E(300), R(10)}, L::DysonSphere);
			T.RequiresAtSite = FName(TEXT("DysonSwarm"));
			T.RequiresAtSiteCount = 5;
			T.bFleetOnly = true;
			T.BuildSpeed = 0.25f;
			T.ShipSpeed = 0.10f;
			T.bMegastructure = true;
			Types.Add(T);
		}
		return Types;
	}
}

const TArray<APSInfrastructure::FType>& APSInfrastructure::Types()
{
	static const TArray<FType> Catalogue = APSInfrastructureCatalogLocal::Build();
	return Catalogue;
}

const APSInfrastructure::FType* APSInfrastructure::Find(const FName Id)
{
	return Types().FindByPredicate([Id](const FType& Type) { return Type.Id == Id; });
}

bool APSInfrastructure::GetChain(const FName Type, TArray<FName>& OutSteps)
{
	// Rio 03.10: the two chains of the hand-made level's structures, each step standing on the one before it.
	static const TArray<FName> Hubs = {FName(TEXT("SpaceHub")), FName(TEXT("GrandHub"))};
	static const TArray<FName> Megastructures = {FName(TEXT("SpaceElevator")), FName(TEXT("OrbitalRing")),
		FName(TEXT("DysonSwarm")), FName(TEXT("DysonSphere"))};
	OutSteps.Reset();
	for (const TArray<FName>* Chain : {&Hubs, &Megastructures})
	{
		if (Chain->Contains(Type))
		{
			OutSteps = *Chain;
			return true;
		}
	}
	return false;
}

void APSInfrastructure::DescribeRequirements(const FType& Type, TArray<FText>& OutLines)
{
	OutLines.Reset();
	if (Type.bNeedsStationHere)
	{
		OutLines.Add(LOCTEXT("RequiresStation", "REQUIRES AN ORBITAL STATION OVER THE WORLD"));
	}
	if (!Type.RequiresAtSite.IsNone())
	{
		const FType* Needed = Find(Type.RequiresAtSite);
		const FText Name = Needed ? Needed->Name : FText::FromName(Type.RequiresAtSite);
		const FText Where = Type.Placement == EPlacement::StarSystem ? LOCTEXT("WhereStar", "AROUND THE SAME STAR")
			: LOCTEXT("WhereWorld", "AT THE SAME WORLD");
		OutLines.Add(Type.RequiresAtSiteCount > 1
			? FText::Format(LOCTEXT("RequiresCount", "REQUIRES {0} x {1} {2}"), FText::AsNumber(Type.RequiresAtSiteCount), Name, Where)
			: FText::Format(LOCTEXT("RequiresAtSite", "REQUIRES {0} {1}"), Name, Where));
	}
	if (!Type.RequiresInSystem.IsNone())
	{
		const FType* Needed = Find(Type.RequiresInSystem);
		OutLines.Add(FText::Format(LOCTEXT("RequiresInSystem", "REQUIRES {0} IN THE STAR SYSTEM"),
			Needed ? Needed->Name : FText::FromName(Type.RequiresInSystem)));
	}
	if (Type.bNeedsGround)
	{
		OutLines.Add(LOCTEXT("RequiresGround", "REQUIRES SOLID GROUND ON THE EQUATOR (NO GIANTS)"));
	}
}

FText APSInfrastructure::ChainRefusal(const FType& Type, const FChainState& State)
{
	// Rio 03.10, chains: each step stands on the one before it (a station > hub > grand hub; a station > elevator > ring >
	// swarm > sphere).
	if (Type.bNeedsStationHere && !State.bStationHere)
	{
		return LOCTEXT("NeedsStationHere", "Requires an orbital station over this world first (any station, a hub or the home complex).");
	}
	if (!Type.RequiresAtSite.IsNone())
	{
		const FType* Needed = Find(Type.RequiresAtSite);
		const FText NeededName = Needed ? Needed->Name : FText::FromName(Type.RequiresAtSite);
		const int32 Wanted = FMath::Max(Type.RequiresAtSiteCount, 1);
		if (State.RequiredHere < Wanted)
		{
			return Wanted > 1
				? FText::Format(LOCTEXT("NeedsCountAtSite", "Requires {0} x {1} here first (now {2})."), FText::AsNumber(Wanted), NeededName,
					FText::AsNumber(State.RequiredHere))
				: FText::Format(LOCTEXT("NeedsAtSite", "Requires {0} here first."), NeededName);
		}
	}
	if (!Type.RequiresInSystem.IsNone() && State.RequiredInSystem <= 0)
	{
		const FType* Needed = Find(Type.RequiresInSystem);
		return FText::Format(LOCTEXT("NeedsInSystem", "Requires {0} in this star system first."),
			Needed ? Needed->Name : FText::FromName(Type.RequiresInSystem));
	}
	return FText::GetEmpty();
}

int32 APSInfrastructure::HubBerthsFor(const FType& Type, const TArray<FName>& StandingHere)
{
	// A hub's berths take more of the ordinary orbital stations at its world: not more hubs or megastructures.
	if (Type.Category != ECategory::Station || Type.Placement != EPlacement::Orbit)
	{
		return 0;
	}
	int32 Berths = 0;
	for (const FName& Id : StandingHere)
	{
		if (const FType* Standing = Find(Id))
		{
			Berths += FMath::Max(Standing->HubBerths, 0);
		}
	}
	return Berths;
}

FText APSInfrastructure::DepartmentName(const EDepartment Department)
{
	switch (Department)
	{
	case EDepartment::Exploration: return LOCTEXT("Exploration", "EXPLORATION");
	case EDepartment::Industry: return LOCTEXT("Industry", "INDUSTRY");
	case EDepartment::Science: return LOCTEXT("Science", "SCIENCE");
	case EDepartment::CivilAffairs: return LOCTEXT("CivilAffairs", "CIVIL AFFAIRS");
	case EDepartment::Military: return LOCTEXT("Military", "MILITARY");
	case EDepartment::FleetCommand: return LOCTEXT("FleetCommand", "FLEET COMMAND");
	case EDepartment::Transport: return LOCTEXT("Transport", "TRANSPORT");
	default: return FText::GetEmpty();
	}
}

FLinearColor APSInfrastructure::DepartmentColour(const EDepartment Department)
{
	switch (Department)
	{
	case EDepartment::Exploration: return FLinearColor(0.30f, 0.80f, 1.00f, 1.0f);
	case EDepartment::Industry: return FLinearColor(1.00f, 0.66f, 0.22f, 1.0f);
	case EDepartment::Science: return FLinearColor(0.70f, 0.55f, 1.00f, 1.0f);
	case EDepartment::CivilAffairs: return FLinearColor(0.30f, 0.95f, 0.60f, 1.0f);
	case EDepartment::Military: return FLinearColor(1.00f, 0.35f, 0.30f, 1.0f);
	case EDepartment::FleetCommand: return FLinearColor(0.95f, 0.85f, 0.35f, 1.0f);
	case EDepartment::Transport: return FLinearColor(0.35f, 0.95f, 0.95f, 1.0f);
	default: return FLinearColor::White;
	}
}

FText APSInfrastructure::CategoryName(const ECategory Category)
{
	switch (Category)
	{
	case ECategory::Outpost: return LOCTEXT("CategoryOutpost", "OUTPOST");
	case ECategory::Station: return LOCTEXT("CategoryStation", "STATION");
	case ECategory::Relay: return LOCTEXT("CategoryRelay", "RELAY");
	case ECategory::Transport: return LOCTEXT("CategoryTransport", "TRANSPORT");
	case ECategory::Megastructure: return LOCTEXT("CategoryMegastructure", "MEGASTRUCTURE");
	case ECategory::Hub: return LOCTEXT("CategoryHub", "HUB");
	default: return FText::GetEmpty();
	}
}

FText APSInfrastructure::PlacementName(const EPlacement Placement)
{
	switch (Placement)
	{
	case EPlacement::Orbit: return LOCTEXT("PlacementOrbit", "IN ORBIT OF A WORLD");
	case EPlacement::Surface: return LOCTEXT("PlacementSurface", "ON A WORLD'S SURFACE");
	case EPlacement::StarSystem: return LOCTEXT("PlacementStarSystem", "IN A STAR SYSTEM");
	case EPlacement::HighOrbit: return LOCTEXT("PlacementHighOrbit", "IN A HIGH ORBIT OF A WORLD");
	case EPlacement::AroundWorld: return LOCTEXT("PlacementAroundWorld", "AROUND A WORLD: EQUATOR TO ORBIT");
	default: return FText::GetEmpty();
	}
}

FText APSInfrastructure::ResourceName(const EResource Resource)
{
	switch (Resource)
	{
	case EResource::Metals: return LOCTEXT("Metals", "METALS");
	case EResource::Volatiles: return LOCTEXT("Volatiles", "VOLATILES");
	case EResource::Energy: return LOCTEXT("Energy", "ENERGY");
	case EResource::Research: return LOCTEXT("Research", "RESEARCH");
	case EResource::Influence: return LOCTEXT("Influence", "INFLUENCE");
	default: return FText::GetEmpty();
	}
}

FLinearColor APSInfrastructure::ResourceColour(const EResource Resource)
{
	switch (Resource)
	{
	case EResource::Metals: return FLinearColor(0.80f, 0.78f, 0.74f, 1.0f);
	case EResource::Volatiles: return FLinearColor(0.40f, 0.85f, 1.00f, 1.0f);
	case EResource::Energy: return FLinearColor(1.00f, 0.86f, 0.30f, 1.0f);
	case EResource::Research: return FLinearColor(0.72f, 0.58f, 1.00f, 1.0f);
	case EResource::Influence: return FLinearColor(0.35f, 0.95f, 0.60f, 1.0f);
	default: return FLinearColor::White;
	}
}

FText APSInfrastructure::DescribeAmounts(const TArray<FAmount>& Amounts)
{
	if (Amounts.IsEmpty()) return LOCTEXT("Nothing", "NOTHING");
	FString Text;
	for (const FAmount& Amount : Amounts)
	{
		if (!Text.IsEmpty()) Text += TEXT("  /  ");
		Text += APSUINumber::Number(FMath::RoundToInt(Amount.Value)).ToString() + TEXT(" ") + ResourceName(Amount.Resource).ToString();
	}
	return FText::FromString(Text);
}

#undef LOCTEXT_NAMESPACE
