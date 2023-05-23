// Fill out your copyright notice in the Description page of Project Settings.


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

void APlanetarySystem::SetStarSpectralClass(ESpectralClass NewStarSpectralClass)
{
	this->StarSpectralClass = NewStarSpectralClass;
}
