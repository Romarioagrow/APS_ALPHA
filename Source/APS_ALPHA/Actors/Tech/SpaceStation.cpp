#include "SpaceStation.h"

ASpaceStation::ASpaceStation()
{
	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("StationGravitySphereCollisionComponent"));

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(GravityCollisionZone);
	SpawnPoint->SetWorldRotation(GetActorRotation());
}
