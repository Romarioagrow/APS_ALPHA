#include "SpaceStation.h"

ASpaceStation::ASpaceStation()
{
	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("StationGravitySphereCollisionComponent"));
	GravityCollisionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GravityCollisionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	GravityCollisionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GravityCollisionZone->SetGenerateOverlapEvents(true);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(GravityCollisionZone);
	SpawnPoint->SetWorldRotation(GetActorRotation());
}

void ASpaceStation::BeginPlay()
{
	Super::BeginPlay();

	// Enforce the finite gravity-volume contract even when an old station
	// Blueprint serialized a different collision profile.
	GravityCollisionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GravityCollisionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	GravityCollisionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GravityCollisionZone->SetGenerateOverlapEvents(true);
}
