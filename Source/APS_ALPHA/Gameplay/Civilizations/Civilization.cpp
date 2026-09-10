// Fill out your copyright notice in the Description page of Project Settings.


#include "Civilization.h"

UCivilization::UCivilization()
{
}

void UCivilization::InitializeFromSpawnParameters(const USpawnParameters* Parameters)
{
	if (!Parameters)
	{
		return;
	}

	Name = Parameters->CivilizationName;
	Archetype = Parameters->CivilizationArchetype;
	Government = Parameters->GovernmentType;
	Economy = Parameters->EconomicSystem;
	Society = Parameters->SocietyType;
	Population = FMath::Max(1, Parameters->FoundingPopulation);
	Credits = FMath::Max<int64>(0, Parameters->StartingCredits);
	TechnologyLevel = FMath::Clamp(Parameters->TechnologyLevel, 1, 10);
	FleetSize = FMath::Clamp(Parameters->StartingFleetSize, 1,
		USpawnParameters::MaxStartingFleetSize);

	Infrastructure.StarOutposts = FMath::Clamp(Parameters->StarOutposts, 0,
		USpawnParameters::MaxInfrastructurePerCategory);
	Infrastructure.PlanetOutposts = FMath::Clamp(Parameters->PlanetOutposts, 0,
		USpawnParameters::MaxInfrastructurePerCategory);
	Infrastructure.OrbitalStations = FMath::Clamp(Parameters->OrbitalOutposts, 1,
		USpawnParameters::MaxInfrastructurePerCategory);
	Infrastructure.GroundSettlements = FMath::Clamp(Parameters->GroundOutposts, 0,
		USpawnParameters::MaxInfrastructurePerCategory);

	Divisions.Exploration = FMath::Clamp(Parameters->ExplorationDivisionLevel, 0, 20);
	Divisions.Industry = FMath::Clamp(Parameters->IndustryDivisionLevel, 0, 20);
	Divisions.Science = FMath::Clamp(Parameters->ScienceDivisionLevel, 0, 20);
	Divisions.CivilAffairs = FMath::Clamp(Parameters->CivilAffairsDivisionLevel, 0, 20);
	Divisions.Military = FMath::Clamp(Parameters->MilitaryDivisionLevel, 0, 20);
	Divisions.FleetCommand = FMath::Clamp(Parameters->FleetDivisionLevel, 0, 20);
}

FleetDivision::FleetDivision()
{
}
