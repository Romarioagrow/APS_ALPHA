#include "AutonomousOutpost.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAutonomousOutpost::AAutonomousOutpost()
{
	PrimaryActorTick.bCanEverTick = false;
	InGameName = NSLOCTEXT("APSCivilization", "AutonomousOutpost", "Autonomous Outpost");

	OutpostRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OutpostRoot"));
	RootComponent = OutpostRoot;
	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OutpostCore"));
	SolarWingA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SolarWingA"));
	SolarWingB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SolarWingB"));
	Antenna = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OutpostAntenna"));
	Core->SetupAttachment(OutpostRoot);
	SolarWingA->SetupAttachment(OutpostRoot);
	SolarWingB->SetupAttachment(OutpostRoot);
	Antenna->SetupAttachment(OutpostRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (CylinderMesh.Succeeded())
	{
		Core->SetStaticMesh(CylinderMesh.Object);
		Core->SetRelativeScale3D(FVector(12.0, 12.0, 4.0));
	}
	if (CubeMesh.Succeeded())
	{
		SolarWingA->SetStaticMesh(CubeMesh.Object);
		SolarWingB->SetStaticMesh(CubeMesh.Object);
		SolarWingA->SetRelativeLocation(FVector(0.0, 2000.0, 0.0));
		SolarWingB->SetRelativeLocation(FVector(0.0, -2000.0, 0.0));
		SolarWingA->SetRelativeScale3D(FVector(3.0, 16.0, 0.35));
		SolarWingB->SetRelativeScale3D(FVector(3.0, 16.0, 0.35));
	}
	if (ConeMesh.Succeeded())
	{
		Antenna->SetStaticMesh(ConeMesh.Object);
		Antenna->SetRelativeLocation(FVector(0.0, 0.0, 850.0));
		Antenna->SetRelativeScale3D(FVector(2.0, 2.0, 6.0));
	}
}

