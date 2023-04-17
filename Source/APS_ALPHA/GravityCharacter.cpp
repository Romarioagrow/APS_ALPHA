// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityCharacter.h"
#include <Kismet/KismetMathLibrary.h>

// Sets default values
AGravityCharacter::AGravityCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Создание и настройка SpringArmComponent
    CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
    CameraSpringArm->SetupAttachment(RootComponent);
    CameraSpringArm->TargetArmLength = 300.0f; // Задайте нужную дистанцию между камерой и персонажем
    CameraSpringArm->bUsePawnControlRotation = true;

    // Создание и настройка CameraComponent
    PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
    PlayerCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
}

// Called when the game starts or when spawned
void AGravityCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    UpdateGravity();
    UpdateCameraOrientation();
}

// Called to bind functionality to input
void AGravityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AGravityCharacter::SetGravityType(EGravityType NewGravityType)
{
	CurrentGravityType = NewGravityType;
	UpdateGravity();
}

void AGravityCharacter::UpdateGravity()
{
    switch (CurrentGravityType)
    {
    case EGravityType::ZeroG:
        // Логика обновления гравитации в режиме невесомости
        UpdateZeroGGravity();
        UpdateZeroGCamera();
        break;
    case EGravityType::OnStation:
        // Логика обновления гравитации на станции
        UpdateStationGravity();
        UpdateStationCamera();
        break;
    case EGravityType::OnPlanet:
        // Логика обновления гравитации на планете
        UpdatePlanetGravity();
        UpdatePlanetCamera();
        break;
    case EGravityType::OnShip:
        // Логика обновления гравитации на корабле
        UpdateShipGravity();
        UpdateShipCamera();
        break;
    }
}

void AGravityCharacter::UpdateCameraOrientation()
{
    // Вращение камеры в зависимости от типа гравитации
    switch (CurrentGravityType)
    {
    case EGravityType::ZeroG:
        // Код для ориентации камеры в невесомости
        break;

    case EGravityType::OnStation:
        // Код для ориентации камеры на станции
        break;

    case EGravityType::OnPlanet:
        // Код для ориентации камеры на планете
        break;

    case EGravityType::OnShip:
        // Код для ориентации камеры на корабле
        break;

    default:
        break;
    }
}

void AGravityCharacter::UpdateZeroGGravity()
{
}

void AGravityCharacter::UpdateZeroGCamera()
{
}

void AGravityCharacter::UpdateStationGravity()
{
}

void AGravityCharacter::UpdateStationCamera()
{
}

void AGravityCharacter::UpdatePlanetGravity()
{
    //FVector PlanetCenter = GetPlanetCenter(); // Здесь должна быть функция, которая возвращает центр текущей планеты
    //FVector GravityDirection = (GetActorLocation() - PlanetCenter).GetSafeNormal();
    //FVector GravityForce = -GravityDirection * GetPlanetGravity(); // Здесь должна быть функция, которая возвращает гравитационную силу текущей планеты
    //GetCharacterMovement()->AddForce(GravityForce);

    //FVector NewUp = -GravityDirection;
    //FRotator NewRotation = UKismetMathLibrary::MakeRotationFromAxes(GetActorForwardVector(), GetActorRightVector(), NewUp);
    //SetActorRotation(NewRotation);

    //// Обновление движения персонажа с учетом текущей гравитации
    //FVector InputDirection = FVector(InputComponent->GetAxisValue(TEXT("MoveForward")), InputComponent->GetAxisValue(TEXT("MoveRight")), 0.0f);
    //FVector DesiredMovementDirection = GetActorRotation().RotateVector(InputDirection);
    //FVector FinalMovement = DesiredMovementDirection * CharacterMovementSpeed;
    //GetCharacterMovement()->AddInputVector(FinalMovement);
}

void AGravityCharacter::UpdatePlanetCamera()
{
}

void AGravityCharacter::UpdateShipGravity()
{
}

void AGravityCharacter::UpdateShipCamera()
{
}
