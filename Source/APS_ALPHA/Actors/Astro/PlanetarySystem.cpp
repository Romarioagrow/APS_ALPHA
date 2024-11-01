#include "PlanetarySystem.h"

void APlanetarySystem::SetStar(AStar* NewStar)
{
	this->Star = NewStar;
}

void APlanetarySystem::SetAmountOfPlanets(int Amount)
{
	this->AmountOfPlanets = Amount;
}

void APlanetarySystem::SetPlanetarySystemType(EPlanetarySystemType Type)
{
	this->PlanetarySystemType = Type;
}

void APlanetarySystem::SetOrbitDistributionType(EOrbitDistributionType NewOrbitDistributionType)
{
	this->OrbitDistributionType = NewOrbitDistributionType;
}

void APlanetarySystem::SetStarFullSpectralName(FName NewStarFullSpectralName)
{
	this->FullSpectralName = NewStarFullSpectralName;
}

void APlanetarySystem::SetPlanetsList(TArray<TSharedPtr<FPlanetData>> NewPlanetsList)
{
	this->PlanetsList = NewPlanetsList;
}
