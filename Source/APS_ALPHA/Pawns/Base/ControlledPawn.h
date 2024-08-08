// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "GameFramework/Pawn.h"
#include "ControlledPawn.generated.h"

UCLASS()
class APS_ALPHA_API AControlledPawn : public APawn, public IItemInfoInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AControlledPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FText GetInGameName_Implementation() const override;
	
	virtual UTexture2D* GetAvatarPicture_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	FText InGameName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	UTexture2D* AvatarPicture;
};
