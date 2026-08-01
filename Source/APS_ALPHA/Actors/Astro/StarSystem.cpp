#include "StarSystem.h"

AStarSystem::AStarSystem()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	StarSystemZone = CreateDefaultSubobject<USphereComponent>(TEXT("StarSystemZoneComponent"));
	StarSystemZone->SetupAttachment(RootComponent);
	StarSystemZone->SetVisibility(false);
	StarSystemZone->SetHiddenInGame(true);
}

void AStarSystem::BeginPlay()
{
	Super::BeginPlay();
	StarSystemZone->SetVisibility(false, true);
	StarSystemZone->SetHiddenInGame(true, true);
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
