// Fill out your copyright notice in the Description page of Project Settings.


#include "Star.h"

AStar::AStar()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetarySystemZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetarySystemZoneComponent"));
	PlanetarySystemZone->SetupAttachment(RootComponent);

	// Создайте и прикрепите компонент меша к корневому компоненту
	//StarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarMesh"));
	StarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarMesh"));
	StarMesh->SetupAttachment(RootComponent);
}

void AStar::BeginPlay()
{
	Super::BeginPlay();

	//StarDynamicMaterial = UMaterialInstanceDynamic::Create(StarMesh->GetMaterial(0), this);
	StarDynamicMaterial = UMaterialInstanceDynamic::Create(StarMesh->GetMaterial(0), this);
	if (StarDynamicMaterial)
	{
		StarMesh->SetMaterial(0, StarDynamicMaterial);
	}
}

void AStar::SetStarProperties(FLinearColor Color, float Multiplier)
{
	if (StarDynamicMaterial != nullptr)
	{
		StarDynamicMaterial->SetVectorParameterValue(FName("Color"), Color);
		StarDynamicMaterial->SetScalarParameterValue(FName("Multiplier"), Multiplier);
	}
}

void AStar::AddPlanet(APlanet* Planet)
{
	this->Planets.Add(Planet);
}

void AStar::SetPlanetarySystem(APlanetarySystem* NewPlanetarySystem)
{
	this->PlanetarySystem = NewPlanetarySystem;
}

void AStar::SetLuminosity(float StarLuminosity)
{
	this->Luminosity = StarLuminosity;
}

void AStar::SetSurfaceTemperature(int NewSurfaceTemperature)
{
	this->SurfaceTemperature = NewSurfaceTemperature;
}

void AStar::SetStarType(EStellarType NewStarType)
{
	this->StellarClass = NewStarType;
}

void AStar::SetStarSpectralClass(ESpectralClass NewStarSpectralClass)
{
	this->SpectralClass = NewStarSpectralClass;
}

void AStar::SetStarSpectralType(ESpectralType NewSpectralType)
{
	this->SpectralType = NewSpectralType;
}

void AStar::SetFullSpectralClass(FName NewFullSpectralClass)
{
	this->FullSpectralClass = NewFullSpectralClass;
}

void AStar::SetFullSpectralName(FName NewFullSpectralName)
{
	this->FullSpectralName = NewFullSpectralName;
}

void AStar::SetSpectralSubclass(int NewSpectralSubclass)
{
	this->SpectralSubclass = NewSpectralSubclass;
}
