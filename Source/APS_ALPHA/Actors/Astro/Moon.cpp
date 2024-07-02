// Fill out your copyright notice in the Description page of Project Settings.


#include "Moon.h"

AMoon::AMoon()
{
	//OrbitAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("OrbitAnchor"));
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("MoonRootComponent"));
	RootComponent = Root;
}

void AMoon::SetParentPlanet(APlanet* Planet)
{
	this->ParentPlanet = Planet;
}

void AMoon::SetMoonType(EMoonType Type)
{
	this->MoonType = Type;
}

void AMoon::SetMoonDensity(double NewMoonDensity)
{
	this->MoonDensity = NewMoonDensity;
}

void AMoon::SetMoonGravity(double NewMoonGravity)
{
	this->MoonGravity = NewMoonGravity;
}

void AMoon::DisableSphereMesh()
{
	// Получаем первый статический меш компонент
	UStaticMeshComponent* SphereMesh = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (SphereMesh)
	{
		// Отключаем его (делаем невидимым, например)
		SphereMesh->SetVisibility(false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StaticMeshComponent not found!"));
	}
}

void AMoon::EnableSphereMesh()
{
	// Получаем первый статический меш компонент
	UStaticMeshComponent* SphereMesh = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (SphereMesh)
	{
		SphereMesh->SetVisibility(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StaticMeshComponent not found!"));
	}
}
