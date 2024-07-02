#pragma once
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EStarClusterType : uint8
{
	OpenCluster 	UMETA(DisplayName = "Open Cluster"),
	GlobularCluster UMETA(DisplayName = "Globular Cluster"),
	Supercluster 	UMETA(DisplayName = "Supercluster"),
	Nebula 			UMETA(DisplayName = "Nebula"),
	Unknown			UMETA(DisplayName = "Unknown")
	// ,Embedded 
}; 
