// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceStation.h"

ASpaceStation::ASpaceStation()
{
	// Создание компонента SphereCollisionComponent
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	RootComponent = SphereCollisionComponent;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(RootComponent);
}
