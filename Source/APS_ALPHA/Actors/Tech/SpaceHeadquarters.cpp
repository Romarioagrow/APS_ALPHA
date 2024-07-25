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
}

FVector ASpaceHeadquarters::GetStartPointPosition()
{
	return StartPoint->GetComponentLocation();
}
