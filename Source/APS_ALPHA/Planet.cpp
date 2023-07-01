// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"
#include "PlanetGenerationModel.h"



APlanet::APlanet()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetaryZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetaryZoneComponent"));
	PlanetaryZone->SetupAttachment(RootComponent);

	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetGravityCollisionZoneComponent"));
	GravityCollisionZone->SetupAttachment(RootComponent);

	/*WorldScapeRootInstance = CreateDefaultSubobject<AWorldScapeRoot>(TEXT("WorldScapeRoot"));
	WorldScapeRootInstance->SetupAttachment(RootComponent);*/
}

void APlanet::BeginPlay()
{
	PlanetEnvironmentGenerator = NewObject<APlanetEnvironmentGenerator>();
	if (PlanetEnvironmentGenerator)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PlanetSurfaceGenerator has been created successfully."));
		UE_LOG(LogTemp, Warning, TEXT("PlanetSurfaceGenerator has been created successfully."));
		PlanetEnvironmentGenerator->InitWorldScape();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to create PlanetSurfaceGenerator."));
		UE_LOG(LogTemp, Warning, TEXT("Failed to create PlanetSurfaceGenerator."));
	}


	//WorldScapeRootInstance = NewObject<AWorldScapeRoot>();
	//if (WorldScapeRootInstance)
	//{
	//	// TODO: Добавьте любую дополнительную логику инициализации здесь
	//	GEngine->AddOnScreenDebugMessage(-1, 51.f, FColor::Red, TEXT("WorldScapeRootInstance has been created successfully."));
	//	UE_LOG(LogTemp, Warning, TEXT("WorldScapeRootInstance has been created successfully."));
	//}
	//else
	//{
	//	// TODO: Добавьте обработку ошибок, если NewObject вернул nullptr
	//	GEngine->AddOnScreenDebugMessage(-1, 51.f, FColor::Red, TEXT("Failed to create WorldScapeRootInstance."));
	//	UE_LOG(LogTemp, Warning, TEXT("Failed to create WorldScapeRootInstance."));
	//}
}

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
