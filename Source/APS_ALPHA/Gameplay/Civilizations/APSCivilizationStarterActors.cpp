#include "APSCivilizationStarterActors.h"

#include "APSCivilizationIdentityComponent.h"
#include "APSStarterDressing.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void ConfigureWalkablePrimitive(UStaticMeshComponent* Component)
	{
		if (!Component)
		{
			return;
		}
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(true);
		Component->SetSimulatePhysics(false);
		Component->SetEnableGravity(false);
	}
}

AAPSCivilizationBaseModule::AAPSCivilizationBaseModule()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Tags.AddUnique(TEXT("APS.Civilization.Base"));
	Tags.AddUnique(TEXT("APS.Placeholder.Modular"));

	ModuleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModuleRoot"));
	SetRootComponent(ModuleRoot);
	CivilizationIdentity = CreateDefaultSubobject<UAPSCivilizationIdentityComponent>(
		TEXT("CivilizationIdentity"));
	Foundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Foundation"));
	Habitat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Habitat"));
	Airlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Airlock"));
	Foundation->SetupAttachment(ModuleRoot);
	Habitat->SetupAttachment(ModuleRoot);
	Airlock->SetupAttachment(ModuleRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Foundation->SetStaticMesh(Cube.Object);
		Habitat->SetStaticMesh(Cube.Object);
		Airlock->SetStaticMesh(Cube.Object);
	}
	Foundation->SetRelativeLocation(FVector(0.0, 0.0, 50.0));
	Foundation->SetRelativeScale3D(FVector(80.0, 55.0, 1.0));
	Habitat->SetRelativeLocation(FVector(-800.0, 0.0, 650.0));
	Habitat->SetRelativeScale3D(FVector(45.0, 38.0, 11.0));
	Airlock->SetRelativeLocation(FVector(1900.0, 0.0, 350.0));
	Airlock->SetRelativeScale3D(FVector(10.0, 14.0, 6.0));
	ConfigureWalkablePrimitive(Foundation);
	ConfigureWalkablePrimitive(Habitat);
	ConfigureWalkablePrimitive(Airlock);
}

AAPSCivilizationLandingPad::AAPSCivilizationLandingPad()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Tags.AddUnique(TEXT("APS.Civilization.LandingPad"));
	Tags.AddUnique(TEXT("APS.Placeholder.Modular"));

	PadRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PadRoot"));
	SetRootComponent(PadRoot);
	CivilizationIdentity = CreateDefaultSubobject<UAPSCivilizationIdentityComponent>(
		TEXT("CivilizationIdentity"));
	Deck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Deck"));
	AccessRamp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccessRamp"));
	CharacterSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterSpawnPoint"));
	Deck->SetupAttachment(PadRoot);
	AccessRamp->SetupAttachment(PadRoot);
	CharacterSpawnPoint->SetupAttachment(PadRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cylinder.Succeeded())
	{
		Deck->SetStaticMesh(Cylinder.Object);
	}
	if (Cube.Succeeded())
	{
		AccessRamp->SetStaticMesh(Cube.Object);
	}
	Deck->SetRelativeLocation(FVector(0.0, 0.0, 25.0));
	Deck->SetRelativeScale3D(FVector(90.0, 90.0, 0.5));
	AccessRamp->SetRelativeLocation(FVector(-5600.0, 0.0, -35.0));
	AccessRamp->SetRelativeRotation(FRotator(0.0, 0.0, -4.0));
	AccessRamp->SetRelativeScale3D(FVector(25.0, 18.0, 0.35));
	CharacterSpawnPoint->SetRelativeLocation(FVector(-6500.0, 0.0, 300.0));
	ConfigureWalkablePrimitive(Deck);
	ConfigureWalkablePrimitive(AccessRamp);
}

