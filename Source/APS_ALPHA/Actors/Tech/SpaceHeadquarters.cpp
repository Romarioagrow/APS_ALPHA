#include "SpaceHeadquarters.h"

ASpaceHeadquarters::ASpaceHeadquarters()
{
	// Создаем HQ и устанавливаем его как RootComponent
	HQ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HQ"));
	RootComponent = HQ;

	StartPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartPoint"));
	StartPoint->SetupAttachment(HQ);

	// The base class hangs PlayerStartPoint off the gravity sphere, which stops being the root
	// here; keep the pilot start on the HQ hull so it moves with the actor.
	PlayerStartPoint->SetupAttachment(HQ);
}

void ASpaceHeadquarters::BeginPlay()
{
	Super::BeginPlay();
}

FVector ASpaceHeadquarters::GetStartPointPosition()
{
	return StartPoint->GetComponentLocation();
}

FVector ASpaceHeadquarters::GetPlayerStartLocation() const
{
	if (IsValid(PlayerStartPoint) && !PlayerStartPoint->GetRelativeLocation().IsNearlyZero(1.0))
	{
		return PlayerStartPoint->GetComponentLocation();
	}
	return IsValid(StartPoint) ? StartPoint->GetComponentLocation() : Super::GetPlayerStartLocation();
}
