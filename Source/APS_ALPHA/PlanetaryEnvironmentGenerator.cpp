// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetaryEnvironmentGenerator.h"
#include "Planet.h"
#include "Moon.h"

// Sets default values
APlanetaryEnvironmentGenerator::APlanetaryEnvironmentGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APlanetaryEnvironmentGenerator::BeginPlay()
{
	Super::BeginPlay();


    if (false)
    {
        InitWorldScape(GetWorld());
    }
}

// Called every frame
void APlanetaryEnvironmentGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    

    /// if pawn.distance < planet-affectDistance * WSCScale -> Spawn planet environment
    /// if pawn.distance > planet-affectDistance * WSCScale -> Destroy planet environment
}

void APlanetaryEnvironmentGenerator::InitAtmoScape(UWorld* World, double Radius, APlanetaryBody* NewPlanetaryBody)
{
    PlanetAtmosphere = World->SpawnActor<AAtmosphere>(AAtmosphere::StaticClass(), FTransform());

    if (PlanetAtmosphere)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PlanetAtmosphere AtmoScapeInstance has been created successfully."));
        // Установка параметров и свойств для объекта Atmosphere.
        PlanetAtmosphere->PlanetRadius = Radius - 1; // Atm Dead Zone 
        PlanetAtmosphere->SetActorLocation(NewPlanetaryBody->GetActorLocation());
        PlanetAtmosphere->SetActorRotation(NewPlanetaryBody->GetActorRotation());
        PlanetAtmosphere->AttachToActor(NewPlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);
    }
}


void APlanetaryEnvironmentGenerator::InitWorldScape(UWorld* World)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("InitWorldScape!"));
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->bGenerateWorldScape = true;

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldScape!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }
}

void APlanetaryEnvironmentGenerator::GenerateWorldscapeSurfaceByModel(UWorld* World, APlanet* NewPlanet)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldscapeSurfaceByModel!"));
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {
        PlanetaryBody = NewPlanet;

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        double PlanetRadiusKM = NewPlanet->RadiusKM;

        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->PlanetScale = PlanetRadiusKM * 100000;
        WorldScapeRootInstance->DistanceToFreezeGeneration = PlanetRadiusKM * 1000000;


        WorldScapeRootInstance->SetActorLocation(NewPlanet->GetActorLocation());
        WorldScapeRootInstance->SetActorRotation(NewPlanet->GetActorRotation());
        WorldScapeRootInstance->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);
        //NewPlanet->Mesh
        //NewPlanet->SetupWorldScapeRoot(WorldScapeRootInstance);

        //WorldScapeRootInstance->bGenerateWorldScape = true;
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldScape!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }

}
void APlanetaryEnvironmentGenerator::GenerateWorldscapeSurfaceByModel(UWorld* World, AMoon* NewMoon)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldscapeSurfaceByModel!"));
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        PlanetaryBody = NewMoon;

        double PlanetRadiusKM = NewMoon->RadiusKM;

        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->PlanetScale = PlanetRadiusKM * 100000;
        WorldScapeRootInstance->DistanceToFreezeGeneration = PlanetRadiusKM;


        WorldScapeRootInstance->SetActorLocation(NewMoon->GetActorLocation());
        WorldScapeRootInstance->SetActorRotation(NewMoon->GetActorRotation());
        WorldScapeRootInstance->AttachToActor(NewMoon, FAttachmentTransformRules::KeepWorldTransform);
        //NewPlanet->Mesh
        //NewPlanet->SetupWorldScapeRoot(WorldScapeRootInstance);

        //WorldScapeRootInstance->bGenerateWorldScape = true;
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldScape!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }

}

void APlanetaryEnvironmentGenerator::SpawnPlanetEnvironment()
{
    if (WorldScapeRootInstance)
    {

        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("SpawnPlanetEnvironment!"));
        WorldScapeRootInstance->bGenerateWorldScape = true;
        WorldScapeRootInstance->SetActorHiddenInGame(false);    
        WorldScapeRootInstance->SetActorTickEnabled(true);
        WorldScapeRootInstance->SetActorEnableCollision(true);

        if (WorldScapeRootInstance->GetAttachParentActor() != PlanetaryBody)
        {
            WorldScapeRootInstance->SetActorLocation(FVector(0.0, 0.0, 0.0));
            WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("WorldScapeRootInstance respawned!"));
        }

        /*if (WorldScapeRootInstance->GetAttachParentActor() != PlanetaryBody)
        {
            WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);
            WorldScapeRootInstance->SetActorLocation(FVector(0.0, 0.0, 0.0));
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("WorldScapeRootInstance respawned!"));

        }*/

        //if not attached 
       /* if (WorldScapeRootInstance->GetAttachParent() != PlanetaryBody)
        {
            WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);
        }
        WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);
        WorldScapeRootInstance->SetActorLocation(FVector(0.0));*/
    }
}

void APlanetaryEnvironmentGenerator::DestroyPlanetEnvironment()
{
    if (WorldScapeRootInstance)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("DestroyPlanetEnvironment!"));
        WorldScapeRootInstance->bGenerateWorldScape = false;
        //WorldScapeRootInstance->SetHidden(true);// = true;
        WorldScapeRootInstance->SetActorHiddenInGame(true);
        WorldScapeRootInstance->SetActorTickEnabled(false);
        WorldScapeRootInstance->SetActorEnableCollision(false);

        WorldScapeRootInstance->DetachRootComponentFromParent();
    }

}

//void APlanetEnvironmentGenerator::SetupDefaultGenerator()
//{
//    WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
//    WorldScapeRootInstance->bGenerateWorldScape = true;
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("SetupDefaultGenerator!"));
//}