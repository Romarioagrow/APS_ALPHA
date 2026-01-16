#include "GravityCharacterAPS.h"
#include "Components/CapsuleComponent.h"
#include <Kismet/GameplayStatics.h>
#include "APS_ALPHA/Gameplay/Gravity/PlanetGravityActor.h"
#include "APS_ALPHA/Gameplay/Gravity/SpaceshipGravityActor.h"
#include "APS_ALPHA/Gameplay/Gravity/StationGravityActor.h"

// Sets default values
AGravityCharacterAPS::AGravityCharacterAPS()
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
	GetCapsuleComponent()->SetPhysicsMaxAngularVelocityInDegrees(180.f);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody,
	                                                     ECollisionResponse::ECR_Overlap);
}

// Called when the game starts or when spawned
void AGravityCharacterAPS::BeginPlay()
{
	Super::BeginPlay();

	// set up input component
	InputComponent = NewObject<UInputComponent>(this, TEXT("InputComponent"));

	// Bind overlaps
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AGravityCharacterAPS::OnBeginOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &AGravityCharacterAPS::OnEndOverlap);
}

// Called every frame
void AGravityCharacterAPS::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateGravity();
	//UpdateCameraOrientation();

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue,
	                                 FString::Printf(TEXT("GravityDirection: %s"), *GravityDirection.ToString()));
	/*GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue,
	                                 FString::Printf(
		                                 TEXT("CurrentGravityType: %s"), *GetGravityTypeAsString(CurrentGravityType)));*/
}

void AGravityCharacterAPS::UpdateGravity()
{
	// GravityDirection = -GetActorUpVector();

	switch (CurrentGravityType)
	{
	case EGravityType::ZeroG:
		// ������ ���������� ���������� � ������ �����������
		UpdateZeroGGravity();
	//UpdateZeroGCamera();
		break;
	case EGravityType::OnStation:
		// ������ ���������� ���������� �� �������
		UpdateStationGravity();
	// UpdateStationCamera();
		break;
	case EGravityType::OnPlanet:
		// ������ ���������� ���������� �� �������
		UpdatePlanetGravity();
	// UpdatePlanetCamera();
		break;
	case EGravityType::OnShip:
		// ������ ���������� ���������� �� �������
		UpdateShipGravity();
	// UpdateShipCamera();
		break;
	}
}

void AGravityCharacterAPS::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                       const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("BeginOverlap with: %s"), *OtherActor->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
	                                 FString::Printf(TEXT("BeginOverlap with: %s"), *OtherActor->GetName()));

	if (OtherActor->IsA(AStationGravityActor::StaticClass()))
	{
		SwitchGravityToStation(OtherActor);
	}
	else if (OtherActor->IsA(APlanetGravityActor::StaticClass()))
	{
		SwitchGravityToPlanet(OtherActor);
	}
	else if (OtherActor->IsA(ASpaceshipGravityActor::StaticClass()))
	{
		SwitchGravityToSpaceship(OtherActor);
	}
}

void AGravityCharacterAPS::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("EndOverlap with: %s"), *OtherActor->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
	                                 FString::Printf(TEXT("EndOverlap with: %s"), *OtherActor->GetName()));

	FVector GravityTargetLocation = OtherActor->GetActorLocation();
	//FVector GravityTargetDirection = GravityTargetLocation - GetActorLocation();
	//GravityTargetDirection.Normalize();

	UpdateGravityStatus();
}

void AGravityCharacterAPS::UpdateGravityStatus()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("UpdateGravityStatus")));

	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "EventManager", TaggedActors);

	if (TaggedActors.Num() > 0)
	{
		UClass* FirstGravityActor = TaggedActors[0]->GetClass();
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
		                                 FString::Printf(
			                                 TEXT("FirstGravityActor : %s"), *FirstGravityActor->GetName()));

		// switch gravity to first 
		// ... 
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("FirstGravityActorq 0")));
		CurrentGravityType = EGravityType::ZeroG;
	}
}

void AGravityCharacterAPS::SwitchGravityToStation(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToStation")));

	CurrentGravityType = EGravityType::OnStation;
	GravityTargetActor = Cast<AStationGravityActor>(OtherActor);

	//FVector GravityTargetUpVector = GravityRotationTarget->GetActorUpVector();
	//FVector CapusleForwardVector = GetCapsuleComponent()->GetForwardVector();
	//FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(GravityTargetUpVector, CapusleForwardVector);
	//FRotator RotationRotator = RotationMatrix.Rotator();

	//float DeltaTime = GetWorld()->GetDeltaSeconds(); // Get the time between frames
	//float InterpolationSpeed = 5.0f;
	//FRotator ActorLocation = GetActorLocation().Rotation();

	//GravityTargetRotation = FMath::RInterpTo(ActorLocation, RotationRotator, DeltaTime, InterpolationSpeed);

	//MakeRotFromZX()
	//RotateToStationGravity(CurrentGravityActor);
}

