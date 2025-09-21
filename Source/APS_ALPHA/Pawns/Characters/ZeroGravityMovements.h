// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZeroGravityMovements.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class APS_ALPHA_API UZeroGravityMovements : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default Deltas for this component's properties
	UZeroGravityMovements();

	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY()
	ACharacter* Character;

	UFUNCTION()
	void MoveUp(float Delta);

	UFUNCTION()
	void RotatePitch(float Delta);
	
	UFUNCTION()
	void RotateRoll(float Delta);
	
	UFUNCTION()
	void MoveForward(float Delta);

	UFUNCTION()
	void MoveRight(float Delta);

	UFUNCTION()
	void RotateYaw(float Delta);

	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	void SetupInput(UInputComponent* InputComponent);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
