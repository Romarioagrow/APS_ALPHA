#include "WorldActor.h"

// Sets default values
AWorldActor::AWorldActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AWorldActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWorldActor::CalculateAffectionRadius()
{
	FVector Origin, BoxExtent;
	double SphereRadius;
	this->GetActorBounds(false, Origin, BoxExtent);
	SphereRadius = BoxExtent.GetMax();
	this->AffectionRadiusKM = SphereRadius / 100000.0;

	FString RadiusMessage = FString::Printf(TEXT("Actor %s has a radius of %f km"), *(this->GetName()), AffectionRadiusKM);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, RadiusMessage);
}

/*FText AWorldActor::GetInGameName_Implementation() const
{
	return IItemInfoInterface::GetInGameName_Implementation();
}

UTexture2D* AWorldActor::GetAvatarPicture_Implementation() const
{
	return IItemInfoInterface::GetAvatarPicture_Implementation();
}*/


FText AWorldActor::GetInGameName_Implementation() const
{
	return InGameName;
}

UTexture2D* AWorldActor::GetAvatarPicture_Implementation() const
{
	return AvatarPicture;
}