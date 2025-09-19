// Fill out your copyright notice in the Description page of Project Settings.


#include "APS_GravityCharacter.h"

// Sets default values
AAPS_GravityCharacter::AAPS_GravityCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAPS_GravityCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAPS_GravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AAPS_GravityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

