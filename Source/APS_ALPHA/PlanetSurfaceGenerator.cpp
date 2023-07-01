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

    if (true)
    {
        InitWorldScape();
    }
}

// Called every frame
void APlanetEnvironmentGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APlanetEnvironmentGenerator::InitWorldScape()
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("InitWorldScape!"));
   // WorldScapeRootInstance = NewObject<AWorldScapeRoot>();
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = GetWorld()->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {
        // TODO: Добавьте любую дополнительную логику инициализации здесь
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        

        //SetupDefaultGenerator();
        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->bGenerateWorldScape = true;
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("SetupDefaultGenerator!"));
    }
    else
    {
        // TODO: Добавьте обработку ошибок, если NewObject вернул nullptr
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }
}

void APlanetEnvironmentGenerator::SetupDefaultGenerator()
{
   
    
    WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
    WorldScapeRootInstance->bGenerateWorldScape = true;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("SetupDefaultGenerator!"));

}