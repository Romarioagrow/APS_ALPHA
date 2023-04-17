// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityCharacter.h"
#include <Kismet/KismetMathLibrary.h>

// Sets default values
AGravityCharacter::AGravityCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // �������� � ��������� SpringArmComponent
    CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
    CameraSpringArm->SetupAttachment(RootComponent);
    CameraSpringArm->TargetArmLength = 300.0f; // ������� ������ ��������� ����� ������� � ����������
    CameraSpringArm->bUsePawnControlRotation = true;

    // �������� � ��������� CameraComponent
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
        // ������ ���������� ���������� � ������ �����������
        UpdateZeroGGravity();
        UpdateZeroGCamera();
        break;
    case EGravityType::OnStation:
        // ������ ���������� ���������� �� �������
        UpdateStationGravity();
        UpdateStationCamera();
        break;
    case EGravityType::OnPlanet:
        // ������ ���������� ���������� �� �������
        UpdatePlanetGravity();
        UpdatePlanetCamera();
        break;
    case EGravityType::OnShip:
        // ������ ���������� ���������� �� �������
        UpdateShipGravity();
        UpdateShipCamera();
        break;
    }
}

void AGravityCharacter::UpdateCameraOrientation()
{
    // �������� ������ � ����������� �� ���� ����������
    switch (CurrentGravityType)
    {
    case EGravityType::ZeroG:
        // ��� ��� ���������� ������ � �����������
        break;

    case EGravityType::OnStation:
        // ��� ��� ���������� ������ �� �������
        break;

    case EGravityType::OnPlanet:
        // ��� ��� ���������� ������ �� �������
        break;

    case EGravityType::OnShip:
        // ��� ��� ���������� ������ �� �������
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
    //FVector PlanetCenter = GetPlanetCenter(); // ����� ������ ���� �������, ������� ���������� ����� ������� �������
    //FVector GravityDirection = (GetActorLocation() - PlanetCenter).GetSafeNormal();
    //FVector GravityForce = -GravityDirection * GetPlanetGravity(); // ����� ������ ���� �������, ������� ���������� �������������� ���� ������� �������
    //GetCharacterMovement()->AddForce(GravityForce);

    //FVector NewUp = -GravityDirection;
    //FRotator NewRotation = UKismetMathLibrary::MakeRotationFromAxes(GetActorForwardVector(), GetActorRightVector(), NewUp);
    //SetActorRotation(NewRotation);

    //// ���������� �������� ��������� � ������ ������� ����������
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
