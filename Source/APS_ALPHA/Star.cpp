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

void AStar::SetStarType(EStellarClass NewStarType)
{
	this->StellarClass = NewStarType;
}

void AStar::SetStarSpectralClass(ESpectralClass NewStarSpectralClass)
{
	this->SpectralClass = NewStarSpectralClass;
}

void AStar::SetStarSpectralType(ESpectralType NewSpectralType)
{
	this->SpectralType = NewSpectralType;
}

void AStar::SetFullSpectralClass(FName NewFullSpectralClass)
{
	this->FullSpectralClass = NewFullSpectralClass;
}

void AStar::SetFullSpectralName(FName NewFullSpectralName)
{
	this->FullSpectralName = NewFullSpectralName;
}

void AStar::SetSpectralSubclass(int NewSpectralSubclass)
{
	this->SpectralSubclass = NewSpectralSubclass;
}
