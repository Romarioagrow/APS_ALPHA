// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityActor.h"

// Sets default values
AGravityActor::AGravityActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Создаем USphereComponent и назначаем его в качестве корневого компонента
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	RootComponent = SphereCollisionComponent;
}

// Called when the game starts or when spawned
void AGravityActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGravityActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

