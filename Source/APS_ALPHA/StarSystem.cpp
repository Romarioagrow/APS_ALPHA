// Fill out your copyright notice in the Description page of Project Settings.


#include "StarSystem.h"

void AStarSystem::SetStarsAmount(int Amount)
{
	this->StarsAmount = Amount;
}

void AStarSystem::AddPlanetarySystem(APlanetarySystem* PlanetarySystem)
{
	PlanetarySystems.Add(PlanetarySystem);
}
