// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <Components/SphereComponent.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityActor.generated.h"

UCLASS()
class APS_ALPHA_API AGravityActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGravityActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;
};
