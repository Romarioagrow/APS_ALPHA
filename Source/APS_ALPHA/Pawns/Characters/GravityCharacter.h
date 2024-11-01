#pragma once

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/ArrowComponent.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GravityTypeEnum.h"
#include "GameFramework/Character.h"
#include "GravityCharacter.generated.h"

class AGravityActor;
class ASpaceshipGravityActor;
class APlanetGravityActor;
enum class EGravityType : uint8;

UCLASS()
class APS_ALPHA_API AGravityCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGravityCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void RotateMeshTowardsForwardVector();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * @brief Overlaps
	*/
protected:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                  int32 OtherBodyIndex);

	void UpdateGravityStatus();

	void SwitchGravityToStation(AActor* OtherActor);

	void SwitchGravityToPlanet(AActor* OtherActor);

	void SwitchGravityToSpaceship(AActor* OtherActor);

	void RotateToPlanetGravity(APlanetGravityActor* StationGravityActor);

	void RotateToSpaceshipGravity(ASpaceshipGravityActor* StationGravityActor);

	/**
	 * @Gravity Params
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	EGravityType CurrentGravityType{
		EGravityType::ZeroG
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	FVector GravityDirection{
		0.0f, 0.0f, -1.0f
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	FRotator GravityTargetRotation{
		0.0f, 0.0f, -1.0f
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	float GravityStrength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	AGravityActor* GravityTargetActor;

public:
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetGravityType(EGravityType NewGravityType);

protected:
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void UpdateGravity();

	UFUNCTION(BlueprintCallable, Category = "GravityCamera")
	void UpdateCameraOrientation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	/**
	 * @brief Camera
	*/
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	USpringArmComponent* CameraSpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCameraComponent* PlayerCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float YawSpeed{100.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float PitchSpeed{100.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UArrowComponent* ArrowForwardVector;

private:
	void Turn(float Value);

	void LookUp(float Value);

	void MoveForward(float Value);

	void MoveRight(float Value);

	void MoveUp(float Value);

	void RotatePitch(float Value);

	void RotateRoll(float Value);

	void UpdateZeroGGravity();

	void UpdateStationGravity();

	void UpdatePlanetGravity();

	void UpdateShipGravity();

	FString GetGravityTypeAsString(EGravityType GravityType);
};
