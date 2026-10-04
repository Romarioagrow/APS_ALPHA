#pragma once

#include "CoreMinimal.h"

/**
 * Infrastructure catalogue (Rio 02.10: "many more structures by division: mining, science, civil affairs, diplomacy,
 * civil rights, military, fleet; transport structures, relays, megastructures; manage and build them from every
 * screen"). One table of structure types: the department that runs them, where they stand, what they need, how long
 * the construction ships work, what they cost, what they yield while they stand and what they change in the rules.
 * Construction ships raise them (fleet order BUILD at a planet, moon or star system); the colony's own small modules
 * stay in APSColonyModuleCatalogue (build mode).
 */
namespace APSInfrastructure
{
	/** The six divisions of the civilization, and transport (raised by industry, run by fleet command). */
	enum class EDepartment : uint8
	{
		Exploration,
		Industry,
		Science,
		CivilAffairs,
		Military,
		FleetCommand,
		Transport,
		Count
	};

	enum class ECategory : uint8
	{
		Outpost,
		Station,
		Relay,
		Transport,
		Megastructure,
		/** Rio 03.10: the huge hub stations of the hand-made level, a tier above the stations. */
		Hub,
		Count
	};

	/** Where it stands: in orbit of a planet or moon, on its surface, or in a star system around its star. */
	enum class EPlacement : uint8
	{
		Orbit,
		Surface,
		StarSystem,
		/** Rio 03.10, hubs: a high orbit of a planet or moon, placed by the runtime (never by hand). */
		HighOrbit,
		/** Rio 03.10, world megastructures: the space elevator (equator to stationary orbit) and the orbital ring (around
		 * the equator), placed by the world's own geometry (Gameplay/Megastructures). */
		AroundWorld
	};

	/** The civilization's stocks. */
	enum class EResource : uint8
	{
		Metals,
		Volatiles,
		Energy,
		Research,
		Influence,
		Count
	};

	struct FAmount
	{
		EResource Resource{EResource::Metals};
		float Value{0.0f};
	};

	/** The look of a built structure: the generator's existing actor families until each type has its own model. */
	enum class EVisual : uint8
	{
		Outpost,
		Station,
		Shipyard,
		Headquarters,
		Beacon,
		// Rio 03.10: the hand-made level's own meshes at their authored size (Gameplay/Megastructures). Appended.
		SpaceHub,
		GrandHub,
		SpaceElevator,
		OrbitalRing,
		DysonSwarm,
		DysonSphere
	};

	/** Hubs and megastructures: their look, place and construction stages come from Gameplay/Megastructures. */
	inline bool IsMegaVisual(const EVisual Visual)
	{
		return Visual >= EVisual::SpaceHub && Visual <= EVisual::DysonSphere;
	}

	struct FType
	{
		FName Id;
		FText Name;
		/** One line: what it is for. */
		FText Role;
		EDepartment Department{EDepartment::Industry};
		ECategory Category{ECategory::Station};
		EPlacement Placement{EPlacement::Orbit};
		/**
		 * What the place must be known to: at a world the survey level (1 surveyed, 2 studied), in a star system its
		 * knowledge (1 scanned, 2 surveyed).
		 */
		uint8 RequiredKnowledge{1};
		/** A structure of the civilization that must stand at the same place first (NAME_None: none). */
		FName RequiresAtSite;
		/** The department level the work needs. */
		int32 RequiredLevel{0};
		float BuildSeconds{60.0f};
		TArray<FAmount> Cost;
		/** Per minute while it stands. */
		TArray<FAmount> Yield;
		/** How many may stand at one place. */
		int32 LimitPerSite{1};
		/** Claims the star system it stands in (beacons, relays, administrations). */
		bool bClaims{false};
		/** Reach of a relay in the network, AU (0: none). */
		float RelayReachAu{0.0f};
		/** Scans the star systems within this reach, AU (deep-space scanners, observatories). */
		float ScanReachAu{0.0f};
		/** Rule changes while it stands, summed over the civilization: +0.1 = 10% faster. */
		float SurveySpeed{0.0f};
		float BuildSpeed{0.0f};
		float ShipSpeed{0.0f};
		/** Rule changes at its own place only: work there is this much faster. */
		float LocalWorkSpeed{0.0f};
		EVisual Visual{EVisual::Station};
		/** Megastructures and gates: shown as such and built in stages (BuildSeconds is the whole). */
		bool bMegastructure{false};
		/** Only at a gas or ice giant (harvesters). */
		bool bGiantOnly{false};
		/** Needs an unlock from a department mission first (APSMissions). */
		bool bNeedsUnlock{false};
		/** The look's size against the family's model (megastructures are larger). */
		float VisualScale{1.0f};

