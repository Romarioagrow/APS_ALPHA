#include "Colony.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AColony::AColony()
{
	PrimaryActorTick.bCanEverTick = false;
	InGameName = NSLOCTEXT("APSCivilization", "GroundSettlement", "Ground Settlement");

	ColonyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ColonyRoot"));
	RootComponent = ColonyRoot;
	Foundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SettlementFoundation"));
	HabitatDome = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HabitatDome"));
	Foundation->SetupAttachment(ColonyRoot);
	HabitatDome->SetupAttachment(ColonyRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CylinderMesh.Succeeded())
	{
		Foundation->SetStaticMesh(CylinderMesh.Object);
		Foundation->SetRelativeScale3D(FVector(30.0, 30.0, 2.0));
	}
	if (SphereMesh.Succeeded())
	{
		HabitatDome->SetStaticMesh(SphereMesh.Object);
		HabitatDome->SetRelativeLocation(FVector(0.0, 0.0, 600.0));
		HabitatDome->SetRelativeScale3D(FVector(14.0, 14.0, 7.0));
	}

	const FVector TowerLocations[] = {
		FVector(1900.0, 0.0, 700.0), FVector(-950.0, 1650.0, 700.0),
		FVector(-950.0, -1650.0, 700.0)};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(TowerLocations); ++Index)
	{
		UStaticMeshComponent* Tower = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("SettlementTower%d"), Index + 1));
		Tower->SetupAttachment(ColonyRoot);
		if (CylinderMesh.Succeeded())
		{
			Tower->SetStaticMesh(CylinderMesh.Object);
			Tower->SetRelativeLocation(TowerLocations[Index]);
			Tower->SetRelativeScale3D(FVector(4.5, 4.5, 12.0));
		}
		Towers.Add(Tower);
	}
}

