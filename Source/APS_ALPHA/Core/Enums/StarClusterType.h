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
	Hourglass = 7 		UMETA(DisplayName = "Hourglass")
	// ,Embedded 
}; 
