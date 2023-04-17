// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityCharacter.h"
#include <Kismet/KismetMathLibrary.h>

// Sets default values
AGravityCharacter::AGravityCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // �������� � ��������� InputComponent
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    // �������� � ��������� ArrowForwardVector
    ArrowForwardVector = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowForwardVector"));
    ArrowForwardVector->SetupAttachment(RootComponent);

    // �������� � ��������� SpringArmComponent
    CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
    CameraSpringArm->SetupAttachment(ArrowForwardVector);
    CameraSpringArm->TargetArmLength = 300.0f;
    CameraSpringArm->bUsePawnControlRotation = false;

    // �������� � ��������� CameraComponent
    PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
    PlayerCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
}

// Called when the game starts or when spawned
void AGravityCharacter::BeginPlay()
{
	Super::BeginPlay();

    //SetActorRotation(FRotator(-80.0f, -150.0f, -110.0f));
}

// Called every frame
void AGravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    UpdateGravity();
    UpdateCameraOrientation();

    GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("GravityDirection %s"), *GravityDirection.ToString()));
}

// Called to bind functionality to input
void AGravityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    // ��������� ���������� ����������� ���������
	PlayerInputComponent->BindAxis("MoveForward", this, &AGravityCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGravityCharacter::MoveRight);
	// ��������� ���������� �������� ������
	PlayerInputComponent->BindAxis("Turn", this, &AGravityCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGravityCharacter::LookUp);
}

void AGravityCharacter::SetGravityType(EGravityType NewGravityType)
{
	CurrentGravityType = NewGravityType;
	UpdateGravity();
}

void AGravityCharacter::UpdateGravity()
{
    GravityDirection = -GetActorUpVector();
    GetController()->SetControlRotation(UKismetMathLibrary::MakeRotationFromAxes(GetActorForwardVector(), GetActorRightVector(), -GravityDirection));
    
    //switch (CurrentGravityType)
    //{
    //case EGravityType::ZeroG:
    //    // ������ ���������� ���������� � ������ �����������
    //    UpdateZeroGGravity();
    //    UpdateZeroGCamera();
    //    break;
    //case EGravityType::OnStation:
    //    // ������ ���������� ���������� �� �������
    //    UpdateStationGravity();
    //    UpdateStationCamera();
    //    break;
    //case EGravityType::OnPlanet:
    //    // ������ ���������� ���������� �� �������
    //    UpdatePlanetGravity();
    //    UpdatePlanetCamera();
    //    break;
    //case EGravityType::OnShip:
    //    // ������ ���������� ���������� �� �������
    //    UpdateShipGravity();
    //    UpdateShipCamera();
    //    break;
    //}
}

void AGravityCharacter::UpdateCameraOrientation()
{
    // �������� ������ � ����������� �� ���� ����������
    //switch (CurrentGravityType)
    //{
    //case EGravityType::ZeroG:
    //    // ��� ��� ���������� ������ � �����������
    //    break;

    //case EGravityType::OnStation:
    //    // ��� ��� ���������� ������ �� �������
    //    break;

    //case EGravityType::OnPlanet:
    //    // ��� ��� ���������� ������ �� �������
    //    break;

    //case EGravityType::OnShip:
    //    // ��� ��� ���������� ������ �� �������
    //    break;

    //default:
    //    break;
    //}

    //GetController()->SetControlRotation(UKismetMathLibrary::MakeRotationFromAxes(GetActorForwardVector(), GetActorRightVector(), -GravityDirection));
}

void AGravityCharacter::Turn(float Value)
{
    // ���������� �������� Yaw ArrowForwardVector
    float NewYaw = ArrowForwardVector->GetRelativeRotation().Yaw + Value * YawSpeed * GetWorld()->GetDeltaSeconds();
    FRotator NewRotation = FRotator(0.f, NewYaw, 0.f);
    ArrowForwardVector->SetRelativeRotation(NewRotation);
}

void AGravityCharacter::LookUp(float Value)
{
    // ���������� �������� Pitch CameraSpringArm
    float NewPitch = CameraSpringArm->GetRelativeRotation().Pitch + Value * PitchSpeed * GetWorld()->GetDeltaSeconds();
    NewPitch = FMath::Clamp<float>(NewPitch, -80.f, 80.f);
    FRotator NewRotation = FRotator(NewPitch, 0.f, 0.f);
    CameraSpringArm->SetRelativeRotation(NewRotation);
}

void AGravityCharacter::MoveForward(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f))
    {
        // ����� ����������� ����������� ��������� � ����������� �� ������� ���������� ������
        const FRotator Rotation = Controller->GetControlRotation();
        const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void AGravityCharacter::MoveRight(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f))
    {
        // ����� ����������� ����������� ��������� � ����������� �� ������� ���������� ������
        const FRotator Rotation = Controller->GetControlRotation();
        const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
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
