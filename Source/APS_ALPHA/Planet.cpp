// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"
#include "PlanetGenerationModel.h"

void APlanet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PlayerPawn != nullptr)
	{
		double Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
		if (Distance < AffectionRadiusKM * WSCZoneScale * 100000)
		{
			if (!bEnvironmentSpawned)
			{
				PlanetaryEnvironmentGenerator->SpawnPlanetEnvironment();
				bEnvironmentSpawned = true;
			}
		}
		else
		{
			if (bEnvironmentSpawned)
			{
				PlanetaryEnvironmentGenerator->DestroyPlanetEnvironment();
				bEnvironmentSpawned = false;
			}
		}
	}
}

void APlanet::CheckPlayerPawn()
{
	// Ищем PlayerPawn в мире
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		PlayerPawn = PlayerController->GetPawn();
	}

	// Если PlayerPawn найден, проверяем расстояние и останавливаем таймер
	if (PlayerPawn != nullptr)
	{
		GetWorldTimerManager().ClearTimer(PlayerPawnTimerHandle);

		double Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
		//GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Magenta, FString::Printf(TEXT("Planet Distance to player: %f"), Distance));
		if (Distance < AffectionRadiusKM * WSCZoneScale * 100000)
		{
			PlanetaryEnvironmentGenerator->SpawnPlanetEnvironment();
			bEnvironmentSpawned = true;
		}
		else
		{
			PlanetaryEnvironmentGenerator->DestroyPlanetEnvironment();
			bEnvironmentSpawned = false;
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0, FColor::Magenta, TEXT("PlayerPawn nullptr!"));
	}
}





void APlanet::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(PlayerPawnTimerHandle, this, &APlanet::CheckPlayerPawn, 1.0f, true);

	//if (PlayerPawn != nullptr)
	//{
	//	double Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
	//	//GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Magenta, FString::Printf(TEXT("Planet Distance to player: %f"), Distance));
	//	if (Distance < AffectionRadiusKM * WSCZoneScale * 100000)
	//	{
	//		PlanetaryEnvironmentGenerator->SpawnPlanetEnvironment();
	//		bEnvironmentSpawned = true;
	//	}
	//	else
	//	{
	//		PlanetaryEnvironmentGenerator->DestroyPlanetEnvironment();
	//		bEnvironmentSpawned = false;
	//	}
	//}
	//else
	//{
	//	GEngine->AddOnScreenDebugMessage(-1, 20.0, FColor::Magenta, TEXT("PlayerPawn nullptr!"));
	//}
}

APlanet::APlanet()
{
	PrimaryActorTick.bCanEverTick = true;
	
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetaryZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetaryZoneComponent"));
	PlanetaryZone->SetupAttachment(RootComponent);

	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetGravityCollisionZoneComponent"));
	GravityCollisionZone->SetupAttachment(RootComponent);



}

bool APlanet::IsNotGasGiant()
{
	return  PlanetType != EPlanetType::GasGiant 
		&& PlanetType != EPlanetType::HotGiant
		&& PlanetType != EPlanetType::IceGiant;
}

//void APlanet::BeginPlay()
//{
//	PlanetEnvironmentGenerator = NewObject<APlanetEnvironmentGenerator>();
//	if (PlanetEnvironmentGenerator)
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PlanetSurfaceGenerator has been created successfully."));
//		UE_LOG(LogTemp, Warning, TEXT("PlanetSurfaceGenerator has been created successfully."));
//		
//		if (bGenerateByDefault)
//		{
//			UWorld* World = GetWorld();
//			if (World)
//			{
//				PlanetEnvironmentGenerator->InitWorldScape(World);
//				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("World Scape Initiated!"));
//			}
//			else
//			{
//				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World IS NULL!!!"));
//			}
//		}
//	}
//	else
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to create PlanetSurfaceGenerator."));
//		UE_LOG(LogTemp, Warning, TEXT("Failed to create PlanetSurfaceGenerator."));
//	}
//}

void APlanet::AddMoon(AMoon* Moon)
{
	Moons.Add(Moon);
}

void APlanet::SetPlanetType(EPlanetType NewPlanetType)
{
	this->PlanetType = NewPlanetType;
}

void APlanet::SetPlanetZone(EPlanetaryZoneType NewPlanetZone)
{
	this->PlanetZone = NewPlanetZone;
}

void APlanet::SetPlanetDensity(double NewPlanetDensity)
{
	this->PlanetDensity = NewPlanetDensity;
}

void APlanet::SetPlanetGravityStrength(double NewPlanetGravityStrength)
{
	this->PlanetGravityStrength = NewPlanetGravityStrength;
}

void APlanet::SetTemperature(double NewTemperature)
{
	this->Temperature = NewTemperature;
}

void APlanet::SetAmountOfMoons(int NewAmountOfMoons)
{
	this->AmountOfMoons = NewAmountOfMoons;
}

void APlanet::SetMoonsList(TArray<TSharedPtr<FMoonData>> NewMoonsList)
{
	this->MoonsList = NewMoonsList;
}

void APlanet::SetParentStar(AStar* Star)
{
	this->ParnetStar = Star;
}
