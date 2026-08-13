#include "StarSystem.h"

AStarSystem::AStarSystem()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	StarSystemZone = CreateDefaultSubobject<USphereComponent>(TEXT("StarSystemZoneComponent"));
	StarSystemZone->SetupAttachment(RootComponent);
	StarSystemZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StarSystemZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	StarSystemZone->SetGenerateOverlapEvents(false);
	StarSystemZone->SetCanEverAffectNavigation(false);
	StarSystemZone->SetVisibility(false);
	StarSystemZone->SetHiddenInGame(true);
}

void AStarSystem::BeginPlay()
{
	Super::BeginPlay();
	StarSystemZone->SetVisibility(false, true);
	StarSystemZone->SetHiddenInGame(true, true);
	// This is a logical system envelope consumed as radius data. Never let stale
	// Blueprint collision defaults turn the hidden envelope into a flight barrier.
	StarSystemZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StarSystemZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	StarSystemZone->SetGenerateOverlapEvents(false);
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
