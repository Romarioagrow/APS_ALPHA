#include "APSCivilizationStarterActors.h"

#include "APSCivilizationIdentityComponent.h"
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
		Component->SetMobility(EComponentMobility::Static);
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

FTransform AAPSCivilizationLandingPad::GetCharacterSpawnTransform() const
{
	return CharacterSpawnPoint
		? CharacterSpawnPoint->GetComponentTransform()
		: GetActorTransform();
}
