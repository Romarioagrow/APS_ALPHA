#include "SuperSpeedSpectatorPawn.h"

ASuperSpeedSpectatorPawn::ASuperSpeedSpectatorPawn()
{
	// Определите здесь любые свойства, если это необходимо
}

void ASuperSpeedSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Привязка осевых входных событий
	PlayerInputComponent->BindAxis("MoveForward", this, &ASuperSpeedSpectatorPawn::SuperSpeedMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASuperSpeedSpectatorPawn::SuperSpeedMoveRight);
}




void ASuperSpeedSpectatorPawn::SuperSpeedMoveForward(float Val)
{
    if (Val != 0.f)
    {
        APlayerController* PlayerController = Cast<APlayerController>(GetController());
        if (PlayerController)
        {
            const FVector Forward = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
            SetActorLocation(GetActorLocation() + Forward * Val * 1000000000.0);
        }
    }
}

void ASuperSpeedSpectatorPawn::SuperSpeedMoveRight(float Val)
{
    if (Val != 0.f)
    {
        APlayerController* PlayerController = Cast<APlayerController>(GetController());
        if (PlayerController)
        {
            const FRotator CameraRot = PlayerController->PlayerCameraManager->GetCameraRotation();
            const FVector Right = FRotationMatrix(CameraRot).GetScaledAxis(EAxis::Y);
            SetActorLocation(GetActorLocation() + Right * Val * 1000000000.0);
        }
    }
}



//void ASuperSpeedSpectatorPawn::SuperSpeedMoveForward(float Val)
//{
//	if (Val != 0.f)
//	{
//
//		// умножьте Val на желаемую скорость перед передачей его в SetActorLocation
//		const FVector Forward = GetActorForwardVector();
//		SetActorLocation(GetActorLocation() + Forward * Val * 1000000000.0);
//		//// умножьте Val на желаемую скорость перед передачей его в AddMovementInput
//		//const FVector Forward = GetActorForwardVector();
//		//AddMovementInput(Forward, Val * 1000000000.0);
//		//UE_LOG(LogTemp, Warning, TEXT("SuperSpeedMoveForward"));
//	}
//}
//
//void ASuperSpeedSpectatorPawn::SuperSpeedMoveRight(float Val)
//{
//	if (Val != 0.f)
//	{
//
//		// умножьте Val на желаемую скорость перед передачей его в SetActorLocation
//		const FVector Right = GetActorRightVector();
//		SetActorLocation(GetActorLocation() + Right * Val * 1000000000.0);
//
//		//// умножьте Val на желаемую скорость перед передачей его в AddMovementInput
//		//const FVector Right = GetActorRightVector();
//		//AddMovementInput(Right, Val * 1000000000.0);
//		//UE_LOG(LogTemp, Warning, TEXT("SuperSpeedMoveRight"));
//	}
//}
