// Fill out your copyright notice in the Description page of Project Settings.


#include "Star.h"

AStar::AStar()
{
}

void AStar::AddPlanet(APlanet* Planet)
{
	this->Planets.Add(Planet);
}

void AStar::SetLuminosity(float StarLuminosity)
{
	this->Luminosity = StarLuminosity;
}

void AStar::SetSurfaceTemperature(int NewSurfaceTemperature)
{
	this->SurfaceTemperature = NewSurfaceTemperature;
}

void AStar::SetStarType(EStarType NewStarType)
{
	this->StarType = NewStarType;
}

void AStar::SetStarSpectralClass(EStarSpectralClass NewStarSpectralClass)
{
	this->StarSpectralClass = NewStarSpectralClass;
}
