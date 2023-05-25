// Fill out your copyright notice in the Description page of Project Settings.


#include "Moon.h"

void AMoon::SetParentPlanet(APlanet* Planet)
{
	this->ParentPlanet = Planet;
}

void AMoon::SetMoonType(EMoonType Type)
{
	this->MoonType = Type;
}

void AMoon::SetMoonDensity(double NewMoonDensity)
{
	this->MoonDensity = NewMoonDensity;
}

void AMoon::SetMoonGravity(double NewMoonGravity)
{
	this->MoonGravity = NewMoonGravity;
}
