#include "APSShipCatalog.h"

bool UAPSShipCatalog::IsStartingFleetCandidate(const FAPSShipCatalogEntry& Entry) const
{
	const uint8 Size = static_cast<uint8>(Entry.SizeClass);
	return Entry.ShipClass
		&& Entry.bAllowInStartingFleet
		&& Entry.Weight > 0.0f
		&& Size >= static_cast<uint8>(StartingFleetMinSizeClass)
		&& Size <= static_cast<uint8>(StartingFleetMaxSizeClass);
}

int32 UAPSShipCatalog::PickStartingFleetIndex(FRandomStream& Stream) const
{
	float TotalWeight = 0.0f;
	int32 LastCandidate = INDEX_NONE;
	for (int32 Index = 0; Index < Ships.Num(); ++Index)
	{
		if (IsStartingFleetCandidate(Ships[Index]))
		{
			TotalWeight += Ships[Index].Weight;
			LastCandidate = Index;
		}
	}
	if (LastCandidate == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	float Roll = Stream.FRandRange(0.0f, TotalWeight);
	for (int32 Index = 0; Index < Ships.Num(); ++Index)
	{
		if (!IsStartingFleetCandidate(Ships[Index]))
		{
			continue;
		}
		Roll -= Ships[Index].Weight;
		if (Roll <= 0.0f)
		{
			return Index;
		}
	}
	// Floating-point remainder lands on the last candidate.
	return LastCandidate;
}

TSubclassOf<ASpaceship> UAPSShipCatalog::PickStartingFleetShip(FRandomStream& Stream) const
{
	const int32 Index = PickStartingFleetIndex(Stream);
	return Ships.IsValidIndex(Index) ? Ships[Index].ShipClass : nullptr;
}
