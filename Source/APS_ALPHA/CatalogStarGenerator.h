// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Classes/Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CatalogStarGenerator.generated.h"

UCLASS()
class APS_ALPHA_API ACatalogStarGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACatalogStarGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/*TMap<FString, UHierarchicalInstancedStaticMeshComponent*> HISMMap;*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UHierarchicalInstancedStaticMeshComponent* HISMComponent;

};
