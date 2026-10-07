#pragma once

#include "CoreMinimal.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Identity includes the source UObject serial and topology; an index is never ownership. */
struct FAPSGameplayStellarKey
{
	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> Source;
	FGuid StableId;
	uint64 BuildSerial{0};
	int32 Index{INDEX_NONE};
	int32 InstanceCount{0};
	bool operator==(const FAPSGameplayStellarKey& Other) const
	{
		return Source == Other.Source && StableId == Other.StableId
			&& BuildSerial == Other.BuildSerial && Index == Other.Index
			&& InstanceCount == Other.InstanceCount;
	}
	friend uint32 GetTypeHash(const FAPSGameplayStellarKey& Key)
	{
		return HashCombine(HashCombine(GetTypeHash(Key.Source), GetTypeHash(Key.StableId)),
			HashCombine(GetTypeHash(Key.BuildSerial), HashCombine(GetTypeHash(Key.Index),
				GetTypeHash(Key.InstanceCount))));
	}
};

enum class EAPSGameplayStellarSuppression : uint8
{
	None = 0,
	Materialized = 1,
	SafetyExclusion = 2,
	UnclassifiedExternal = 4
};

struct FAPSGameplayNativeDemand
{
	FAPSGameplayStellarKey Key;
	FTransform BaseTransform{FTransform::Identity};
	TWeakObjectPtr<UStaticMesh> SourceMesh;
	double PhysicalRadiusCm{0.0};
	double PixelRadius{0.0};
};

/** Sparse view ownership: only a ready pair may own the matching point's zero. */
struct FAPSGameplayNativePair
{
	FAPSGameplayNativeDemand Demand;
	TWeakObjectPtr<UStaticMeshComponent> Photosphere;
	TWeakObjectPtr<UStaticMeshComponent> Corona;
	FTransform LastPublishedPoint{FTransform::Identity};
	uint64 BoundMutationSerial{0};
	uint64 BoundFrame{0};
	bool bAssigned{false};
	bool bBound{false};
	bool bOwnsPoint{false};
	bool bShowCorona{true};
};

namespace APSGameplayNativeStars
{
// Catalog/model radius in physical centimetres; independent of HISM impostor scale.
double PhysicalRadiusCm(const FAPSGameplayStellarKey& Key);
// Rio 03.10: false while aps.Stars.NativeMode draws every resolved star as an instanced photosphere (no view selection).
bool UsesViewSelection();
}
