// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetSurfaceGenerator.h"

// Sets default values
APlanetEnvironmentGenerator::APlanetEnvironmentGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APlanetEnvironmentGenerator::BeginPlay()
{
	Super::BeginPlay();

    WorldScapeRootInstance = NewObject<AWorldScapeRoot>();

    if (WorldScapeRootInstance)
    {
        // TODO: Добавьте любую дополнительную логику инициализации здесь
        GEngine->AddOnScreenDebugMessage(-1, 51.f, FColor::Red, TEXT("WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("WorldScapeRootInstance has been created successfully."));

    }
    else
    {
        // TODO: Добавьте обработку ошибок, если NewObject вернул nullptr
        GEngine->AddOnScreenDebugMessage(-1, 51.f, FColor::Red, TEXT("Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("Failed to create WorldScapeRootInstance."));

    }
}

// Called every frame
void APlanetEnvironmentGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

