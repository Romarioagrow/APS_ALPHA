// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"

APlanet::APlanet()
{
	// �������� ���������� SphereCollisionComponent
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	RootComponent = SphereCollisionComponent;
}
