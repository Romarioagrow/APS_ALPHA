// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityCharacter.h"
#include <Kismet/KismetMathLibrary.h>
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AGravityCharacter::AGravityCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Создание и настройка InputComponent
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    // Создание и настройка ArrowForwardVector
    ArrowForwardVector = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowForwardVector"));
    ArrowForwardVector->SetupAttachment(RootComponent);

    // Создание и настройка SpringArmComponent
    CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
    CameraSpringArm->SetupAttachment(ArrowForwardVector);
    CameraSpringArm->TargetArmLength = 300.0f;
    CameraSpringArm->bUsePawnControlRotation = false;

    // Создание и настройка CameraComponent
    PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
    PlayerCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
    GetCapsuleComponent()->SetPhysicsMaxAngularVelocityInDegrees(180.f);
}

// Called when the game starts or when spawned
void AGravityCharacter::BeginPlay()
{
	Super::BeginPlay();

    // set up input component
    InputComponent = NewObject<UInputComponent>(this, TEXT("InputComponent"));
}

// Called every frame
void AGravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    UpdateGravity();
    UpdateCameraOrientation();

    GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("GravityDirection: %s"), *GravityDirection.ToString()));
    GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("CurrentGravityType: %s"), *GetGravityTypeAsString(CurrentGravityType)));
}

void AGravityCharacter::RotateMeshTowardsForwardVector()
{
    // Получаем текущий Z ротатор CapsuleComponent
    FRotator MeshCurrentRotation = GetMesh()->GetRelativeRotation();

    // Получаем направление ArrowForwardVector
    FRotator ForwardRelativeRotation = ArrowForwardVector->GetRelativeRotation();

    ForwardRelativeRotation.Yaw -= 90.0f;

    // Интерполяция между текущим вращением меша и направлением ArrowForwardVector
    float InterpolationSpeed = 5.0f; // Задаем скорость интерполяции
    float DeltaTime = GetWorld()->GetDeltaSeconds(); // Получаем время между кадрами
    FRotator InterpolatedRotation = FMath::RInterpTo(MeshCurrentRotation, ForwardRelativeRotation, DeltaTime, InterpolationSpeed);

    // Применяем интерполированное вращение к мешу
    GetMesh()->SetRelativeRotation(InterpolatedRotation);

    UE_LOG(LogTemp, Warning, TEXT("MeshCurrentRotation : %s"), 
        *MeshCurrentRotation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("ForwardRelativeRotation : %s"), 
        *ForwardRelativeRotation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("GetCapsuleComponent RelativeRotation: %s"), 
        *GetCapsuleComponent()->GetRelativeRotation().ToString());
}


// Called to bind functionality to input
void AGravityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Настройка параметров перемещения персонажа
	PlayerInputComponent->BindAxis("MoveForward", this, &AGravityCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGravityCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &AGravityCharacter::MoveUp);

	// Настройка параметров поворота камеры
	PlayerInputComponent->BindAxis("Turn", this, &AGravityCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGravityCharacter::LookUp);
}

void AGravityCharacter::SetGravityType(EGravityType NewGravityType)
{
	CurrentGravityType = NewGravityType;
	UpdateGravity();
}


void AGravityCharacter::Turn(float Value)
{
    if (Value != 0.0f)
    {
        // Обновление смещения Yaw ArrowForwardVector
        float NewYaw = ArrowForwardVector->GetRelativeRotation().Yaw + Value * YawSpeed * GetWorld()->GetDeltaSeconds();
        FRotator NewRotation = FRotator(0.f, NewYaw, 0.f);
        ArrowForwardVector->SetRelativeRotation(NewRotation);
    }
}

void AGravityCharacter::LookUp(float Value)
{
    if (Value != 0.0f)
    {
        // Обновление смещения Pitch CameraSpringArm
        float NewPitch = CameraSpringArm->GetRelativeRotation().Pitch + Value * PitchSpeed * GetWorld()->GetDeltaSeconds();
        NewPitch = FMath::Clamp<float>(NewPitch, -80.f, 80.f);
        FRotator NewRotation = FRotator(NewPitch, 0.f, 0.f);
        CameraSpringArm->SetRelativeRotation(NewRotation);
    }
}

void AGravityCharacter::MoveForward(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f))
    {
        // Найти направление перемещения персонажа в зависимости от текущей ориентации камеры
        const FQuat RotationQuat = ArrowForwardVector->GetComponentTransform().GetRotation();
        const FRotator Rotation = RotationQuat.Rotator();
        const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
        GetCapsuleComponent()->AddForce(Direction * Value * 100000.f);
        RotateMeshTowardsForwardVector();
    }
}

void AGravityCharacter::MoveRight(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f))
    {
        // Найти направление перемещения персонажа в зависимости от текущей ориентации камеры
        const FQuat RotationQuat = ArrowForwardVector->GetComponentTransform().GetRotation();
        const FRotator Rotation = RotationQuat.Rotator();
        const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
        GetCapsuleComponent()->AddForce(Direction * Value * 100000.f);
        RotateMeshTowardsForwardVector();
    }
}

void AGravityCharacter::MoveUp(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f))
    {
        // Найти направление перемещения персонажа в зависимости от текущей ориентации камеры
        const FQuat RotationQuat = ArrowForwardVector->GetComponentTransform().GetRotation();
        const FRotator Rotation = RotationQuat.Rotator();
        const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Z);
        GetCapsuleComponent()->AddForce(Direction * Value * 100000.f);
        RotateMeshTowardsForwardVector();
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

FString AGravityCharacter::GetGravityTypeAsString(EGravityType GravityType)
{
    const UEnum* EnumPtr = StaticEnum<EGravityType>();
    return EnumPtr ? EnumPtr->GetNameStringByValue(static_cast<int32>(GravityType)) : FString();
}

void AGravityCharacter::UpdateGravity()
{
    GravityDirection = -GetActorUpVector();

    //switch (CurrentGravityType)
    //{
    //case EGravityType::ZeroG:
    //    // Логика обновления гравитации в режиме невесомости
    //    UpdateZeroGGravity();
    //    UpdateZeroGCamera();
    //    break;
    //case EGravityType::OnStation:
    //    // Логика обновления гравитации на станции
    //    UpdateStationGravity();
    //    UpdateStationCamera();
    //    break;
    //case EGravityType::OnPlanet:
    //    // Логика обновления гравитации на планете
    //    UpdatePlanetGravity();
    //    UpdatePlanetCamera();
    //    break;
    //case EGravityType::OnShip:
    //    // Логика обновления гравитации на корабле
    //    UpdateShipGravity();
    //    UpdateShipCamera();
    //    break;
    //}
}

void AGravityCharacter::UpdateCameraOrientation()
{
    // Вращение камеры в зависимости от типа гравитации
    //switch (CurrentGravityType)
    //{
    //case EGravityType::ZeroG:
    //    // Код для ориентации камеры в невесомости
    //    break;

    //case EGravityType::OnStation:
    //    // Код для ориентации камеры на станции
    //    break;

    //case EGravityType::OnPlanet:
    //    // Код для ориентации камеры на планете
    //    break;

    //case EGravityType::OnShip:
    //    // Код для ориентации камеры на корабле
    //    break;

    //default:
    //    break;
    //}

    //GetController()->SetControlRotation(UKismetMathLibrary::MakeRotationFromAxes(GetActorForwardVector(), GetActorRightVector(), -GravityDirection));
}