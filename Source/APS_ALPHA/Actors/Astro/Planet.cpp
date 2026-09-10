#include "Planet.h"
#include "Moon.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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

	GasGiantVisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("GasGiantVisualComponent"));
	GasGiantVisualComponent->SetupAttachment(RootComponent);
	GasGiantVisualComponent->ComponentTags.Add(TEXT("APS.GasGiantVisual"));
	GasGiantVisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GasGiantVisualComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	GasGiantVisualComponent->SetGenerateOverlapEvents(false);
	GasGiantVisualComponent->SetCanEverAffectNavigation(false);
	GasGiantVisualComponent->CastShadow = true;
	GasGiantVisualComponent->SetVisibility(false);
	GasGiantVisualComponent->SetHiddenInGame(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GasGiantMesh(
		TEXT("/Game/APS/APS_ALPHA/Assets/AI_PLanetss/giant_icosphere_jupiter1to1_sub6.giant_icosphere_jupiter1to1_sub6"));
	if (GasGiantMesh.Succeeded())
	{
		GasGiantVisualComponent->SetStaticMesh(GasGiantMesh.Object);
	}
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
	RefreshGasGiantVisual();
	if (!IsNotGasGiant())
	{
		// Gas giants never enter WorldScape, so their lightweight mesh is the
		// authoritative surface from the first frame.
		EnableSphereMesh();
	}
}

void APlanet::RefreshGasGiantVisual()
{
	if (!IsValid(GasGiantVisualComponent)
		|| !IsValid(GasGiantVisualComponent->GetStaticMesh()))
	{
		return;
	}

	// The restored legacy Jupiter mesh was imported at a physical authoring scale.
	// Normalize it to the same local radius as the Blueprint sphere so all existing
	// actor kilometre/presentation scaling remains authoritative.
	double ReferenceLocalRadius = 50.0;
	TInlineComponentArray<UStaticMeshComponent*> Meshes;
	GetComponents(Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh) || Mesh == GasGiantVisualComponent
			|| !IsValid(Mesh->GetStaticMesh()))
		{
			continue;
		}
		const double CandidateRadius = Mesh->GetStaticMesh()->GetBounds().SphereRadius
			* Mesh->GetRelativeScale3D().GetAbsMax();
		if (FMath::IsFinite(CandidateRadius) && CandidateRadius > UE_SMALL_NUMBER)
		{
			ReferenceLocalRadius = CandidateRadius;
			break;
		}
	}
	const double SourceRadius = GasGiantVisualComponent->GetStaticMesh()
		->GetBounds().SphereRadius;
	if (FMath::IsFinite(SourceRadius) && SourceRadius > UE_SMALL_NUMBER)
	{
		GasGiantVisualComponent->SetRelativeScale3D(FVector(
			ReferenceLocalRadius / SourceRadius));
	}
	GasGiantVisualComponent->SetRelativeLocation(FVector::ZeroVector);

	if (!IsValid(GasGiantMaterialInstance))
	{
		UMaterialInterface* BaseMaterial = GasGiantVisualComponent->GetMaterial(0);
		if (IsValid(BaseMaterial))
		{
			GasGiantMaterialInstance = UMaterialInstanceDynamic::Create(
				BaseMaterial, this, TEXT("MID_APS_GasGiantVisual"));
			GasGiantVisualComponent->SetMaterial(0, GasGiantMaterialInstance);
		}
	}
	if (IsValid(GasGiantMaterialInstance))
	{
		FLinearColor TypeTint(0.93f, 0.76f, 0.56f, 1.0f);
		switch (PlanetType)
		{
		case EPlanetType::HotGiant:
			TypeTint = FLinearColor(1.00f, 0.47f, 0.20f, 1.0f);
			break;
		case EPlanetType::IceGiant:
			TypeTint = FLinearColor(0.42f, 0.72f, 1.00f, 1.0f);
			break;
		default:
			break;
		}
		const uint32 StableHash = HashCombine(GetTypeHash(WorldScapeSeed),
			GetTypeHash(static_cast<uint8>(PlanetType)));
		const float SeedVariation = 0.92f
			+ static_cast<float>(StableHash % 1000u) / 1000.0f * 0.16f;
		TypeTint *= SeedVariation;
		TypeTint.A = 1.0f;
		GasGiantMaterialInstance->SetVectorParameterValue(
			TEXT("BaseColorFactor"), TypeTint);
		GasGiantMaterialInstance->SetVectorParameterValue(
			TEXT("BaseColorFactor_RGB"), TypeTint);
		GasGiantMaterialInstance->SetScalarParameterValue(
			TEXT("RoughnessFactor"), PlanetType == EPlanetType::IceGiant ? 0.36f : 0.42f);
	}
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
		const bool bGasVisual = SphereMesh->ComponentHasTag(TEXT("APS.GasGiantVisual"));
		const bool bShow = IsNotGasGiant() ? !bGasVisual : bGasVisual;
		SphereMesh->SetHiddenInGame(!bShow, false);
		SphereMesh->SetVisibility(bShow, false);
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
