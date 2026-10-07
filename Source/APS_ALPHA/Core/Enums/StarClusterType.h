#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EStarClusterType : uint8
{
	OpenCluster = 0 	UMETA(DisplayName = "Open / Compact"),
	GlobularCluster = 1 UMETA(DisplayName = "Globular"),
	Supercluster = 2 	UMETA(DisplayName = "Supercluster"),
	Nebula = 3 			UMETA(DisplayName = "Spiral Nebula"),
	// Keep Unknown at its historic serialized value. New authored formations are
	// appended so existing Blueprint/save enum bytes remain compatible.
	Unknown = 4			UMETA(DisplayName = "Unknown"),
	ElongatedStream = 5 UMETA(DisplayName = "Elongated Stream"),
	RingArc = 6 			UMETA(DisplayName = "Ring / Arc"),
	Hourglass = 7 		UMETA(DisplayName = "Hourglass"),
	// Rio 03.10: further formations (UStarClusterGenerator::SampleSeededFormation), appended.
	YoungAssociation = 8	UMETA(DisplayName = "Young Association"),
	MovingGroup = 9			UMETA(DisplayName = "Moving Group"),
	SuperStarCluster = 10	UMETA(DisplayName = "Super Star Cluster"),
	EmbeddedCluster = 11	UMETA(DisplayName = "Embedded / Filaments"),
	DoubleCluster = 12		UMETA(DisplayName = "Double Cluster")
}; 