void AAPSCivilizationBaseModule::BeginPlay()
{
	Super::BeginPlay();
	using namespace APSStarterDressing;
	// Foundation 80 x 55 m (top at 1 m), hall 45 x 38 x 11 m centred at x -8 m, airlock 10 x 14 x 6 m at x +19 m: every
	// addition stays inside the foundation's footprint, which the colony's placement measures.
	Paint(this, Foundation, Metal);
	Paint(this, Habitat, Hull);
	Paint(this, Airlock, Hull);
	for (const double Side : {-1.0, 1.0})
	{
		// Warm windows along both long walls between dark bands, and a dark band under the roof edge.
		Part(this, ModuleRoot, EShape::Cube, FVector(-800.0, Side * 1906.0, 820.0), FVector(3900.0, 12.0, 110.0), Warm, 6.0f);
		Part(this, ModuleRoot, EShape::Cube, FVector(-800.0, Side * 1904.0, 700.0), FVector(4420.0, 10.0, 40.0), Dark);
		Part(this, ModuleRoot, EShape::Cube, FVector(-800.0, Side * 1904.0, 940.0), FVector(4420.0, 10.0, 40.0), Dark);
		Part(this, ModuleRoot, EShape::Cube, FVector(-800.0, Side * 1904.0, 1165.0), FVector(4500.0, 12.0, 70.0), Metal);
	}
	// Solar panels on the hall's roof.
	for (const double X : {-2550.0, -1800.0, -1050.0, -300.0})
	{
		for (const double Y : {-950.0, 950.0})
		{
			Part(this, ModuleRoot, EShape::Cube, FVector(X, Y, 1225.0), FVector(650.0, 1500.0, 30.0), Solar);
		}
	}
	// The command tower at the airlock end, a ring of lit windows, the mast and its warning light.
	Part(this, ModuleRoot, EShape::Cylinder, FVector(800.0, 0.0, 1500.0), FVector(900.0, 900.0, 600.0), Hull);
	Part(this, ModuleRoot, EShape::Cylinder, FVector(800.0, 0.0, 1640.0), FVector(920.0, 920.0, 70.0), Warm, 5.0f);
	Part(this, ModuleRoot, EShape::Cylinder, FVector(800.0, 0.0, 1820.0), FVector(960.0, 960.0, 40.0), Metal);
	Part(this, ModuleRoot, EShape::Cylinder, FVector(800.0, 0.0, 2230.0), FVector(40.0, 40.0, 780.0), Metal);
	Part(this, ModuleRoot, EShape::Sphere, FVector(800.0, 0.0, 2640.0), FVector(70.0, 70.0, 70.0), Red, 12.0f);
	Lamp(this, ModuleRoot, FVector(800.0, 0.0, 2700.0), Red, 300.0f, 1500.0f);
	// The airlock door: an amber frame, the dark door and the light over it.
	Part(this, ModuleRoot, EShape::Cube, FVector(2403.0, 0.0, 340.0), FVector(8.0, 520.0, 520.0), Amber);
	Part(this, ModuleRoot, EShape::Cube, FVector(2408.0, 0.0, 320.0), FVector(8.0, 380.0, 440.0), Dark);
	Part(this, ModuleRoot, EShape::Cube, FVector(2410.0, 0.0, 590.0), FVector(8.0, 220.0, 30.0), Warm, 8.0f);
	Lamp(this, ModuleRoot, FVector(2650.0, 0.0, 520.0), Warm, 500.0f, 2200.0f);
	// Hazard markings on the foundation's edge in front of the airlock.
	for (const double Y : {-2400.0, -1600.0, -800.0, 0.0, 800.0, 1600.0, 2400.0})
	{
		Part(this, ModuleRoot, EShape::Cube, FVector(3900.0, Y, 104.0), FVector(160.0, 400.0, 8.0), Amber);
	}
	// Floodlights on the foundation's corners.
	for (const double X : {-3850.0, 3850.0})
	{
		for (const double Y : {-2600.0, 2600.0})
		{
			Part(this, ModuleRoot, EShape::Cylinder, FVector(X, Y, 300.0), FVector(30.0, 30.0, 400.0), Metal);
			Part(this, ModuleRoot, EShape::Cube, FVector(X, Y, 520.0), FVector(70.0, 70.0, 40.0), Warm, 6.0f);
			Lamp(this, ModuleRoot, FVector(X, Y, 560.0), Warm, 800.0f, 3000.0f);
		}
	}
}

void AAPSCivilizationLandingPad::BeginPlay()
{
	Super::BeginPlay();
	using namespace APSStarterDressing;
	// The deck is a 90 m disc with its top at 50 cm; the actor's X/Y scale grows it for bigger ships and these markings
	// with it. They lie 2-4 cm proud of the deck: the ship is set on the deck's top.
	Paint(this, Deck, APSStarterDressing::Deck);
	Paint(this, AccessRamp, Metal);
	constexpr double Top = 50.0;
	for (int32 Dash = 0; Dash < 24; ++Dash)
	{
		const double Angle = 2.0 * PI * Dash / 24.0;
		Part(this, PadRoot, EShape::Cube, FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * 4250.0 + FVector(0.0, 0.0, Top + 2.0),
			FVector(140.0, 520.0, 4.0), Amber, 0.0f, FRotator(0.0, FMath::RadiansToDegrees(Angle), 0.0));
	}
	for (int32 Dash = 0; Dash < 16; ++Dash)
	{
		const double Angle = 2.0 * PI * (Dash + 0.5) / 16.0;
		Part(this, PadRoot, EShape::Cube, FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * 1900.0 + FVector(0.0, 0.0, Top + 2.0),
			FVector(60.0, 420.0, 4.0), Marking, 0.0f, FRotator(0.0, FMath::RadiansToDegrees(Angle), 0.0));
	}
	// The H, its bar across the ramp's line so a pilot coming in reads it.
	Part(this, PadRoot, EShape::Cube, FVector(0.0, -650.0, Top + 2.0), FVector(1800.0, 180.0, 4.0), Marking);
	Part(this, PadRoot, EShape::Cube, FVector(0.0, 650.0, Top + 2.0), FVector(1800.0, 180.0, 4.0), Marking);
	Part(this, PadRoot, EShape::Cube, FVector(0.0, 0.0, Top + 2.0), FVector(180.0, 1300.0, 4.0), Marking);
	// Landing lights round the rim: green glow tiles, four of them lit.
	for (int32 Index = 0; Index < 12; ++Index)
	{
		const double Angle = 2.0 * PI * Index / 12.0;
		const FVector Rim = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * 4420.0;
		Part(this, PadRoot, EShape::Cube, Rim + FVector(0.0, 0.0, Top + 2.0), FVector(70.0, 70.0, 4.0), Green, 8.0f);
		if (Index % 3 == 0)
		{
			Lamp(this, PadRoot, Rim + FVector(0.0, 0.0, Top + 120.0), Green, 250.0f, 1600.0f);
		}
	}
}

FTransform AAPSCivilizationLandingPad::GetCharacterSpawnTransform() const
{
	return CharacterSpawnPoint
		? CharacterSpawnPoint->GetComponentTransform()
		: GetActorTransform();
}
