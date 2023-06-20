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

void APilotingVehicle::ReleaseControl()
{
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Orange, FString::Printf(TEXT("APilotingVehicle ReleaseControl")));


    // Ensure the pilot exists and is attached to the spaceship
    if (Pilot && Pilot->IsAttachedTo(this))
    {
        // Get the player controller
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

        if (PlayerController)
        {
                // Try to cast the previous pawn to AGravityCharacterPawn
            if (AGravityCharacterPawn* PreviousGravityPawn = Cast<AGravityCharacterPawn>(Pilot))
            {
                // Unpossess the spaceship
                PlayerController->UnPossess();

                // Enable the pawn's input and movement
                Pilot->EnableInput(PlayerController);
                Pilot->SetActorEnableCollision(true);
                Pilot->SetActorTickEnabled(true);

                // Re-possess the previous pawn
                PlayerController->Possess(PreviousGravityPawn);

                // Detach the pilot from the PilotChair of the spaceship
                Pilot->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

                // Reset the Pilot pointer
                Pilot = nullptr;


            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Previous Pawn is not a AGravityCharacterPawn."));
            }
        }
    }
}

