#pragma once

#include "CoreMinimal.h"

class FConfigFile;
class UGameSave;
class UGeneratedWorld;
class UWorld;
struct FGeneratedWorldData;

/** One planet of a saved world's home system, as the .apsmeta sidecar records it for the world browser. */
struct FAPSWorldPlanetRecord
{
	/** EPlanetType display name ("Forest Planet"; the bare enum name in builds without editor metadata). */
	FString Type;
	double OrbitAu{0.0};
	int32 RadiusKm{0};
	int32 Moons{0};
	/** 0 = the home star; the companions of a multiple system follow in their own order. */
	int32 Star{0};
	bool bInhabited{false};
};

/** The true home system of a save (sidecar version 2), read from the live hierarchy when the game saved it. */
struct FAPSWorldSystemRecord
{
	/** ESpectralClass display name of the home star ("K - Orange"). */
	FString StarClassLabel;
	/** EStellarType display name ("Main Sequence", "Super Giant"). */
	FString StellarType;
	FString HomeStarName;
	FString HomePlanetName;
	int32 StarCount{1};
	/** Index into Planets; INDEX_NONE when the home world is not one of them. */
	int32 HomePlanetIndex{INDEX_NONE};
	/** Home star first, each star's planets in orbit order. */
	TArray<FAPSWorldPlanetRecord> Planets;
};

/**
 * The world browser's metadata sidecar, Saved/SaveGames/<Slot>.apsmeta: a few lines the browser can read for ~230
 * saves without opening an 80 MB .sav. Rio 03.10: the version 1 fields echoed the menu model's editor buffer (a G star
 * and one frozen planet in almost every save), so the game now records the live home system (version 2). The old keys
 * stay, now with true values, so older builds still read new sidecars; new keys are optional for older sidecars.
 */
namespace APSWorldBrowserMetadata
{
	FString SidecarPath(const FString& SlotName);

	/** Writes the sidecar of a world the game has just saved. LiveWorld supplies the true home system (its home
	 * planet comes from Model or the live generator); without it only the version 1 fields are written. */
	bool WriteForSave(const UGameSave* Save, const FGeneratedWorldData& WorldData, const UWorld* LiveWorld,
		const UGeneratedWorld* Model);

	/** Reads a sidecar into OutMetadata (also the ones written before 04.10, whose planet list FConfigFile::Read cut). */
	bool ReadSidecar(const FString& Path, FConfigFile& OutMetadata);

	/** The version 2 system record of an already read sidecar; false for sidecars written before it existed. */
	bool ReadSystemRecord(const FConfigFile& Metadata, FAPSWorldSystemRecord& OutRecord);
}
