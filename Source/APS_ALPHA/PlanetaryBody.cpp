// Fill out your copyright notice in the Description page of Project Settings.
#include "PlanetaryBody.h"

#include "PlanetaryEnvironmentGenerator.h"

APlanetaryBody::APlanetaryBody()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlanetaryBody::BeginPlay()
{
	Super::BeginPlay();
	
	PlanetaryEnvironmentGenerator = NewObject<APlanetaryEnvironmentGenerator>();
	if (PlanetaryEnvironmentGenerator)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("PlanetSurfaceGenerator has been created successfully."));

		if (bGenerateByDefault)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				PlanetaryEnvironmentGenerator->InitWorldScape(World);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("World Scape Initiated!"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World IS NULL!!!"));
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to create PlanetSurfaceGenerator."));
		UE_LOG(LogTemp, Warning, TEXT("Failed to create PlanetSurfaceGenerator."));
	}

	//PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

}

void APlanetaryBody::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


