// Fill out your copyright notice in the Description page of Project Settings.


#include "PilotingVehicle.h"

void APilotingVehicle::TakeControl(APawn* Pawn)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("APilotingVehicle TakeControl")));

	//Pilot = Cast<AGravityCharacterPawn>(Pawn);
    if (Pawn)
    {
        Pilot = Cast<AGravityCharacterPawn>(Pawn);
        if (!Pilot)
        {
            UE_LOG(LogTemp, Warning, TEXT("TakeControl called with non-AGravityCharacterPawn Pawn."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TakeControl called with null Pawn."));
    }
}
