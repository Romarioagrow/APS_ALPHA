#include "Moon.h"
#include "Components/StaticMeshComponent.h"

AMoon::AMoon()
{
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
	TInlineComponentArray<UStaticMeshComponent*> SphereMeshes;
	GetComponents(SphereMeshes);
	if (SphereMeshes.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("StaticMeshComponent not found!"));
		return;
	}

	for (UStaticMeshComponent* SphereMesh : SphereMeshes)
	{
		if (!IsValid(SphereMesh)) continue;
		SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		SphereMesh->SetHiddenInGame(true, false);
		SphereMesh->SetVisibility(false, false);
	}
}

void AMoon::EnableSphereMesh()
{
	TInlineComponentArray<UStaticMeshComponent*> SphereMeshes;
	GetComponents(SphereMeshes);
	if (SphereMeshes.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("StaticMeshComponent not found!"));
		return;
	}

	for (UStaticMeshComponent* SphereMesh : SphereMeshes)
	{
		if (!IsValid(SphereMesh)) continue;
		SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		SphereMesh->SetHiddenInGame(false, false);
		SphereMesh->SetVisibility(true, false);
	}
}
