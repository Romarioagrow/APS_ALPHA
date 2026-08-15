#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"
#include "APSCanonicalStellarDataset.generated.h"

/** One immutable canonical cluster record, independent of render LOD/instance index. */
USTRUCT()
struct FAPSCanonicalClusterSystemRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid StableId;

	UPROPERTY()
	int32 CanonicalIndex{INDEX_NONE};

	UPROPERTY()
	FVector ClusterLocalLocation{FVector::ZeroVector};

	UPROPERTY()
	FStarModel PrimaryStarModel;

	UPROPERTY()
	FStarSystemModel SystemModel;
};

/**
 * Finalized actor-free stellar dataset carried by UGeneratedWorld across level
 * travel. Preview and gameplay render different immutable prefixes, but neither
 * is allowed to regenerate or reinterpret these canonical records.
 */
USTRUCT()
struct FAPSCanonicalStellarDataset
{
	GENERATED_BODY()

	static constexpr uint32 CurrentVersion = 2u;

	UPROPERTY()
	uint32 Version{CurrentVersion};

	/** Fingerprint of model inputs only; render budget/focus/camera are excluded. */
	UPROPERTY()
	uint32 InputHash{0u};

	/** Fingerprint of the finalized descriptor and every canonical cluster record. */
	UPROPERTY()
	uint32 DatasetHash{0u};

	/** Monotonic within one editable UGeneratedWorld; preserved by DuplicateObject. */
	UPROPERTY()
	uint64 BuildSerial{0u};

	UPROPERTY()
	int32 WorldGenerationSeed{0};

	UPROPERTY()
	int32 GalaxyGenerationSeed{0};

	UPROPERTY()
	int64 GalaxyModeledStarCount{0};

	UPROPERTY()
	int32 GalaxySize{0};

	UPROPERTY()
	double GalaxyStarDensity{0.0};

	UPROPERTY()
	EGalaxyType GalaxyType{EGalaxyType::Elliptical};

	UPROPERTY()
	EGalaxyClass GalaxyClass{EGalaxyClass::E0};

	UPROPERTY()
	FVector GalaxyCatalogHalfExtent{FVector::ZeroVector};

	UPROPERTY()
	int32 ClusterGenerationSeed{0};

	UPROPERTY()
	int32 ClusterModeledCount{0};

	UPROPERTY()
	double ClusterDensity{0.0};

	UPROPERTY()
	FVector ClusterBounds{FVector::ZeroVector};

	UPROPERTY()
	EStarClusterType ClusterType{EStarClusterType::Unknown};

	UPROPERTY()
	EStarClusterSize ClusterSize{EStarClusterSize::Unknown};

	UPROPERTY()
	EStarClusterPopulation ClusterPopulation{EStarClusterPopulation::Unknown};

	UPROPERTY()
	EStarClusterComposition ClusterComposition{EStarClusterComposition::Unknown};

	UPROPERTY()
	int32 HomeCanonicalIndex{INDEX_NONE};

	UPROPERTY()
	FGuid HomeStableId;

	UPROPERTY()
	TArray<FAPSCanonicalClusterSystemRecord> ClusterRecords;

	UPROPERTY()
	bool bFinalized{false};

	bool IsUsable(const uint32 ExpectedInputHash) const
	{
		if (!bFinalized || Version != CurrentVersion || InputHash != ExpectedInputHash
			|| DatasetHash == 0u || GalaxyGenerationSeed <= 0
			|| GalaxyModeledStarCount <= 0 || ClusterGenerationSeed <= 0
			|| ClusterModeledCount <= 0 || ClusterRecords.Num() != ClusterModeledCount
			|| HomeCanonicalIndex < 0 || HomeCanonicalIndex >= ClusterRecords.Num()
			|| !HomeStableId.IsValid()
			|| ClusterRecords[HomeCanonicalIndex].StableId != HomeStableId)
		{
			return false;
		}

		TSet<FGuid> StableIds;
		StableIds.Reserve(ClusterRecords.Num());
		for (int32 RecordIndex = 0; RecordIndex < ClusterRecords.Num(); ++RecordIndex)
		{
			const FAPSCanonicalClusterSystemRecord& Record = ClusterRecords[RecordIndex];
			if (!Record.StableId.IsValid() || StableIds.Contains(Record.StableId)
				|| Record.CanonicalIndex != RecordIndex
				|| Record.SystemModel.StableId != Record.StableId
				|| !Record.PrimaryStarModel.Location.Equals(Record.ClusterLocalLocation, 0.001)
				|| !Record.SystemModel.Location.Equals(Record.ClusterLocalLocation, 0.001))
			{
				return false;
			}
			StableIds.Add(Record.StableId);
		}
		return true;
	}

	void ResetForInput(const uint32 NewInputHash, const int32 NewWorldSeed)
	{
		const uint64 NextBuildSerial = BuildSerial + 1u;
		*this = FAPSCanonicalStellarDataset{};
		InputHash = NewInputHash;
		WorldGenerationSeed = NewWorldSeed;
		BuildSerial = NextBuildSerial;
	}
};
