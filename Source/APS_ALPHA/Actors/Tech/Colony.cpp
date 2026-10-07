#include "Colony.h"

#include "APS_ALPHA/Gameplay/Civilizations/APSStarterDressing.h"

#include "Components/ChildActorComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"

namespace APSColonyHeadquarters
{
	TAutoConsoleVariable<int32> CVarHeadquarters(TEXT("aps.Colony.HQ"), 1,
		TEXT("1: the home colony's main building is BP_ColonyHQ on its 80 x 55 m foundation (Rio 03.10); ")
		TEXT("0: the compact dome, towers and disc. Applies to colonies founded or loaded afterwards."));
	const TCHAR* const BuildingClassPath = TEXT("/Game/APS/APS_ALPHA/Assets/ColonyHQ/BP_ColonyHQ.BP_ColonyHQ_C");
	/** The foundation under the building: +X (toward the landing pad) by Y by height, its top where the floors start. */
	const FVector FoundationSizeCm(8000.0, 5500.0, 100.0);
	/** A new surface start's pilot (Rio 03.10), from HQ_Spots.json: the hall spans x -800..1200 and y -700..700 on the
	 * 105 cm floor. In its front half, between the kiosks and the sofas, facing the holotable and the console row (-X). */
	const FVector ArrivalSpotCm(950.0, 0.0, 105.0);
	constexpr double ArrivalYawDegrees = 180.0;
}

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

bool AColony::UseHeadquartersLook()
{
	using namespace APSColonyHeadquarters;
	if (HeadquartersBuilding.IsValid())
	{
		return true;
	}
	if (CVarHeadquarters.GetValueOnGameThread() == 0)
	{
		return false;
	}
	UClass* BuildingClass = LoadClass<AActor>(nullptr, BuildingClassPath);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!BuildingClass || !Cube || !Foundation || !ColonyRoot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Colony] %s: headquarters %s is missing; the compact base stays"),
			*GetName(), BuildingClassPath);
		return false;
	}
	// Everything of the compact look goes: the dome, the towers and the dressing BeginPlay hung on them.
	HiddenCompactParts.Reset();
	HiddenCompactLights.Reset();
	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive && Primitive != Foundation && Primitive->IsVisible())
		{
			HiddenCompactParts.Emplace(Primitive, static_cast<uint8>(Primitive->GetCollisionEnabled()));
			Primitive->SetVisibility(false, false);
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	TInlineComponentArray<ULightComponent*> Lights(this);
	for (ULightComponent* Light : Lights)
	{
		if (Light && Light->IsVisible())
		{
			HiddenCompactLights.Add(Light);
			Light->SetVisibility(false, false);
		}
	}
	// The foundation: its underside at the pivot and its top 1 m up, where the building's floors start.
	Foundation->SetStaticMesh(Cube);
	Foundation->SetRelativeLocation(FVector(0.0, 0.0, FoundationSizeCm.Z * 0.5));
	Foundation->SetRelativeScale3D(FoundationSizeCm / 100.0);
	APSStarterDressing::Paint(this, Foundation, APSStarterDressing::Metal);

	UChildActorComponent* Building = NewObject<UChildActorComponent>(this, TEXT("ColonyHeadquarters"), RF_Transient);
	Building->SetupAttachment(ColonyRoot);
	Building->SetChildActorClass(BuildingClass);
	AddInstanceComponent(Building);
	Building->RegisterComponent();
	HeadquartersBuilding = Building;
	UE_LOG(LogTemp, Log, TEXT("[APS.Colony] %s: headquarters %s on an 80 x 55 m foundation (%d compact parts hidden)"),
		*GetName(), *GetNameSafe(Building->GetChildActor()), HiddenCompactParts.Num());
	return true;
}

bool AColony::GetHeadquartersArrival(FTransform& OutSpot) const
{
	using namespace APSColonyHeadquarters;
	const UChildActorComponent* Building = HeadquartersBuilding.Get();
	if (!Building || !IsValid(Building->GetChildActor()))
	{
		return false;
	}
	OutSpot = FTransform(FRotator(0.0, ArrivalYawDegrees, 0.0), ArrivalSpotCm) * Building->GetComponentTransform();
	OutSpot.SetScale3D(FVector::OneVector);
	return true;
}

void AColony::UseCompactLook()
{
	if (UChildActorComponent* Building = HeadquartersBuilding.Get())
	{
		Building->DestroyComponent();
	}
	HeadquartersBuilding.Reset();
	for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, uint8>& Part : HiddenCompactParts)
	{
		if (UPrimitiveComponent* Primitive = Part.Key.Get())
		{
			Primitive->SetVisibility(true, false);
			Primitive->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(Part.Value));
		}
	}
	for (const TWeakObjectPtr<ULightComponent>& Light : HiddenCompactLights)
	{
		if (ULightComponent* Each = Light.Get())
		{
			Each->SetVisibility(true, false);
		}
	}
	HiddenCompactParts.Reset();
	HiddenCompactLights.Reset();
	if (Foundation)
	{
		if (UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
		{
			Foundation->SetStaticMesh(Cylinder);
		}
		Foundation->SetRelativeLocation(FVector::ZeroVector);
		Foundation->SetRelativeScale3D(FVector(30.0, 30.0, 2.0));
		APSStarterDressing::Paint(this, Foundation, APSStarterDressing::Metal);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Colony] %s: compact base"), *GetName());
}
