// Fill out your copyright notice in the Description page of Project Settings.


#include "PilotingVehicle.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

void APilotingVehicle::TakeControl(APawn* Pawn)
{
	BeginVehicleControl(Pawn);
}

void APilotingVehicle::ReleaseControl()
{
	EndVehicleControl();
}

bool APilotingVehicle::CanRequestVehicleControl(APawn* RequestingPawn) const
{
	return IsValid(RequestingPawn) && !IsValid(Pilot) && IsValid(RequestingPawn->GetController());
}

bool APilotingVehicle::RequestVehicleControl(APawn* RequestingPawn)
{
	return BeginVehicleControl(RequestingPawn);
}

bool APilotingVehicle::RequestReleaseVehicleControl()
{
	return EndVehicleControl();
}

USceneComponent* APilotingVehicle::GetPilotSeatComponent() const
{
	return GetRootComponent();
}

FTransform APilotingVehicle::GetPilotExitTransform() const
{
	const FVector ExitLocation = GetActorLocation() + GetActorRightVector() * 200.0;
	return FTransform(GetActorRotation(), ExitLocation, FVector::OneVector);
}

bool APilotingVehicle::BeginVehicleControl(APawn* RequestingPawn)
{
	if (!CanRequestVehicleControl(RequestingPawn))
	{
		return false;
	}

	AController* RequestingController = RequestingPawn->GetController();
	Pilot = RequestingPawn;
	PilotController = RequestingController;
	bPilotCollisionWasEnabled = RequestingPawn->GetActorEnableCollision();
	bPilotTickWasEnabled = RequestingPawn->IsActorTickEnabled();
	bPilotWasHiddenInGame = RequestingPawn->IsHidden();

	if (UPrimitiveComponent* PilotRoot = Cast<UPrimitiveComponent>(RequestingPawn->GetRootComponent()))
	{
		bPilotRootWasSimulatingPhysics = PilotRoot->IsSimulatingPhysics();
		if (bPilotRootWasSimulatingPhysics)
		{
			PilotRoot->SetSimulatePhysics(false);
		}
	}

	if (ACharacter* Character = Cast<ACharacter>(RequestingPawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			PilotMovementMode = static_cast<uint8>(Movement->MovementMode);
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}

	RequestingPawn->SetActorEnableCollision(false);
	RequestingPawn->SetActorTickEnabled(false);
	PilotSkeletalComponents.Reset();
	PilotSkeletalTickStates.Reset();
	TArray<USkeletalMeshComponent*> SkeletalComponents;
	RequestingPawn->GetComponents(SkeletalComponents);
	for (USkeletalMeshComponent* SkeletalComponent : SkeletalComponents)
	{
		if (!IsValid(SkeletalComponent))
		{
			continue;
		}
		PilotSkeletalComponents.Add(SkeletalComponent);
		PilotSkeletalTickStates.Add(SkeletalComponent->IsComponentTickEnabled());
		SkeletalComponent->SetComponentTickEnabled(false);
	}
	if (bHidePilotDuringControl)
	{
		RequestingPawn->SetActorHiddenInGame(true);
	}

	if (USceneComponent* PilotSeat = GetPilotSeatComponent())
	{
		RequestingPawn->AttachToComponent(PilotSeat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	RequestingController->Possess(this);

	if (GetController() != RequestingController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s failed to transfer controller to vehicle."), *GetName());
		EndVehicleControl();
		return false;
	}

	OnPilotControlStarted(RequestingPawn);
	UE_LOG(LogTemp, Log, TEXT("%s is now piloted by %s."), *GetName(), *RequestingPawn->GetName());
	return true;
}

bool APilotingVehicle::EndVehicleControl()
{
	if (!IsValid(Pilot))
	{
		return false;
	}

	APawn* PreviousPilot = Pilot;
	AController* ControllerToRestore = GetController() ? GetController() : PilotController.Get();
	const FTransform ExitTransform = GetPilotExitTransform();

	if (ControllerToRestore && ControllerToRestore->GetPawn() == this)
	{
		ControllerToRestore->UnPossess();
	}

	PreviousPilot->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	FVector ExitLocation = ExitTransform.GetLocation();
	const FRotator ExitRotation = ExitTransform.Rotator();
	if (UWorld* World = GetWorld())
	{
		World->FindTeleportSpot(PreviousPilot, ExitLocation, ExitRotation);
	}
	PreviousPilot->SetActorLocationAndRotation(ExitLocation, ExitRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (UPrimitiveComponent* PilotRoot = Cast<UPrimitiveComponent>(PreviousPilot->GetRootComponent()))
	{
		PilotRoot->SetSimulatePhysics(bPilotRootWasSimulatingPhysics);
	}

	PreviousPilot->SetActorEnableCollision(bPilotCollisionWasEnabled);
	PreviousPilot->SetActorTickEnabled(bPilotTickWasEnabled);
	PreviousPilot->SetActorHiddenInGame(bPilotWasHiddenInGame);
	for (int32 ComponentIndex = 0; ComponentIndex < PilotSkeletalComponents.Num(); ++ComponentIndex)
	{
		if (USkeletalMeshComponent* SkeletalComponent = PilotSkeletalComponents[ComponentIndex].Get())
		{
			const bool bWasTickEnabled = PilotSkeletalTickStates.IsValidIndex(ComponentIndex)
				&& PilotSkeletalTickStates[ComponentIndex];
			SkeletalComponent->SetComponentTickEnabled(bWasTickEnabled);
		}
	}
	PilotSkeletalComponents.Reset();
	PilotSkeletalTickStates.Reset();

	if (ACharacter* Character = Cast<ACharacter>(PreviousPilot))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetMovementMode(static_cast<EMovementMode>(PilotMovementMode));
			Movement->Velocity = GetVelocity();
		}
	}

	if (ControllerToRestore)
	{
		ControllerToRestore->Possess(PreviousPilot);
	}

	OnPilotControlEnded(PreviousPilot);
	UE_LOG(LogTemp, Log, TEXT("%s released pilot %s."), *GetName(), *PreviousPilot->GetName());

	Pilot = nullptr;
	PilotController = nullptr;
	return true;
}