void AGravityCharacterAPS::SwitchGravityToPlanet(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToPlanet")));

	CurrentGravityType = EGravityType::OnPlanet;
	APlanetGravityActor* CurrentGravityActor = Cast<APlanetGravityActor>(OtherActor);
	//RotateToPlanetGravity(CurrentGravityActor);
}

void AGravityCharacterAPS::SwitchGravityToSpaceship(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToSpaceship")));

	CurrentGravityType = EGravityType::OnPlanet;
	GravityTargetActor = Cast<ASpaceshipGravityActor>(OtherActor);
	//RotateToSpaceshipGravity(CurrentGravityActor);
}

//void AGravityCharacter::RotateToStationGravity(AStationGravityActor* StationGravityActor)
//{
//    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("RotateToStationGravity")));
//
//    //FVector GravityUpVector = StationGravityActor->GetActorLocation() - GetActorLocation();
//    //GravityUpVector.Normalize();
//
//    //FVector CapsuleForwardVector = GetCapsuleComponent()->GetForwardVector();
//
//    //FQuat TargetRotation = FQuat::FindBetween(GetCapsuleComponent()->GetUpVector(), GravityUpVector) * GetCapsuleComponent()->GetComponentQuat();
//
//    //float InterpolationSpeed = 5.0f; // ������ �������� ������������
//    //float DeltaTime = GetWorld()->GetDeltaSeconds(); // �������� ����� ����� �������
//    //FQuat InterpolatedRotation = FQuat::Slerp(GetCapsuleComponent()->GetComponentQuat(), TargetRotation, DeltaTime * InterpolationSpeed);
//
//    //GetCapsuleComponent()->SetWorldRotation(InterpolatedRotation);
//}

void AGravityCharacterAPS::RotateToPlanetGravity(APlanetGravityActor* StationGravityActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("RotateToPlanetGravity")));
}

void AGravityCharacterAPS::RotateToSpaceshipGravity(ASpaceshipGravityActor* StationGravityActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("RotateToShipGravity")));
}

void AGravityCharacterAPS::RotateMeshTowardsForwardVector()
{
	// �������� ������� Z ������� CapsuleComponent
	FRotator MeshCurrentRotation = GetMesh()->GetRelativeRotation();

	// �������� ����������� ArrowForwardVector
	FRotator ForwardRelativeRotation = ArrowForwardVector->GetRelativeRotation();

	ForwardRelativeRotation.Yaw -= 90.0f;

	// ������������ ����� ������� ��������� ���� � ������������ ArrowForwardVector
	float InterpolationSpeed = 5.0f; // ������ �������� ������������
	float DeltaTime = GetWorld()->GetDeltaSeconds(); // �������� ����� ����� �������
	FRotator InterpolatedRotation = FMath::RInterpTo(MeshCurrentRotation, ForwardRelativeRotation, DeltaTime,
	                                                 InterpolationSpeed);

	// ��������� ����������������� �������� � ����
	GetMesh()->SetRelativeRotation(InterpolatedRotation);

	UE_LOG(LogTemp, Warning, TEXT("MeshCurrentRotation : %s"),
	       *MeshCurrentRotation.ToString());
	UE_LOG(LogTemp, Warning, TEXT("ForwardRelativeRotation : %s"),
	       *ForwardRelativeRotation.ToString());
	UE_LOG(LogTemp, Warning, TEXT("GetCapsuleComponent RelativeRotation: %s"),
	       *GetCapsuleComponent()->GetRelativeRotation().ToString());
}


// Called to bind functionality to input
void AGravityCharacterAPS::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// ��������� ���������� ����������� ���������
	PlayerInputComponent->BindAxis("MoveForward", this, &AGravityCharacterAPS::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGravityCharacterAPS::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &AGravityCharacterAPS::MoveUp);

	// ��������� ���������� �������� ������
	PlayerInputComponent->BindAxis("Turn", this, &AGravityCharacterAPS::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGravityCharacterAPS::LookUp);

	PlayerInputComponent->BindAxis("RotatePitch", this, &AGravityCharacterAPS::RotatePitch);
	PlayerInputComponent->BindAxis("RotateRoll", this, &AGravityCharacterAPS::RotateRoll);
}

void AGravityCharacterAPS::SetGravityType(EGravityType NewGravityType)
{
	CurrentGravityType = NewGravityType;
	UpdateGravity();
}


