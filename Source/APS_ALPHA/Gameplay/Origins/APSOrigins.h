#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Model/APSWorldRules.h"

class UWorld;
enum class ECharSpawnPlace : uint8;

/**
 * Rio 07–09.10 (GAME_CONCEPT_ORIGIN §2.1): an origin as data — where the pilot starts, who left the traces, the order
 * the resources open in, which onboarding route runs. Four records now; a fifth is a record, not a branch.
 * Plain C++, read by the menu (names, descriptions) and by the start of a generated civilization.
 */
namespace APSOrigins
{
	struct FDefinition
	{
		APSWorldRules::EOrigin Id{APSWorldRules::EOrigin::None};
		/** Stable key for logs, routes and texts ("Ark"). */
		const TCHAR* Key{TEXT("")};
		FText Name;
		FText Subtitle;
		FText Description;
		/** Where the pilot starts (ECharSpawnPlace). */
		ECharSpawnPlace StartPlace;
		/** The order the five stocks open in (APSInfrastructure::EResource values), concept §5. */
		uint8 ResourceOrder[5]{0, 1, 2, 3, 4};
		/** Onboarding route id (UAPSQuestSubsystem); empty until the route exists. */
		const TCHAR* OnboardingRoute{TEXT("")};
		/** False while the origin's dependencies are missing (EXODUS waits for the galaxy modes). */
		bool bAvailable{true};
	};

	APS_ALPHA_API const TArray<FDefinition>& All();
	APS_ALPHA_API const FDefinition* Find(APSWorldRules::EOrigin Id);
}

/** Tokens of the ORIGIN ladder, kept in the mission board's Unlocked set (saved; concept §3, T-03). */
namespace APSProgressionTokens
{
	APS_ALPHA_API FName Launch();
	APS_ALPHA_API FName StellarDrive();
	APS_ALPHA_API FName GalaxyHulls();
	APS_ALPHA_API FName VehiclesRover();
	APS_ALPHA_API FName VehiclesHover();
	APS_ALPHA_API FName VehiclesDrone();
	APS_ALPHA_API FName ResMetals();
	APS_ALPHA_API FName ResVolatiles();
	APS_ALPHA_API FName ResResearch();
	APS_ALPHA_API FName ResInfluence();
	APS_ALPHA_API FName TabInfrastructure();
	APS_ALPHA_API FName TabFleet();
	APS_ALPHA_API FName TabMap();
	/** The RES_* token of a stock (APSInfrastructure::EResource value); NAME_None out of range. */
	APS_ALPHA_API FName ResourceToken(int32 Resource);
	/** The token in words for refusals and journal lines ("THE LAUNCH", "METALS"). */
	APS_ALPHA_API FText Title(FName Token);
	/** The journal line when it opens. */
	APS_ALPHA_API FText Opened(FName Token);
	/**
	 * Opens a token of the ladder: the mission board keeps it (saved), the stocks are recounted, the journal tells it under
	 * Category. False when there is no board, no token, or it was open already.
	 */
	APS_ALPHA_API bool Grant(const UWorld* World, FName Token, FName Category);
}
