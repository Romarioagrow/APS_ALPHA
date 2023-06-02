// Fill out your copyright notice in the Description page of Project Settings.


#include "StarSystem.h"

void AStarSystem::SetStarsAmount(int Amount)
{
	this->StarsAmount = Amount;
}

void AStarSystem::AddNewStar(AStar* NewStar)
{
	Stars.Add(NewStar);
}

void AStarSystem::SetStarSystemType(EStarSystemType Type)
{
	this->StarSystemType = Type;
}
