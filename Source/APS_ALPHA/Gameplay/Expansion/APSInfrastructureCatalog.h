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
		Count
	};

	/** Where it stands: in orbit of a planet or moon, on its surface, or in a star system around its star. */
	enum class EPlacement : uint8
	{
		Orbit,
		Surface,
		StarSystem
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
		Beacon
	};

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
	};

	APS_ALPHA_API const TArray<FType>& Types();
	APS_ALPHA_API const FType* Find(FName Id);
	APS_ALPHA_API FText DepartmentName(EDepartment Department);
	APS_ALPHA_API FLinearColor DepartmentColour(EDepartment Department);
	APS_ALPHA_API FText CategoryName(ECategory Category);
	APS_ALPHA_API FText PlacementName(EPlacement Placement);
	APS_ALPHA_API FText ResourceName(EResource Resource);
	APS_ALPHA_API FLinearColor ResourceColour(EResource Resource);
	/** "120 METALS  /  40 ENERGY" for a cost or a yield. */
	APS_ALPHA_API FText DescribeAmounts(const TArray<FAmount>& Amounts);
}
