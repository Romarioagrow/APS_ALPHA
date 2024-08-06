// Fill out your copyright notice in the Description page of Project Settings.


#include "ControlledPawn.h"

// Sets default values
AControlledPawn::AControlledPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AControlledPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AControlledPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AControlledPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

FText AControlledPawn::GetInGameName_Implementation() const
{
	return InGameName;
}

UTexture2D* AControlledPawn::GetAvatarPicture_Implementation() const
{
	return AvatarPicture;
}