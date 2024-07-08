// Fill out your copyright notice in the Description page of Project Settings.


#include "CelestialBody.h"

ACelestialBody::ACelestialBody()
{
}

float ACelestialBody::GetRadius()
{
	return this->Radius;
}

void ACelestialBody::SetRadius(float NewRadius)
{
	this->Radius = NewRadius;
}

void ACelestialBody::SetMass(float NewMass)
{
	this->Mass = NewMass;
}

void ACelestialBody::SetAge(FString NewAge)
{
	this->Age = NewAge;
}

void ACelestialBody::SetName(FName NewName)
{
	this->AstroName = NewName;
}