		// Chains (Rio 03.10: "megastructures buildable in chains, a space elevator, then a space ring, one after another").
		/** How many of RequiresAtSite must stand there (the Dyson sphere wants its swarm segments). */
		int32 RequiresAtSiteCount{1};
		/** A structure that must stand somewhere in the same star system first (NAME_None: none). */
		FName RequiresInSystem;
		/** An orbital station of the civilization over this world first: a catalogue station or hub, or the fleet's own. */
		bool bNeedsStationHere{false};
		/** Anchored in solid ground: never at a gas or ice giant (the space elevator). */
		bool bNeedsGround{false};
		/** Raised only by construction ships, in stages (hubs, megastructures): build mode cannot place it by hand. */
		bool bFleetOnly{false};
		/** Hubs: berths for this many more of every orbital station type at their world. */
		int32 HubBerths{0};
		/** Hubs: work anywhere in the hub's star system is this much faster (+0.2 = 20%). */
		float SystemWorkSpeed{0.0f};
	};

	APS_ALPHA_API const TArray<FType>& Types();
	APS_ALPHA_API const FType* Find(FName Id);
	/**
	 * The build chain a type belongs to, in order (Rio 03.10): ORBITAL STATION > SPACE HUB > GRAND HUB, and
	 * ORBITAL STATION > SPACE ELEVATOR > ORBITAL RING > DYSON SWARM SEGMENT > DYSON SPHERE. The station step is not a
	 * catalogue type (any station counts) and is left out. False when the type is in no chain.
	 */
	APS_ALPHA_API bool GetChain(FName Type, TArray<FName>& OutSteps);
	/** What a type needs before it, one line each ("REQUIRES SPACE ELEVATOR ON THE SAME WORLD"); empty when nothing. */
	APS_ALPHA_API void DescribeRequirements(const FType& Type, TArray<FText>& OutLines);

	/** What a place holds of a type's chain needs, as the runtime counts it (or a test sets it). */
	struct FChainState
	{
		/** An orbital station of the civilization over the world. */
		bool bStationHere{false};
		/** How many of RequiresAtSite stand at the place, and of RequiresInSystem in its star system. */
		int32 RequiredHere{0};
		int32 RequiredInSystem{0};
	};
	/** The first chain need the place lacks ("Requires SPACE ELEVATOR here first."); empty when it has them all. */
	APS_ALPHA_API FText ChainRefusal(const FType& Type, const FChainState& State);
	/** Hubs: the extra berths the types standing at a place give this type there (ordinary orbital stations only). */
	APS_ALPHA_API int32 HubBerthsFor(const FType& Type, const TArray<FName>& StandingHere);
	APS_ALPHA_API FText DepartmentName(EDepartment Department);
	APS_ALPHA_API FLinearColor DepartmentColour(EDepartment Department);
	APS_ALPHA_API FText CategoryName(ECategory Category);
	APS_ALPHA_API FText PlacementName(EPlacement Placement);
	APS_ALPHA_API FText ResourceName(EResource Resource);
	APS_ALPHA_API FLinearColor ResourceColour(EResource Resource);
	/** "120 METALS  /  40 ENERGY" for a cost or a yield. */
	APS_ALPHA_API FText DescribeAmounts(const TArray<FAmount>& Amounts);
}
