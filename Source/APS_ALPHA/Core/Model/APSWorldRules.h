#pragma once

#include "CoreMinimal.h"

class UGeneratedWorld;
class UWorld;

/**
 * Rio 07–09.10 (Docs/Design/GAME_CONCEPT_ORIGIN_2026-10-07.md §2): the rules of a world — the mode preset (ORIGIN /
 * SANDBOX / SPACE TRIPS), the origin, reach, knowledge, goals, resources, others and the sky source — kept on
 * UGeneratedWorld as plain bytes, so every route and every save carries them. Zero everywhere is today's game: an older
 * save, an untouched menu and the SANDBOX preset all read the same. Plain enums: no UENUM, no UHT beyond the uint8
 * properties on the model.
 */
namespace APSWorldRules
{
	enum class EMode : uint8 { Sandbox = 0, Origin, SpaceTrips, Custom };
	enum class EOrigin : uint8 { None = 0, Ark, Adrift, UnderTheRing, Exodus };
	enum class EReach : uint8 { Open = 0, Ladder };
	/** AsToday: the home system Surveyed, the rest Catalogued (the game of 09.10). */
	enum class EKnowledge : uint8 { AsToday = 0, AllKnown, Unknown };
	enum class EGoals : uint8 { Civilization = 0, None };
	enum class EResources : uint8 { Normal = 0, Unlimited, Scarce };
	enum class EOthers : uint8 { Alone = 0, Traces, Neighbours, Unknown };
	enum class ESky : uint8 { Generated = 0, Real };

	/** Bits of UGeneratedWorld::AuthoredLocks (AUTHORED_WORLDS §1): a locked level is never rerolled. */
	enum ELock : uint8
	{
		LockGalaxy = 1,
		LockCluster = 2,
		LockHomeSystem = 4,
		LockStar = 8,
		LockBody = 16,
		LockAll = 31
	};

	struct FRules
	{
		EMode Mode{EMode::Sandbox};
		EOrigin Origin{EOrigin::None};
		EReach Reach{EReach::Open};
		EKnowledge Knowledge{EKnowledge::AsToday};
		EGoals Goals{EGoals::Civilization};
		EResources Resources{EResources::Normal};
		EOthers Others{EOthers::Alone};
		ESky Sky{ESky::Generated};
		FString AuthoredWorldId;
		uint8 AuthoredLocks{0};

		bool IsToday() const
		{
			return Mode == EMode::Sandbox && Origin == EOrigin::None && Reach == EReach::Open
				&& Knowledge == EKnowledge::AsToday && Goals == EGoals::Civilization
				&& Resources == EResources::Normal && Others == EOthers::Alone && Sky == ESky::Generated;
		}
	};

	/** The rules written on a model; all zeros (today) for null. */
	APS_ALPHA_API FRules Of(const UGeneratedWorld* World);
	/** The rules of the running game: UMainGameplayInstance::NewGeneratedWorld; today when there is none. */
	APS_ALPHA_API FRules OfGame(const UWorld* World);
	APS_ALPHA_API void Write(UGeneratedWorld& World, const FRules& Rules);

	/** Sets the knobs a preset stands for (concept §2): SANDBOX = today, ORIGIN = ladder + unknown sky + traces,
	 * SPACE TRIPS = no goals, open reach, all known. CUSTOM leaves the knobs alone. */
	APS_ALPHA_API void ApplyPreset(UGeneratedWorld& World, EMode Mode);

	/** The ladder applies: the world asks for it (Reach == Ladder) and aps.Origin.Enable is on. */
	APS_ALPHA_API bool IsLadder(const UWorld* World);
	/** SPACE TRIPS or any world without civilization goals. */
	APS_ALPHA_API bool IsTrip(const UWorld* World);

	APS_ALPHA_API const TCHAR* ModeName(EMode Mode);
	APS_ALPHA_API const TCHAR* OriginName(EOrigin Origin);
	APS_ALPHA_API const TCHAR* KnowledgeName(EKnowledge Knowledge);
	APS_ALPHA_API const TCHAR* ResourcesName(EResources Resources);
	APS_ALPHA_API const TCHAR* OthersName(EOthers Others);
	/** "MODE=1|ORIGIN=1|..." for logs and descriptors; empty for today's rules, so hashes of old worlds hold. */
	APS_ALPHA_API FString Describe(const FRules& Rules);
}
