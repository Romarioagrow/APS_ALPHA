#include "Planet.h"
#include "Moon.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	void ConfigureNonBlockingPlanetZone(USphereComponent* Zone)
	{
		if (!IsValid(Zone))
		{
			return;
		}

		// These components describe influence/gravity volumes.  They are queries,
		// never terrain: a Blueprint collision override must not turn either sphere
		// into the smooth inner floor below the displaced WorldScape surface.
		Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Zone->SetCollisionResponseToAllChannels(ECR_Ignore);
		Zone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Zone->SetGenerateOverlapEvents(true);
	}
}

void APlanet::HandleOnStellarMode()
{
	IsComputingWSCProximity = false;
}

void APlanet::CheckPlayerPawn()
{
	// Ищем PlayerPawn в мире
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		PlayerPawn = PlayerController->GetPawn();
	}

	// Если PlayerPawn найден, проверяем расстояние и останавливаем таймер
	if (PlayerPawn != nullptr && PlanetaryEnvironmentGenerator != nullptr)
	{
		GetWorldTimerManager().ClearTimer(PlayerPawnTimerHandle);

		double Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
		if (Distance < AffectionRadiusKM * WscZoneScale * 100000)
		{
			PlanetaryEnvironmentGenerator->SpawnWorldScapeRoot();
			bEnvironmentSpawned = true;
		}
		else
		{
			PlanetaryEnvironmentGenerator->DestroyPlanetEnvironment();
			bEnvironmentSpawned = false;
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0, FColor::Magenta, TEXT("PlayerPawn nullptr!"));
	}
}

void APlanet::SetManualPlanet(AWorldScapeRoot* StartHomePlanet)
{
	if (!IsValid(StartHomePlanet))
	{
		return;
	}
	if (APlanetarySurfaceGenerator* Generator = EnsurePlanetaryEnvironmentGenerator())
	{
		Generator->PlanetaryBody = this;
		Generator->WorldScapeRootInstance = StartHomePlanet;
		bEnvironmentSpawned = StartHomePlanet->bGenerateWorldScape && !StartHomePlanet->bFreezeGeneration;
	}
}

void APlanet::DestroyWSC()
{
	SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
}

void APlanet::InitWSC()
{
	// Kept as a compatibility entry point. Streaming owns the single active root;
	// it must never recursively create roots for every moon in the family.
	if (IsNotGasGiant())
	{
		SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
	}
}


void APlanet::BeginPlay()
{
	Super::BeginPlay();
	ConfigureNonBlockingPlanetZone(PlanetaryZone);
	ConfigureNonBlockingPlanetZone(GravityCollisionZone);
	PlanetaryZone->SetVisibility(false, true);
	PlanetaryZone->SetHiddenInGame(true, true);
	GravityCollisionZone->SetVisibility(false, true);
	GravityCollisionZone->SetHiddenInGame(true, true);
}

APlanet::APlanet()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetaryZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetaryZoneComponent"));
	PlanetaryZone->SetupAttachment(RootComponent);
	ConfigureNonBlockingPlanetZone(PlanetaryZone);
	PlanetaryZone->SetVisibility(false);
	PlanetaryZone->SetHiddenInGame(true);

	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetGravityCollisionZoneComponent"));
	GravityCollisionZone->SetupAttachment(RootComponent);
	ConfigureNonBlockingPlanetZone(GravityCollisionZone);
	GravityCollisionZone->SetVisibility(false);
	GravityCollisionZone->SetHiddenInGame(true);
}

bool APlanet::IsNotGasGiant() const
{
	return PlanetType != EPlanetType::GasGiant
		&& PlanetType != EPlanetType::HotGiant
		&& PlanetType != EPlanetType::IceGiant;
}

void APlanet::AddMoon(AMoon* Moon)
{
	Moons.Add(Moon);
}

void APlanet::SetPlanetType(EPlanetType NewPlanetType)
{
	this->PlanetType = NewPlanetType;
}

void APlanet::SetPlanetZone(EPlanetaryZoneType NewPlanetZone)
{
	this->PlanetZone = NewPlanetZone;
}

void APlanet::SetPlanetDensity(double NewPlanetDensity)
{
	this->PlanetDensity = NewPlanetDensity;
}

void APlanet::SetPlanetGravityStrength(double NewPlanetGravityStrength)
{
	this->PlanetGravityStrength = NewPlanetGravityStrength;
}

void APlanet::SetTemperature(double NewTemperature)
{
	this->Temperature = NewTemperature;
}

void APlanet::SetAmountOfMoons(int NewAmountOfMoons)
{
	this->AmountOfMoons = NewAmountOfMoons;
}

void APlanet::SetMoonsList(TArray<TSharedPtr<FMoonData>> NewMoonsList)
{
	this->MoonsList = NewMoonsList;
}

void APlanet::SetParentStar(AStar* Star)
{
	this->ParentStar = Star;
}

void APlanet::SetupHomePlanetFromEditor(AWorldScapeRoot* StartHomePlanet)
{
	RemoveAllChildrenRecursively(this);

	DisableSphereMesh();

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, true);
	StartHomePlanet->AttachToActor(this, AttachmentRules);
	StartHomePlanet->SetActorRelativeLocation(FVector(0.0, 0.0, 0.0));
	SetManualPlanet(StartHomePlanet);
}

void APlanet::RemoveAllChildrenRecursively(AActor* ParentActor)
{
	// Получаем всех дочерних акторов
	TArray<AActor*> AttachedActors;
	ParentActor->GetAttachedActors(AttachedActors);

	// Обходим всех дочерних акторов
	for (AActor* Actor : AttachedActors)
	{
		if (Actor)
		{
			// Удаляем всех дочерних акторов этого актора
			RemoveAllChildrenRecursively(Actor);

			// Удаляем сам актор
			Actor->Destroy();
		}
	}
}

void APlanet::EnableSphereMesh()
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
		// The authored globe is only a distant/loading visual. WorldScape owns all
		// walkable terrain, so this mesh must never become a smooth false ground shell.
		SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		SphereMesh->SetHiddenInGame(false, false);
		SphereMesh->SetVisibility(true, false);
	}
}

void APlanet::DisableSphereMesh()
{
	// Generated body Blueprints can contain more than one authored globe layer.
	// Hiding only GetComponentByClass() left the remaining material shell rendered
	// over the ready WorldScape root and visibly intersecting its terrain.
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

void APlanet::ApplyNewPlanetParameters(AWorldScapeRoot* StartHomePlanet)
{
	SetManualPlanet(StartHomePlanet);
	this->PlanetRadiusKM = StartHomePlanet->PlanetScale / 100000;
	this->GravityCollisionZone->SetSphereRadius(0);
	this->PlanetaryZone->SetSphereRadius(0);

	PlanetaryZone->SetSphereRadius(100);
	AffectionRadiusKM = GetActorScale3D().X / 100000;
}
