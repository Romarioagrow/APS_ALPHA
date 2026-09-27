#pragma once

#include "CoreMinimal.h"

class UGameSave;
class UGeneratedWorld;
class USpawnParameters;

/** Serialization and legacy migration for the actor-free procedural world model. */
namespace APSWorldSaveSnapshot
{
	constexpr int32 LatestSaveFormatVersion = 3;

	/** Captures every reflected model input, edit and canonical stellar record. */
	bool Capture(const UGeneratedWorld* WorldModel, TArray<uint8>& OutBytes);

	/**
	 * Restores the tagged snapshot when present, otherwise migrates the legacy
	 * FGeneratedWorldData summary into a deterministic replay model.
	 */
	UGeneratedWorld* Restore(const UGameSave* Save, UObject* Outer, const FString& SlotName);

	/** Captures the exact civilization recipe used to materialize gameplay. */
	bool CaptureSpawnParameters(const USpawnParameters* Parameters, TArray<uint8>& OutBytes);

	/** Restores a version-tolerant, Outer-owned civilization recipe. */
	USpawnParameters* RestoreSpawnParameters(const UGameSave* Save, UObject* Outer);
}
