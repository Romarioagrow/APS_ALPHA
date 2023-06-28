// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldActor.h"

// Sets default values
AWorldActor::AWorldActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWorldActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWorldActor::CalculateAffectionRadius()
{
	//return 0.0;
	FVector Origin, BoxExtent;
	double SphereRadius;
	this->GetActorBounds(false, Origin, BoxExtent);
	SphereRadius = BoxExtent.GetMax();
	this->AffectionRadiusKM = SphereRadius / 100000.0;
	//return SphereRadius / 100000.0;
	//NewMoon->AffectionRadiusKM = SphereRadius / 100000.0;
}

