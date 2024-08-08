#include "Planet.h"
#include "Moon.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Generation/PlanetaryEnvironmentGenerator.h"

void APlanet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
		if (Distance < AffectionRadiusKM * WSCZoneScale * 100000)
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
}

void APlanet::DestroyWSC()
{
	if (!PlanetaryEnvironmentGenerator)
	{
		UE_LOG(LogTemp, Error, TEXT("PlanetaryEnvironmentGenerator is null!"));
		return;
	}

	if (!PlanetaryEnvironmentGenerator->WorldScapeRootInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("WorldScapeRootInstance is null!"));
		return;
	}

	PlanetaryEnvironmentGenerator->WorldScapeRootInstance->Destroy();
	EnableSphereMesh();

	for (AMoon* Moon : Moons)
	{
		Moon->PlanetaryEnvironmentGenerator->WorldScapeRootInstance->Destroy();
		Moon->EnableSphereMesh();
	}
}

void APlanet::InitWSC()
{
	UE_LOG(LogTemp, Warning, TEXT("InitWSC"));

	UWorld* World = GetWorld();
	if (World && IsNotGasGiant())
	{
		DisableSphereMesh();
		PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(World, this);
	}

	// for each moon init moon
	for (AMoon* Moon : Moons)
	{
		Moon->PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(World, Moon);
		Moon->DisableSphereMesh();
	}
}


void APlanet::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(PlayerPawnTimerHandle, this, &APlanet::CheckPlayerPawn, 1.0f, true);
}

APlanet::APlanet()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetaryZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetaryZoneComponent"));
	PlanetaryZone->SetupAttachment(RootComponent);

	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetGravityCollisionZoneComponent"));
	GravityCollisionZone->SetupAttachment(RootComponent);
}

bool APlanet::IsNotGasGiant()
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
	// Получаем первый статический меш компонент
	UStaticMeshComponent* SphereMesh = Cast<UStaticMeshComponent>(
		GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (SphereMesh)
	{
		// Отключаем его (делаем невидимым, например)
		SphereMesh->SetVisibility(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StaticMeshComponent not found!"));
	}
}

void APlanet::DisableSphereMesh()
{
	// Получаем первый статический меш компонент
	UStaticMeshComponent* SphereMesh = Cast<UStaticMeshComponent>(
		GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (SphereMesh)
	{
		// Отключаем его (делаем невидимым, например)
		SphereMesh->SetVisibility(false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StaticMeshComponent not found!"));
	}
}

void APlanet::ApplyNewPlanetParameters(AWorldScapeRoot* StartHomePlanet)
{
	this->PlanetRadiusKM = StartHomePlanet->PlanetScale / 100000;
	this->GravityCollisionZone->SetSphereRadius(0);
	this->PlanetaryZone->SetSphereRadius(0);

	PlanetaryZone->SetSphereRadius(100);
	AffectionRadiusKM = GetActorScale3D().X / 100000;
}
