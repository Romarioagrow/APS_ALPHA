#include "StarSystem.h"

AStarSystem::AStarSystem()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	StarSystemZone = CreateDefaultSubobject<USphereComponent>(TEXT("StarSystemZoneComponent"));
	StarSystemZone->SetupAttachment(RootComponent);
}

void AStarSystem::SetStarsAmount(int Amount)
{
	this->StarsAmount = Amount;
}

void AStarSystem::AddNewStar(AStar* NewStar)
{
	Stars.Add(NewStar);
}

void AStarSystem::SetStarSystemType(EStarType Type)
{
	this->StarSystemType = Type;
}
