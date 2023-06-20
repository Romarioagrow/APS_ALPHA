// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceStation.h"

ASpaceStation::ASpaceStation()
{
	// Создание компонента SphereCollisionComponent
	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("StationGravitySphereCollisionComponent"));
	GravityCollisionZone->SetupAttachment(RootComponent);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(GravityCollisionZone);
	SpawnPoint->SetWorldRotation(GetActorRotation());
}
