#pragma once

#include "CoreMinimal.h"

class UGameSave;
class UGeneratedWorld;

/** Serialization and legacy migration for the actor-free procedural world model. */
namespace APSWorldSaveSnapshot
{
	constexpr int32 LatestSaveFormatVersion = 2;

	/** Captures every reflected model input, edit and canonical stellar record. */
	bool Capture(const UGeneratedWorld* WorldModel, TArray<uint8>& OutBytes);

	/**
	 * Restores the tagged snapshot when present, otherwise migrates the legacy
	 * FGeneratedWorldData summary into a deterministic replay model.
	 */
	UGeneratedWorld* Restore(const UGameSave* Save, UObject* Outer, const FString& SlotName);
}
