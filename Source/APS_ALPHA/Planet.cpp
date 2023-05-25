// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"
#include "PlanetGenerationModel.h"

APlanet::APlanet()
{
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	RootComponent = SphereCollisionComponent;
}

void APlanet::AddMoon(AMoon* Moon)
{
	Moons.Add(Moon);
}

void APlanet::SetPlanetType(EPlanetType NewPlanetType)
{
	this->PlanetType = NewPlanetType;
}

void APlanet::SetPlanetZone(EPlanetaryZoneType NewPlanetZone)
{
	this->PlanetZone = NewPlanetZone;
}

void APlanet::SetPlanetDensity(double NewPlanetDensity)
{
	this->PlanetDensity = NewPlanetDensity;
}

void APlanet::SetPlanetGravityStrength(double NewPlanetGravityStrength)
{
	this->PlanetGravityStrength = NewPlanetGravityStrength;
}

void APlanet::SetTemperature(double NewTemperature)
{
	this->Temperature = NewTemperature;
}

void APlanet::SetAmountOfMoons(int NewAmountOfMoons)
{
	this->AmountOfMoons = NewAmountOfMoons;
}

void APlanet::SetMoonsList(TArray<FMoonData> NewMoonsList)
{
	this->MoonsList = NewMoonsList;
}

void APlanet::SetParentStar(AStar* Star)
{
	this->ParnetStar = Star;
}
