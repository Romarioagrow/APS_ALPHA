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
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PlanetSurfaceGenerator has been created successfully."));
		UE_LOG(LogTemp, Warning, TEXT("PlanetSurfaceGenerator has been created successfully."));

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

	PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

}

void APlanetaryBody::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if (PlayerPawn != nullptr)
	//{
	//	double Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
	//	//GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Magenta, FString::Printf(TEXT("Planet Distance to player: %f"), Distance));
	//	if (Distance < AffectionRadiusKM * WSCZoneScale * 100000)
	//	{
	//		if (!bEnvironmentSpawned)
	//		{
	//			PlanetaryEnvironmentGenerator->SpawnPlanetEnvironment();
	//			bEnvironmentSpawned = true;
	//		}
	//	}
	//	else
	//	{
	//		if (bEnvironmentSpawned)
	//		{
	//			PlanetaryEnvironmentGenerator->DestroyPlanetEnvironment();
	//			bEnvironmentSpawned = false;
	//		}
	//	}
	//}
	//else
	//{
	//	GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Magenta, TEXT("PlayerPawn nullptr!"));

	//}
	
	//if (PlayerPawn != nullptr)
	//{
	//	double Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
	//	//GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Magenta, FString::Printf(TEXT("Planet Distance to player: %f"), Distance));
	//	if (Distance < AffectionRadiusKM * WSCZoneScale * 100000)
	//	{
	//		if (!bEnvironmentSpawned)
	//		{
	//			PlanetaryEnvironmentGenerator->SpawnPlanetEnvironment();
	//			bEnvironmentSpawned = true;
	//		}
	//	}
	//	else
	//	{
	//		if (bEnvironmentSpawned)
	//		{
	//			PlanetaryEnvironmentGenerator->DestroyPlanetEnvironment();
	//			bEnvironmentSpawned = false;
	//		}
	//	}
	//}
	//else
	//{
	//	GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Magenta, TEXT("PlayerPawn nullptr!"));

	//}

	/*if (PlayerPawn != nullptr)
	{
		double Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
		if (Distance < AffectionRadiusKM * WSCZoneScale * 100000)
		{
			PlanetaryEnvironmentGenerator->SpawnPlanetEnvironment();
		}
		else
		{
			PlanetaryEnvironmentGenerator->DestroyPlanetEnvironment();
		}
	}*/
}


