// Fill out your copyright notice in the Description page of Project Settings.


#include "Spaceship.h"

ASpaceship::ASpaceship()
{
	// �������� ���������� SphereCollisionComponent
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	RootComponent = SphereCollisionComponent;
}
