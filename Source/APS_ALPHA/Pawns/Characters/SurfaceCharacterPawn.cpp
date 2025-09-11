#include "SurfaceCharacterPawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

ASurfaceCharacterPawn::ASurfaceCharacterPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Настройка CharacterMovementComponent для работы с любыми поверхностями
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	MovementComp->MaxWalkSpeed = WalkSpeed;
	MovementComp->JumpZVelocity = JumpVelocity;
	MovementComp->AirControl = 0.35f;
	MovementComp->MaxStepHeight = MaxStepHeight;
	MovementComp->SetWalkableFloorAngle(90.0f); // Позволяет ходить по вертикальным поверхностям
	MovementComp->bOrientRotationToMovement = false; // Мы будем управлять поворотом сами
	MovementComp->bUseControllerDesiredRotation = false;
	MovementComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

	// Создание SpringArm для камеры
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->TargetArmLength = 300.0f;
	CameraSpringArm->bUsePawnControlRotation = true;
	CameraSpringArm->bInheritPitch = true;
	CameraSpringArm->bInheritYaw = true;
	CameraSpringArm->bInheritRoll = false;

	// Создание камеры
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);

	// Создание Arrow компонента для направления движения
	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(RootComponent);

	// Инициализация переменных
	CurrentSurfaceNormal = FVector::UpVector;
	bIsOnSurface = false;
	SurfaceAngle = 0.0f;
}

void ASurfaceCharacterPawn::BeginPlay()
{
	Super::BeginPlay();
}

void ASurfaceCharacterPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableSurfaceMovement)
	{
		UpdateSurfaceOrientation();
		ApplySurfaceGravity();
	}
}

void ASurfaceCharacterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Привязка осей движения
	PlayerInputComponent->BindAxis("MoveForward", this, &ASurfaceCharacterPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASurfaceCharacterPawn::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASurfaceCharacterPawn::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASurfaceCharacterPawn::LookUp);

	// Привязка действий
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASurfaceCharacterPawn::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASurfaceCharacterPawn::StopJump);
}

void ASurfaceCharacterPawn::MoveForward(float Value)
{
	if (Controller && Value != 0.0f)
	{
		// Получаем направление движения относительно камеры
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		
		// Применяем движение
		AddMovementInput(Direction, Value);
	}
}

void ASurfaceCharacterPawn::MoveRight(float Value)
{
	if (Controller && Value != 0.0f)
	{
		// Получаем направление движения относительно камеры
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		// Применяем движение
		AddMovementInput(Direction, Value);
	}
}

void ASurfaceCharacterPawn::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASurfaceCharacterPawn::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASurfaceCharacterPawn::StartJump()
{
	bPressedJump = true;
}

void ASurfaceCharacterPawn::StopJump()
{
	bPressedJump = false;
}

void ASurfaceCharacterPawn::UpdateSurfaceOrientation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Трассировка для определения поверхности под персонажем
	FVector StartLocation = GetActorLocation();
	FVector EndLocation = StartLocation - (GetActorUpVector() * SurfaceDetectionDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);
	
	if (bHit)
	{
		CurrentSurfaceNormal = HitResult.Normal;
		bIsOnSurface = true;
		
		// Вычисляем угол поверхности
		SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(CurrentSurfaceNormal, FVector::UpVector)));
		
		// Ориентируем персонажа по поверхности
		FVector NewUpVector = -CurrentSurfaceNormal;
		FVector NewForwardVector = GetActorForwardVector();
		
		// Проецируем forward vector на плоскость поверхности
		NewForwardVector = FVector::VectorPlaneProject(NewForwardVector, NewUpVector).GetSafeNormal();
		
		// Создаем новую ротацию
		FRotator NewRotation = FRotationMatrix::MakeFromXZ(NewForwardVector, NewUpVector).Rotator();
		
		// Плавно интерполируем к новой ротации
		FRotator CurrentRotation = GetActorRotation();
		FRotator TargetRotation = FMath::RInterpTo(CurrentRotation, NewRotation, GetWorld()->GetDeltaSeconds(), SurfaceOrientationSpeed);
		
		SetActorRotation(TargetRotation);
		
		// Обновляем Arrow компонент для отображения направления
		DirectionArrow->SetWorldRotation(FRotator(0.0f, TargetRotation.Yaw, 0.0f));
	}
	else
	{
		bIsOnSurface = false;
		CurrentSurfaceNormal = FVector::UpVector;
		SurfaceAngle = 0.0f;
	}
}

void ASurfaceCharacterPawn::ApplySurfaceGravity()
{
	if (!bIsOnSurface) return;

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (!MovementComp) return;

	// Применяем гравитацию к поверхности
	FVector GravityDirection = -CurrentSurfaceNormal;
	FVector GravityForce = GravityDirection * MovementComp->GetGravityZ();
	
	// Применяем силу гравитации
	MovementComp->AddForce(GravityForce);
}

FVector ASurfaceCharacterPawn::GetSurfaceNormal()
{
	return CurrentSurfaceNormal;
}

bool ASurfaceCharacterPawn::IsOnSurface()
{
	return bIsOnSurface;
}
