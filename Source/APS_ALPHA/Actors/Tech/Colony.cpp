#include "Colony.h"

#include "APS_ALPHA/Gameplay/Civilizations/APSStarterDressing.h"

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

void AColony::BeginPlay()
{
	Super::BeginPlay();
	// The home colony's base (the materialization's legacy placeholder) and the generated ground settlements, in the
	// colony modules' look (B8, Rio 01.10). Foundation 30 m across, a dome 14 m wide on it, three towers 19 m out: every
	// addition stays inside the towers' reach, which the colony's placement measures.
	using namespace APSStarterDressing;
	Paint(this, Foundation, Metal);
	Paint(this, HabitatDome, Hull);
	for (UStaticMeshComponent* Tower : Towers)
	{
		Paint(this, Tower, Metal);
		if (Tower)
		{
			const FVector Top = Tower->GetRelativeLocation() + FVector(0.0, 0.0, 600.0);
			Part(this, ColonyRoot, EShape::Cylinder, Top - FVector(0.0, 0.0, 160.0), FVector(462.0, 462.0, 40.0), Warm, 5.0f);
			Part(this, ColonyRoot, EShape::Sphere, Top + FVector(0.0, 0.0, 40.0), FVector(60.0, 60.0, 60.0), Red, 12.0f);
		}
	}
	Lamp(this, ColonyRoot, FVector(1900.0, 0.0, 1360.0), Red, 250.0f, 1500.0f);
	// A belt of lit windows round the dome's widest part between dark bands.
	Part(this, ColonyRoot, EShape::Cylinder, FVector(0.0, 0.0, 600.0), FVector(1408.0, 1408.0, 70.0), Warm, 5.0f);
	Part(this, ColonyRoot, EShape::Cylinder, FVector(0.0, 0.0, 535.0), FVector(1406.0, 1406.0, 25.0), Dark);
	Part(this, ColonyRoot, EShape::Cylinder, FVector(0.0, 0.0, 665.0), FVector(1406.0, 1406.0, 25.0), Dark);
	// The airlock toward the first tower, its amber frame, door and light.
	Part(this, ColonyRoot, EShape::Cube, FVector(800.0, 0.0, 280.0), FVector(320.0, 340.0, 320.0), Hull);
	Part(this, ColonyRoot, EShape::Cube, FVector(963.0, 0.0, 270.0), FVector(8.0, 280.0, 280.0), Amber);
	Part(this, ColonyRoot, EShape::Cube, FVector(968.0, 0.0, 255.0), FVector(8.0, 200.0, 230.0), Dark);
	Part(this, ColonyRoot, EShape::Cube, FVector(970.0, 0.0, 400.0), FVector(8.0, 140.0, 22.0), Warm, 8.0f);
	Lamp(this, ColonyRoot, FVector(1150.0, 0.0, 420.0), Warm, 400.0f, 1800.0f);
	// Amber markings round the foundation's rim.
	for (int32 Dash = 0; Dash < 24; ++Dash)
	{
		const double Angle = 2.0 * PI * Dash / 24.0;
		Part(this, ColonyRoot, EShape::Cube, FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * 1420.0 + FVector(0.0, 0.0, 102.0),
			FVector(110.0, 300.0, 4.0), Amber, 0.0f, FRotator(0.0, FMath::RadiansToDegrees(Angle), 0.0));
	}
	// Two solar arrays between the towers, tilted to the sky.
	for (const double Side : {-1.0, 1.0})
	{
		const FVector Stand(-1000.0, Side * 700.0, 0.0);
		Part(this, ColonyRoot, EShape::Cylinder, Stand + FVector(0.0, 0.0, 200.0), FVector(20.0, 20.0, 200.0), Metal);
		Part(this, ColonyRoot, EShape::Cube, Stand + FVector(0.0, 0.0, 320.0), FVector(420.0, 300.0, 16.0), Solar, 0.0f,
			FRotator(-25.0, 0.0, 0.0));
	}
	// Lamp posts by the airlock.
	for (const double Side : {-1.0, 1.0})
	{
		const FVector Post(1200.0, Side * 600.0, 0.0);
		Part(this, ColonyRoot, EShape::Cylinder, Post + FVector(0.0, 0.0, 250.0), FVector(20.0, 20.0, 300.0), Metal);
		Part(this, ColonyRoot, EShape::Cube, Post + FVector(0.0, 0.0, 410.0), FVector(50.0, 50.0, 30.0), Warm, 6.0f);
		Lamp(this, ColonyRoot, Post + FVector(0.0, 0.0, 440.0), Warm, 600.0f, 2500.0f);
	}
}
