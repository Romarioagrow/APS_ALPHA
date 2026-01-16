// Fill out your copyright notice in the Description page of Project Settings.


#include "ZeroGravityMovements.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UZeroGravityMovements::UZeroGravityMovements()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}

// Called when the game starts
void UZeroGravityMovements::BeginPlay()
{
	Super::BeginPlay();

	
	// 1) Если компонент висит на персонаже — берём владельца
	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		Character = OwnerChar;
	}
	else
	{
		// 2) Иначе берём главного персонажа игрока (PlayerIndex 0)
		if (ACharacter* PCChar = UGameplayStatics::GetPlayerCharacter(this, 0))
		{
			Character = PCChar;
		}
	}

	// Опционально: отключить тик, если персонажа нет
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("ZeroGravityMovements: Player Character not found"));
		SetComponentTickEnabled(false);
	}
}

void UZeroGravityMovements::SetupInput(UInputComponent* InputComponent)
{
	check(InputComponent);
	InputComponent->BindAxis("MoveForward", this, &UZeroGravityMovements::MoveForward);
	InputComponent->BindAxis("MoveRight",   this, &UZeroGravityMovements::MoveRight);
	InputComponent->BindAxis("MoveUp",      this, &UZeroGravityMovements::MoveUp);
	InputComponent->BindAxis("RotatePitch", this, &UZeroGravityMovements::RotatePitch);
	InputComponent->BindAxis("RotateRoll",  this, &UZeroGravityMovements::RotateRoll);
	InputComponent->BindAxis("RotateYaw",   this, &UZeroGravityMovements::RotateYaw);
}

// Called every frame
void UZeroGravityMovements::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UZeroGravityMovements::MoveUp(float Delta)
{
}

void UZeroGravityMovements::RotatePitch(float Delta)
{
}

void UZeroGravityMovements::RotateRoll(float Delta)
{
}

void UZeroGravityMovements::MoveForward(float Delta)
{
}

void UZeroGravityMovements::MoveRight(float Delta)
{
}

void UZeroGravityMovements::RotateYaw(float Delta)
{
}



