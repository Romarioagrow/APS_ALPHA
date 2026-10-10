#pragma once

#include "CoreMinimal.h"

class UGeneratedWorld;
class USpawnParameters;
namespace APSWorldRoll { enum class EScope : uint8; }

/**
 * Rio 09.10 (Docs/Design/AUTHORED_WORLDS_2026-10.md): authored worlds — presets of the generator with one seed, a name,
 * lore and locks, the game's "single" instead of hand-made maps. A preset is a folder under
 * Content/APS/APS_ALPHA/Worlds/Authored/<id>/ with card.json (name, subtitle, lore, tags, locks, origin, route, spawn)
 * and world.json (the reflected fields of UGeneratedWorld to set; everything absent keeps the model's value). Plain C++.
 */
namespace APSAuthoredWorlds
{
	struct FCard
	{
		FString Id;
		FString Name;
		FString Subtitle;
		FString Lore;
		/** Short place line under the card ("TUNDRA MOON · K STAR"). */
		FString Place;
		TArray<FString> Tags;
		/** APSWorldRules::ELock bits REGENERATE must keep. */
		uint8 Locks{0};
		/** APSWorldRules::EOrigin value the card recommends; 0 none. */
		uint8 RecommendedOrigin{0};
		/** The SPACE route (no civilization) instead of CIVILIZATION. */
		bool bSpaceRoute{false};
		/** ECharSpawnPlace value for the pilot; INDEX_NONE keeps the menu's choice. */
		int32 SpawnPlace{INDEX_NONE};
		bool bPlayable{true};
		/** "ready" or why not ("needs nebulae"); shown on the card. */
		FString Readiness;
		FString Directory;
		bool bHasWorld{false};
	};

	/** Content/APS/APS_ALPHA/Worlds/Authored, absolute. */
	APS_ALPHA_API FString Root();
	/** The cards found under Root, sorted by their "order" then id; scanned once, rescanned on request. */
	APS_ALPHA_API const TArray<FCard>& Catalogue(bool bRescan = false);
	APS_ALPHA_API const FCard* Find(const FString& Id);

	/** Applies world.json onto the model and writes the card's id and locks; the spawn place onto Spawn when the card
	 * sets one. False (with a reason) when the file is missing or malformed; the model is then left untouched. */
	APS_ALPHA_API bool Apply(const FCard& Card, UGeneratedWorld& World, USpawnParameters* Spawn, FString* OutFailure = nullptr);

	/** Writes the model's reflected fields to <Root>/<Id>/world.json (actor pointers and the canonical dataset left out)
	 * and a card.json stub when none exists. The console command aps.Worlds.ExportAuthored <id> calls it on the menu model. */
	APS_ALPHA_API bool Export(const UGeneratedWorld& World, const FString& Id, FString& OutPath);

	/** The lock bits a REGENERATE scope would touch. */
	APS_ALPHA_API uint8 LockBitsOf(APSWorldRoll::EScope Scope);
	/** The model came from an authored world and that world locks a level the scope would reroll. */
	APS_ALPHA_API bool IsScopeLocked(const UGeneratedWorld& World, APSWorldRoll::EScope Scope);
}