void AGravityCharacterAPS::Turn(float Value)
{
	if (Value != 0.0f)
	{
		// ���������� �������� Yaw ArrowForwardVector
		float NewYaw = ArrowForwardVector->GetRelativeRotation().Yaw + Value * YawSpeed * GetWorld()->GetDeltaSeconds();
		FRotator NewRotation = FRotator(0.f, NewYaw, 0.f);
		ArrowForwardVector->SetRelativeRotation(NewRotation);
	}
}

void AGravityCharacterAPS::LookUp(float Value)
{
	if (Value != 0.0f)
	{
		// ���������� �������� Pitch CameraSpringArm
		float NewPitch = CameraSpringArm->GetRelativeRotation().Pitch + Value * PitchSpeed * GetWorld()->
			GetDeltaSeconds();
		NewPitch = FMath::Clamp<float>(NewPitch, -80.f, 80.f);
		FRotator NewRotation = FRotator(NewPitch, 0.f, 0.f);
		CameraSpringArm->SetRelativeRotation(NewRotation);
	}
}

void AGravityCharacterAPS::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// ����� ����������� ����������� ��������� � ����������� �� ������� ���������� ������
		const FQuat RotationQuat = ArrowForwardVector->GetComponentTransform().GetRotation();
		const FRotator Rotation = RotationQuat.Rotator();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
		GetCapsuleComponent()->AddForce(Direction * Value * 100000.f);
		RotateMeshTowardsForwardVector();
	}
}

void AGravityCharacterAPS::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// ����� ����������� ����������� ��������� � ����������� �� ������� ���������� ������
		const FQuat RotationQuat = ArrowForwardVector->GetComponentTransform().GetRotation();
		const FRotator Rotation = RotationQuat.Rotator();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
		GetCapsuleComponent()->AddForce(Direction * Value * 100000.f);
		RotateMeshTowardsForwardVector();
	}
}

void AGravityCharacterAPS::MoveUp(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// ����� ����������� ����������� ��������� � ����������� �� ������� ���������� ������
		const FQuat RotationQuat = ArrowForwardVector->GetComponentTransform().GetRotation();
		const FRotator Rotation = RotationQuat.Rotator();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Z);
		GetCapsuleComponent()->AddForce(Direction * Value * 100000.f);
		RotateMeshTowardsForwardVector();
	}
}

void AGravityCharacterAPS::RotatePitch(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// ����� ����������� ����������� ��������� � ����������� �� ������� ���������� ������


		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("RotatePitch")));
	}
}

void AGravityCharacterAPS::RotateRoll(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// ����� ����������� ����������� ��������� � ����������� �� ������� ���������� ������

		FVector NewRotation = GetCapsuleComponent()->GetRelativeRotation().Vector();
		NewRotation.X += Value * PitchSpeed * GetWorld()->GetDeltaSeconds();
		FRotator Rotator = FRotator(NewRotation.X, NewRotation.Y, NewRotation.Z);
		GetCapsuleComponent()->SetRelativeRotation(Rotator);

		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("RotateRoll")));
	}
}

void AGravityCharacterAPS::UpdateZeroGGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ZeroG Gravity")));
}

//void AGravityCharacter::UpdateZeroGCamera()
//{
//}

void AGravityCharacterAPS::UpdateStationGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Station Gravity")));

	//FVector GravityTargetUpVector = GravityTargetActor->GetActorUpVector();
	//FVector CapusleForwardVector = GetCapsuleComponent()->GetForwardVector();
	//FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(GravityTargetUpVector, CapusleForwardVector);
	//FRotator RotationRotator = RotationMatrix.Rotator();

	//float DeltaTime = GetWorld()->GetDeltaSeconds(); // Get the time between frames
	//float InterpolationSpeed = 5.0f;
	//FRotator ActorLocation = GetActorLocation().Rotation();

	//GravityTargetRotation = FMath::RInterpTo(ActorLocation, RotationRotator, DeltaTime, InterpolationSpeed);
	//SetActorRotation(GravityTargetRotation);
}

//void AGravityCharacter::UpdateStationCamera()
//{
//}

void AGravityCharacterAPS::UpdatePlanetGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Planet Gravity")));


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

//void AGravityCharacter::UpdatePlanetCamera()
//{
//}

void AGravityCharacterAPS::UpdateShipGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("Ship Gravity")));
}

//void AGravityCharacter::UpdateShipCamera()
//{
//}

FString AGravityCharacterAPS::GetGravityTypeAsString(EGravityType GravityType)
{
	const UEnum* EnumPtr = StaticEnum<EGravityType>();
	return EnumPtr ? EnumPtr->GetNameStringByValue(static_cast<int32>(GravityType)) : FString();
}


void AGravityCharacterAPS::UpdateCameraOrientation()
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
