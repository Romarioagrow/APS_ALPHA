#pragma once
 
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GravityTypeEnum.h"
#include "APS_ALPHA/Pawns/Characters/APS_GravityCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GravityController.generated.h"
 
/**
 * A Player Controller class which adds input-handling functionality for
 * CharacterMovementController's custom gravity mechanics.
 */
UCLASS()
class AGravityController : public APlayerController
{
	GENERATED_BODY()
 
public:
	virtual void UpdateRotation(float DeltaTime) override;
 
	// Converts a rotation from world space to gravity relative space.
	UFUNCTION(BlueprintPure)
	static FRotator GetGravityRelativeRotation(FRotator Rotation, FVector GravityDirection);
 
	// Converts a rotation from gravity relative space to world space.
	UFUNCTION(BlueprintPure)
	static FRotator GetGravityWorldRotation(FRotator Rotation, FVector GravityDirection);

	UPROPERTY()
	AAPS_GravityCharacter* GravityCharacter;

	UPROPERTY()
	EGravityType CurrentGravityType{
		EGravityType::ZeroG
	};

	UPROPERTY()
	bool bIsZeroG;

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;
 
private:
	FVector LastFrameGravity = FVector::ZeroVector;
};