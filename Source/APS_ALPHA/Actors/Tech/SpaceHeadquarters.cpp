// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceHeadquarters.h"

ASpaceHeadquarters::ASpaceHeadquarters()
{
	// Создаем HQ и устанавливаем его как RootComponent
	HQ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HQ"));
	RootComponent = HQ;

	StartPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartPoint"));
	StartPoint->SetupAttachment(HQ);
}

void ASpaceHeadquarters::BeginPlay()
{
	//this->CalculateAffectionRadius();
}

FVector ASpaceHeadquarters::GetStartPointPosition()
{
	return StartPoint->GetComponentLocation();
}
