#include "APSInfrastructureCatalog.h"

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

		// MEGASTRUCTURES.
		{
			FType T = Make(TEXT("OrbitalRing"), LOCTEXT("OrbitalRing", "ORBITAL RING"),
				LOCTEXT("OrbitalRingRole", "A ring around a world: industry, docks and homes; construction 20% faster."),
				D::Industry, C::Megastructure, P::Orbit, 2, 3, 360.0f, {M(800), E(300)}, {M(20), I(5)}, L::Headquarters);
			T.BuildSpeed = 0.20f;
			T.bMegastructure = true;
			T.bNeedsUnlock = true;
			T.VisualScale = 4.0f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("DysonSwarm"), LOCTEXT("DysonSwarm", "DYSON SWARM SEGMENT"),
				LOCTEXT("DysonSwarmRole", "Collectors around the star: vast energy."),
				D::Industry, C::Megastructure, P::StarSystem, 2, 3, 420.0f, {M(900), R(150)}, {E(60)}, L::Station);
			T.bMegastructure = true;
			T.bNeedsUnlock = true;
			T.LimitPerSite = 5;
			T.VisualScale = 4.0f;
			Types.Add(T);
		}
		{
			FType T = Make(TEXT("SpaceElevator"), LOCTEXT("SpaceElevator", "SPACE ELEVATOR"),
				LOCTEXT("SpaceElevatorRole", "From the surface to orbit without rockets: metals and influence."),
				D::Industry, C::Megastructure, P::Surface, 2, 3, 300.0f, {M(500), E(200)}, {M(10), I(2)}, L::Headquarters);
			T.bMegastructure = true;
			T.bNeedsUnlock = true;
			T.VisualScale = 2.5f;
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
