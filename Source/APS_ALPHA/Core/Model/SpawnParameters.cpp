#include "SpawnParameters.h"

void USpawnParameters::SanitizeForGeneration()
{
	CivilizationName.TrimStartAndEndInline();
	if (CivilizationName.IsEmpty())
	{
		CivilizationName = TEXT("APOSFERA CIVILIZATION");
	}

	FoundingPopulation = FMath::Clamp(FoundingPopulation, 1, 100000000);
	StartingCredits = FMath::Clamp<int64>(StartingCredits, 0, 2000000000LL);
	TechnologyLevel = FMath::Clamp(TechnologyLevel, 1, 10);
	StartingFleetSize = FMath::Clamp(StartingFleetSize, 1, MaxStartingFleetSize);
	StarOutposts = FMath::Clamp(StarOutposts, 0, MaxInfrastructurePerCategory);
	PlanetOutposts = FMath::Clamp(PlanetOutposts, 0, MaxInfrastructurePerCategory);
	OrbitalOutposts = FMath::Clamp(OrbitalOutposts, 1, MaxInfrastructurePerCategory);
	GroundOutposts = FMath::Clamp(GroundOutposts, 0, MaxInfrastructurePerCategory);
	ExplorationDivisionLevel = FMath::Clamp(ExplorationDivisionLevel, 0, 20);
	IndustryDivisionLevel = FMath::Clamp(IndustryDivisionLevel, 0, 20);
	ScienceDivisionLevel = FMath::Clamp(ScienceDivisionLevel, 0, 20);
	CivilAffairsDivisionLevel = FMath::Clamp(CivilAffairsDivisionLevel, 0, 20);
	MilitaryDivisionLevel = FMath::Clamp(MilitaryDivisionLevel, 0, 20);
	FleetDivisionLevel = FMath::Clamp(FleetDivisionLevel, 0, 20);
	GroundVehicleMask = FMath::Clamp(GroundVehicleMask, 0, 7);
}

void USpawnParameters::GetColonyStartModules(const EAPSColonyStartPackage Package, TArray<FName>& OutModules)
{
	OutModules.Reset();
	// Surface modules of APSColonyModuleCatalogue: living quarters and power first, then what makes the colony work.
	if (Package == EAPSColonyStartPackage::Standard || Package == EAPSColonyStartPackage::Settlement)
	{
		OutModules.Append({FName(TEXT("Habitat")), FName(TEXT("SolarArray")), FName(TEXT("Storage"))});
	}
	if (Package == EAPSColonyStartPackage::Settlement)
	{
		OutModules.Append({FName(TEXT("Greenhouse")), FName(TEXT("CommsMast")), FName(TEXT("Floodlight"))});
	}
}

int32 USpawnParameters::GetPlannedInfrastructureActorCount() const
{
	// The home station is represented by OrbitalOutposts and is supplemented by
	// star/planet surface nodes and ground settlements.
	return FMath::Max(0, StarOutposts)
		+ FMath::Max(0, PlanetOutposts)
		+ FMath::Max(1, OrbitalOutposts)
		+ FMath::Max(0, GroundOutposts);
}

int32 USpawnParameters::GetPlannedUnitCount() const
{
	// Rio 02.10: the manifest counts what the player commands (ships, rover / hover / drone), not actors.
	return FMath::Max(1, StartingFleetSize) + FMath::CountBits(static_cast<uint64>(FMath::Clamp(GroundVehicleMask, 0, 7)));
}

int32 USpawnParameters::GetPlannedStructureCount() const
{
	TArray<FName> FoundingModules;
	GetColonyStartModules(ColonyStartPackage, FoundingModules);
	return 2 + GetPlannedInfrastructureActorCount() + FoundingModules.Num();
}

int32 USpawnParameters::GetPlannedPhysicalActorCount() const
{
	// HQ and shipyard are the two fixed core actors. The home station and home
	// ship are already included in their corresponding requested totals.
	return 2 + FMath::Max(1, StartingFleetSize) + GetPlannedInfrastructureActorCount();
}
