#include "AstroGenerator.h"
#include <Kismet/GameplayStatics.h>
#include "PlanetarySurfaceGenerator.h"
#include "PlanetGenerator.h"
#include "APS_ALPHA/Actors/Astro/AstroAnchor.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/HomeSystemPosition.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Structs/GalaxyModel.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include <unordered_map>
#include <functional>
#include "DrawDebugHelpers.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Structs/PlanetAtmosphereModel.h"
#include "APS_ALPHA/Pawns/Characters/GravityCharacterPawn.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"

AAstroGenerator::AAstroGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	GenerationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GenerationRoot"));
	SetRootComponent(GenerationRoot);

	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(GenerationRoot);
	PreviewCamera->SetAbsolute(true, true, true);
	PreviewCamera->SetFieldOfView(55.0f);
}

void AAstroGenerator::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("=== AAstroGenerator::BeginPlay START ==="));
	UE_LOG(LogTemp, Warning, TEXT("bAutoGeneration: %s"), bAutoGeneration ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("bIntegrateStartPlanet: %s"), bIntegrateStartPlanet ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("StartPlanetNumber: %d"), StartPlanetNumber);

	if (bAutoGeneration)
	{
		UE_LOG(LogTemp, Warning, TEXT("bAutoGeneration is true, initializing generators"));
		InitAstroGenerators();

		InitGenerationLevel();

		if (bIntegrateStartPlanet && WSR_StartHomePlanet)
		{
			UE_LOG(LogTemp, Warning, TEXT("Both bAutoGeneration and bIntegrateStartPlanet are true - using special integration method"));
			GenerateStarSystemAndIntegratePlanet();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("bAutoGeneration is false, skipping generation"));
	}

	UE_LOG(LogTemp, Warning, TEXT("=== AAstroGenerator::BeginPlay END ==="));
}

void AAstroGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bPreviewCameraTransitionActive || !PreviewCamera)
	{
		SetActorTickEnabled(false);
		return;
	}

	PreviewCameraTransitionElapsed += FMath::Max(DeltaSeconds, 0.0f);
	const float Alpha = FMath::Clamp(
		PreviewCameraTransitionElapsed / FMath::Max(PreviewCameraTransitionDuration, 0.01f), 0.0f, 1.0f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	FTransform BlendedTransform;
	BlendedTransform.Blend(PreviewCameraStartTransform, PreviewCameraTargetTransform, SmoothAlpha);
	PreviewCamera->SetWorldTransform(BlendedTransform);

	if (Alpha >= 1.0f)
	{
		bPreviewCameraTransitionActive = false;
		SetActorTickEnabled(false);
	}
}

void AAstroGenerator::GenerateWorldByModel()
{
	InitAstroGenerators();

	ApplyWorldModel();

	ApplySpawnParameters();

	InitGenerationLevel();
}

bool AAstroGenerator::RegeneratePreview(UGeneratedWorld* InGeneratedWorld)
{
	if (!IsValid(InGeneratedWorld) || !GetWorld())
	{
		return false;
	}

	ClearGeneratedPreview();
	SetActorScale3D(FVector::OneVector);
	SetGeneratedWorld(InGeneratedWorld);

	InitAstroGenerators();
	ApplyWorldModel();

	const bool bSavedStarterLocation = bSpawnStarterLocation;
	const bool bSavedStarterPlanet = bSpawnStarterPlanet;
	const bool bSavedCharacterSpawn = bCharacterSpawn;
	bSpawnStarterLocation = false;
	bSpawnStarterPlanet = false;
	bCharacterSpawn = false;
	bIsPreviewGeneration = true;
	// Existing Blueprint CDOs can retain the old 10k default even after the C++
	// default changes. Enforce the UI-only cap at runtime; committed gameplay
	// generation never enters this path.
	PreviewMaxInstances = FMath::Clamp(PreviewMaxInstances, 100, 3000);

	InitGenerationLevel();

	bIsPreviewGeneration = false;
	bSpawnStarterLocation = bSavedStarterLocation;
	bSpawnStarterPlanet = bSavedStarterPlanet;
	bCharacterSpawn = bSavedCharacterSpawn;

	FocusPreviewCamera();
	return IsValid(GeneratedStarCluster) || IsValid(GeneratedGalaxy) || IsValid(GeneratedHomeStarSystem);
}

void AAstroGenerator::ClearGeneratedPreview()
{
	TArray<AActor*> GeneratedRoots;
	GeneratedRoots.AddUnique(GeneratedStarCluster);
	GeneratedRoots.AddUnique(GeneratedGalaxy);
	GeneratedRoots.AddUnique(GeneratedHomeStarSystem);
	GeneratedRoots.AddUnique(GeneratedWorld);

	for (AActor* GeneratedRoot : GeneratedRoots)
	{
		if (IsValid(GeneratedRoot) && GeneratedRoot != this)
		{
			DestroyActorTree(GeneratedRoot);
		}
	}

	GeneratedStarCluster = nullptr;
	GeneratedGalaxy = nullptr;
	GeneratedHomeStarSystem = nullptr;
	GeneratedWorld = nullptr;
	HomePlanetarySystem = nullptr;
	HomeStar = nullptr;
	HomePlanet = nullptr;
	StarIndexModelMap.Reset();
}

void AAstroGenerator::FocusPreviewCamera(APlayerController* PlayerController)
{
	FocusPreviewTarget(PreviewFocus, PlayerController);
}

FBox AAstroGenerator::GetPreviewFocusBounds(EAstroPreviewFocus Focus) const
{
	FBox Bounds(EForceInit::ForceInit);
	const auto AddVisibleActorTree = [&Bounds](const AActor* RootActor)
	{
		TArray<const AActor*> Pending;
		if (IsValid(RootActor)) Pending.Add(RootActor);
		while (Pending.Num() > 0)
		{
			const AActor* Actor = Pending.Pop(EAllowShrinking::No);
			TInlineComponentArray<UPrimitiveComponent*> Components;
			Actor->GetComponents(Components);
			for (const UPrimitiveComponent* Component : Components)
			{
				if (IsValid(Component) && Component->IsRegistered() && Component->IsVisible()
					&& !Component->bHiddenInGame && Component->Bounds.SphereRadius > UE_SMALL_NUMBER)
				{
					Bounds += Component->Bounds.GetBox();
				}
			}
			TArray<AActor*> Children;
			Actor->GetAttachedActors(Children, false, true);
			for (const AActor* Child : Children) if (IsValid(Child)) Pending.Add(Child);
		}
	};
	const AActor* FocusActor = nullptr;
	switch (Focus)
	{
	case EAstroPreviewFocus::StarCluster: FocusActor = GeneratedStarCluster; break;
	case EAstroPreviewFocus::Galaxy: FocusActor = GeneratedGalaxy; break;
	case EAstroPreviewFocus::HomeSystem: FocusActor = GeneratedHomeStarSystem; break;
	case EAstroPreviewFocus::HomeStar: FocusActor = HomeStar; break;
	case EAstroPreviewFocus::HomePlanet: FocusActor = HomePlanet; break;
	case EAstroPreviewFocus::Overview:
	default: break;
	}

	if (IsValid(FocusActor))
	{
		AddVisibleActorTree(FocusActor);
		return Bounds;
	}

	const AActor* PreviewRoots[] = {GeneratedStarCluster, GeneratedGalaxy, GeneratedHomeStarSystem, GeneratedWorld};
	for (const AActor* PreviewRoot : PreviewRoots)
	{
		if (IsValid(PreviewRoot))
		{
			AddVisibleActorTree(PreviewRoot);
		}
	}
	return Bounds;
}

void AAstroGenerator::StartPreviewCameraTransition(const FVector& Center, double Radius,
	APlayerController* PlayerController)
{
	if (!PreviewCamera || !GetWorld())
	{
		return;
	}

	const double HalfFovRadians = FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5);
	const double Distance = FMath::Max(Radius * 1.08 / FMath::Tan(HalfFovRadians), Radius * 1.25);
	const FVector ViewDirection = FVector(-1.0, -1.0, 0.45).GetSafeNormal();
	const FVector CameraLocation = Center - ViewDirection * Distance;

	PreviewOrbitCenter = Center;
	PreviewOrbitDistance = Distance;
	PreviewCameraStartTransform = PreviewCamera->GetComponentTransform();
	PreviewCameraTargetTransform = FTransform((Center - CameraLocation).Rotation(), CameraLocation);
	PreviewCameraTransitionElapsed = 0.0f;
	bPreviewCameraTransitionActive = true;
	SetActorTickEnabled(true);
	PreviewCamera->SetActive(true);

	APlayerController* ResolvedController = PlayerController ? PlayerController : GetWorld()->GetFirstPlayerController();
	if (ResolvedController && ResolvedController->GetViewTarget() != this)
	{
		ResolvedController->SetViewTargetWithBlend(this, 0.35f, VTBlend_Cubic);
	}
}

void AAstroGenerator::FocusPreviewTarget(EAstroPreviewFocus NewFocus, APlayerController* PlayerController)
{
	PreviewFocus = NewFocus;
	FBox Bounds = GetPreviewFocusBounds(NewFocus);
	if (!Bounds.IsValid)
	{
		Bounds = FBox(GetActorLocation() - FVector(500.0), GetActorLocation() + FVector(500.0));
	}
	StartPreviewCameraTransition(
		Bounds.GetCenter(), FMath::Max(Bounds.GetExtent().Size(), 500.0), PlayerController);
}

void AAstroGenerator::OrbitPreviewCamera(FVector2D ScreenDelta)
{
	if (!PreviewCamera || PreviewOrbitDistance <= UE_SMALL_NUMBER)
	{
		return;
	}

	bPreviewCameraTransitionActive = false;
	const FVector Offset = PreviewCamera->GetComponentLocation() - PreviewOrbitCenter;
	const FQuat Yaw(FVector::UpVector, FMath::DegreesToRadians(-ScreenDelta.X * 0.18));
	const FQuat Pitch(PreviewCamera->GetRightVector(), FMath::DegreesToRadians(ScreenDelta.Y * 0.14));
	const FVector NewOffset = (Pitch * Yaw).RotateVector(Offset).GetSafeNormal() * PreviewOrbitDistance;
	const FVector NewLocation = PreviewOrbitCenter + NewOffset;
	PreviewCamera->SetWorldLocationAndRotation(NewLocation, (PreviewOrbitCenter - NewLocation).Rotation());
}

void AAstroGenerator::ZoomPreviewCamera(float WheelDelta)
{
	if (!PreviewCamera || FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}

	bPreviewCameraTransitionActive = false;
	PreviewOrbitDistance = FMath::Clamp(
		PreviewOrbitDistance * FMath::Pow(0.82, static_cast<double>(WheelDelta)), 100.0, 1.0e18);
	const FVector ViewDirection = (PreviewCamera->GetComponentLocation() - PreviewOrbitCenter).GetSafeNormal();
	const FVector NewLocation = PreviewOrbitCenter + ViewDirection * PreviewOrbitDistance;
	PreviewCamera->SetWorldLocationAndRotation(NewLocation, (PreviewOrbitCenter - NewLocation).Rotation());
}

void AAstroGenerator::InitAstroGenerators()
{
	GalaxyGenerator = NewObject<UGalaxyGenerator>();
	StarClusterGenerator = NewObject<UStarClusterGenerator>();
	StarSystemGenerator = NewObject<UStarSystemGenerator>();
	PlanetarySystemGenerator = NewObject<UPlanetarySystemGenerator>();
	StarGenerator = NewObject<UStarGenerator>();
	PlanetGenerator = NewObject<UPlanetGenerator>();
	MoonGenerator = NewObject<UMoonGenerator>();

	if (GalaxyGenerator == nullptr
		|| StarClusterGenerator == nullptr
		|| StarSystemGenerator == nullptr
		|| PlanetarySystemGenerator == nullptr
		|| StarGenerator == nullptr
		|| PlanetGenerator == nullptr
		|| MoonGenerator == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("All generators OK!"));
	}
}

void AAstroGenerator::ApplySpawnParameters()
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			USpawnParameters* SpawnParams = GameInstance->GetSubsystem<UMainGameplayInstance>()->SpawnParameters;

			if (SpawnParams)
			{
				// ���������� ����������
				CharSpawnPlace = SpawnParams->CharacterSpawnPlace;
				HomeSpaceStationOrbitHeight = SpawnParams->HomeStationOrbitHeight;
				BP_CharacterClass = SpawnParams->BP_CharacterClass;
				BP_HomeSpaceStation = SpawnParams->BP_HomeSpaceStation;
				BP_HomeSpaceship = SpawnParams->BP_HomeSpaceship;
				BP_HomeSpaceShipyard = SpawnParams->BP_HomeSpaceShipyard;
				BP_HomeSpaceHeadquarters = SpawnParams->BP_HomeSpaceHeadquarters;

				UE_LOG(LogTemp, Log, TEXT("Spawn parameters applied"));
			}
		}
	}
}

void AAstroGenerator::ApplyWorldModel()
{
	if (!GeneratedWorldModel)
	{
		UE_LOG(LogTemp, Error, TEXT("NewGeneratedWorld is nullptr"));
		return;
	}

	// Copy values from NewGeneratedWorld to AAstroGenerator
	bGenerateFullScaledWorld = GeneratedWorldModel->bGenerateFullScaledWorld;
	bGenerateHomeSystem = GeneratedWorldModel->bGenerateHomeSystem;
	bStartWithHomePlanet = GeneratedWorldModel->bStartWithHomePlanet;
	bRandomHomeSystem = GeneratedWorldModel->bRandomHomeSystem;
	bRandomHomeSystemType = GeneratedWorldModel->bRandomHomeSystemType;
	bRandomHomeStar = GeneratedWorldModel->bRandomHomeStar;
	bRandomStartPlanetNumber = GeneratedWorldModel->bRandomStartPlanetNumber;
	AstroGenerationLevel = GeneratedWorldModel->AstroGenerationLevel;
	GalaxyType = GeneratedWorldModel->GalaxyType;
	GalaxyGlass = GeneratedWorldModel->GalaxyClass;
	StarClusterSize = GeneratedWorldModel->StarClusterSize;
	StarClusterType = GeneratedWorldModel->StarClusterType;
	StarClusterPopulation = GeneratedWorldModel->StarClusterPopulation;
	StarClusterComposition = GeneratedWorldModel->StarClusterComposition;
	HomeSystemStarType = GeneratedWorldModel->StarType;
	HomeStarStellarType = GeneratedWorldModel->StellarType;
	HomeStarSpectralClass = GeneratedWorldModel->SpectralClass;
	HomeSystemPlanetaryType = GeneratedWorldModel->PlanetarySystemType;
	HomeSystemOrbitDistributionType = GeneratedWorldModel->OrbitDistributionType;
	HomeSystemPosition = GeneratedWorldModel->HomeSystemPosition;
	GalaxySize = GeneratedWorldModel->GalaxySize;
	GalaxyStarCount = GeneratedWorldModel->GalaxyStarCount;
	PlanetsAmount = GeneratedWorldModel->PlanetsAmount;
	StartPlanetNumber = GeneratedWorldModel->StartPlanetIndex;
	PlanetsAmount = FMath::Max(1, PlanetsAmount);
	StartPlanetNumber = FMath::Clamp(StartPlanetNumber, 1, PlanetsAmount);
	GalaxyStarDensity = GeneratedWorldModel->GalaxyStarDensity;
	HomePlanetarySystem = GeneratedWorldModel->HomePlanetarySystem;
	HomePlanet = GeneratedWorldModel->HomePlanet;

	// The model-driven menu is explicit: every visible selector must deterministically
	// affect the live scene. Blueprint defaults for the old Random buttons otherwise
	// override cluster/galaxy selectors and make the Slate controls appear broken.
	bGenerateRandomCluster = false;
	bGenerateRandomGalaxy = false;
}

void AAstroGenerator::GenerateStarCluster()
{
	if (BP_StarClusterClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BP_StarClusterClass is not set!"));
		return;
	}

	const TSharedPtr<FStarClusterModel> StarClusterModel = MakeShared<FStarClusterModel>();
	if (bGenerateRandomCluster)
	{
		StarClusterGenerator->GetRandomStarClusterModel(StarClusterModel);
	}
	else
	{
		StarClusterModel->StarClusterSize = StarClusterSize;
		StarClusterModel->StarClusterType = StarClusterType;
		StarClusterModel->StarClusterPopulation = StarClusterPopulation;
		StarClusterModel->StarClusterComposition = StarClusterComposition;
	}
	const EStarClusterType ClusterType = StarClusterModel->StarClusterType;
	AStarCluster* NewStarCluster = GetWorld()->SpawnActor<AStarCluster>(BP_StarClusterClass);
	if (!NewStarCluster)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn star cluster."));
		return;
	}
	NewStarCluster->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

	// Calculate Cluster Params
	NewStarCluster->StarAmount = StarClusterGenerator->GetStarsAmountByRange(StarClusterModel->StarClusterSize);
	if (bIsPreviewGeneration)
	{
		NewStarCluster->StarAmount = FMath::Min(NewStarCluster->StarAmount, PreviewMaxInstances);
	}
	NewStarCluster->StarDensity = StarClusterGenerator->GetStarClusterDensityByRange();
	NewStarCluster->ClusterBounds = StarClusterGenerator->GetStarClusterBoundsByRange(ClusterType);
	NewStarCluster->ClusterType = ClusterType;
	NewStarCluster->StarClusterComposition = StarClusterModel->StarClusterComposition;
	NewStarCluster->StarClusterPopulation = StarClusterModel->StarClusterPopulation;
	NewStarCluster->StarClusterSize = StarClusterModel->StarClusterSize;
	NewStarCluster->StarMeshInstances->NumCustomDataFloats = 6;
	if (NewStarCluster->GenerationSeed == 0)
	{
		NewStarCluster->GenerationSeed = FMath::RandRange(1, MAX_int32);
	}
	NewStarCluster->CalculateAffectionRadius();
	NewStarCluster->PotentialStarSystems.Reserve(NewStarCluster->StarAmount);
	NewStarCluster->StarMeshInstances->PreAllocateInstancesMemory(NewStarCluster->StarAmount);

	UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), NewStarCluster->StarAmount);
	UE_LOG(LogTemp, Warning, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
	UE_LOG(LogTemp, Warning, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
	UE_LOG(LogTemp, Warning, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

	for (int32 i = 0; i < NewStarCluster->StarAmount; ++i)
	{
		// Create a star model
		TSharedPtr<FStarModel> NewStarModel = MakeShared<FStarModel>();

		if (bGenerateRandomCluster)
		{
			StarGenerator->GenerateRandomStarModel(NewStarModel);
		}
		else
		{
			StarGenerator->GenerateStarModelByProbability(NewStarModel, StarClusterModel);
		}

		// Position the star in the cluster
		FVector StarPosition = StarClusterGenerator->CalculateStarPosition(i, NewStarCluster, NewStarModel);
		NewStarModel->Location = StarPosition;

		// Create a star instance and add it to the HISM component
		FTransform StarTransform(StarPosition);
		const double FarVisualRadius = UStarGenerator::GetFarStarVisualRadius(NewStarModel->Radius);
		StarTransform.SetScale3D(FVector(FarVisualRadius));
		const int32 StarInstIndex = NewStarCluster->StarMeshInstances->AddInstance(StarTransform, true);
		const FLinearColor ColorValue = StarGenerator->GetStarColor(NewStarModel->SpectralClass,
		                                                            NewStarModel->SpectralSubclass);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R, false);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G, false);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B, false);

		const double StarEmission = UStarGenerator::GetFarStarVisualEmission(NewStarModel->Radius,
			StarGenerator->CalculateEmission(NewStarModel->Luminosity * 25));
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission, false);

		FStarSystemModel PotentialSystemModel;
		const int32 SystemSeed = static_cast<int32>(HashCombine(
			GetTypeHash(NewStarCluster->GenerationSeed), GetTypeHash(StarInstIndex)) & 0x7fffffffu);
		StarSystemGenerator->GeneratePotentialStarSystemModel(
			PotentialSystemModel, *NewStarModel, FMath::Max(SystemSeed, 1));

		FTransform LocalInstanceTransform;
		NewStarCluster->StarMeshInstances->GetInstanceTransform(
			StarInstIndex, LocalInstanceTransform, false);
		NewStarCluster->RegisterPotentialSystem(StarInstIndex, LocalInstanceTransform.GetLocation(),
			*NewStarModel, PotentialSystemModel);

		// Stable per-star surface seed and system occupancy are ready for the unlit HISM material.
		FRandomStream VisualStream(PotentialSystemModel.GenerationSeed);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(
			StarInstIndex, 4, VisualStream.FRand(), false);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(
			StarInstIndex, 5, PotentialSystemModel.PotentialPlanetCount / 12.0f, false);

	}
	NewStarCluster->FinalizeGeneratedInstances();

	GeneratedStarCluster = NewStarCluster;

	if (bGenerateFullScaledWorld)
	{
		SetActorScale3D(FVector(FullScaleValue, FullScaleValue, FullScaleValue));
	}
}

AStarSystem* AAstroGenerator::MaterializeClusterStarSystem(int32 InstanceIndex)
{
	if (!GeneratedStarCluster || !GeneratedStarCluster->StarMeshInstances || !StarGenerator
		|| !StarSystemGenerator || !BP_StarSystemClass || !BP_StarClass)
	{
		return nullptr;
	}

	FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystemMutable(InstanceIndex);
	if (!Record)
	{
		return nullptr;
	}
	if (Record->MaterializedSystem.IsValid())
	{
		return Record->MaterializedSystem.Get();
	}

	FTransform WorldTransform;
	if (!GeneratedStarCluster->StarMeshInstances->GetInstanceTransform(InstanceIndex, WorldTransform, true))
	{
		return nullptr;
	}
	WorldTransform.SetScale3D(FVector::OneVector);

	UWorld* World = GetWorld();
	AStarSystem* StarSystem = World
		? World->SpawnActor<AStarSystem>(BP_StarSystemClass, WorldTransform) : nullptr;
	if (!StarSystem)
	{
		return nullptr;
	}
	StarSystem->AttachToActor(GeneratedStarCluster, FAttachmentTransformRules::KeepWorldTransform);
	StarSystemGenerator->ApplyModel(StarSystem, MakeShared<FStarSystemModel>(Record->SystemModel));

	AStar* Star = World->SpawnActor<AStar>(BP_StarClass, WorldTransform);
	if (!Star)
	{
		StarSystem->Destroy();
		return nullptr;
	}
	const TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>(Record->PrimaryStarModel);
	StarGenerator->ApplyModel(Star, StarModel);
	Star->SetActorLocation(WorldTransform.GetLocation());
	Star->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
	Star->StarRadiusKM = StarModel->Radius * 696340;
	Star->FullSpectralName = Star->GenerateFullSpectralName();
	Star->AstroName = FName(*FString::Printf(TEXT("STAR-%s"),
		*Record->StableId.ToString(EGuidFormats::Short)));
	StarGenerator->ApplySpectralMaterial(Star, StarModel);
	StarSystem->MainStar = Star;
	StarSystem->AddNewStar(Star);
	Star->AttachToActor(StarSystem, FAttachmentTransformRules::KeepWorldTransform);

	FTransform HiddenTransform;
	if (GeneratedStarCluster->StarMeshInstances->GetInstanceTransform(
		InstanceIndex, HiddenTransform, false))
	{
		HiddenTransform.SetScale3D(FVector::ZeroVector);
		GeneratedStarCluster->StarMeshInstances->UpdateInstanceTransform(
			InstanceIndex, HiddenTransform, false, true, true);
		GeneratedStarCluster->StarMeshInstances->BuildTreeIfOutdated(true, true);
	}
	Record->bMaterialized = true;
	Record->MaterializedSystem = StarSystem;

	UE_LOG(LogTemp, Log, TEXT("[APS.Cluster] Materialized system %s at instance %d (%d potential planets)"),
		*Record->StableId.ToString(EGuidFormats::DigitsWithHyphensLower), InstanceIndex,
		Record->SystemModel.PotentialPlanetCount);
	return StarSystem;
}

bool AAstroGenerator::DematerializeClusterStarSystem(int32 InstanceIndex)
{
	if (!GeneratedStarCluster || !GeneratedStarCluster->StarMeshInstances)
	{
		return false;
	}
	FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystemMutable(InstanceIndex);
	if (!Record)
	{
		return false;
	}
	if (Record->MaterializedSystem.IsValid())
	{
		DestroyActorTree(Record->MaterializedSystem.Get());
	}

	const FTransform RestoredTransform(
		FQuat::Identity, Record->ClusterLocalLocation, FVector(Record->PrimaryStarModel.Radius));
	GeneratedStarCluster->StarMeshInstances->UpdateInstanceTransform(
		InstanceIndex, RestoredTransform, false, true, true);
	GeneratedStarCluster->StarMeshInstances->BuildTreeIfOutdated(true, true);
	Record->bMaterialized = false;
	Record->MaterializedSystem.Reset();
	return true;
}

void AAstroGenerator::AddGeneratedWorldModelData()
{
	if (!HomePlanet)
	{
		UE_LOG(LogTemp, Error, TEXT("HomePlanet is null!"));
		return;
	}

	// �������� ���������� ������
	if (!HomePlanet->PlanetData.PlanetModel.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HomePlanet's PlanetModel is invalid!"));
		return;
	}
	
	GeneratedWorldModel->StarsAmount = GeneratedStarCluster->StarAmount;
	GeneratedWorldModel->HomeStarName = HomeStar->AstroName;
	GeneratedWorldModel->FullSpectralName = HomeStar->FullSpectralName;
	GeneratedWorldModel->HomeStarMass = HomeStar->Mass;
	GeneratedWorldModel->HomeStarRadius = HomeStar->Radius;
	GeneratedWorldModel->HomeStarTemperature = HomeStar->SurfaceTemperature;
	GeneratedWorldModel->HomePlanetName = HomePlanet->AstroName;
	GeneratedWorldModel->StarSystemRadius = GeneratedHomeStarSystem->StarSystemRadius;

	HomePlanet->FillPlanetData();
	GeneratedWorldModel->InhabitedPlanets.Add(HomePlanet->PlanetData);
}

void AAstroGenerator::GenerateHomeStarSystem()
{
	/*
	 * Create UGeneratedWorld* GeneratedWorldModel
	 * Model with random params for random generation
	 * Model from GenerationMenu for custom generation
	 */

	if (bAutoGeneration)
	{
		SetAutoStarSystemModel();
	}
	else if (bRandomHomeSystem)
	{
		GenerateRandomStarSystemModel();
	}

	GenerateStarSystemByModel();

	if (GeneratedHomeStarSystem
		&& GeneratedHomeStarSystem->MainStar
		&& GeneratedHomeStarSystem->MainStar->PlanetarySystem)
	{
		TArray<TSharedPtr<FPlanetData>> PlanetDataMap = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetsList;
		UE_LOG(LogTemp, Warning, TEXT("Planet List - "));
		UE_LOG(LogTemp, Warning, TEXT("Planet Amount: %d"), PlanetDataMap.Num());
		ShowPlanetsList(PlanetDataMap);

		if (bSpawnStarterPlanet && GeneratedHomeStarSystem)
		{
			// Check if we should integrate StartPlanet instead of generating new one
			if (bIntegrateStartPlanet && WSR_StartHomePlanet)
			{
				UE_LOG(LogTemp, Warning, TEXT("Integrating StartPlanet into generated star system"));
				IntegrateStartPlanetIntoSystem();
			}
			else
			{
				// Original planet generation logic
				const TSharedPtr<FPlanetModel> HomePlanetModel = PlanetGenerator->CreatePlanetModelFromGeneratedWorld(
					GeneratedWorldModel);
				const TSharedPtr<FPlanetAtmosphereModel> PlanetAtmosphereModel = PlanetGenerator->
					CreateAtmosphereModelFromGeneratedWorld(GeneratedWorldModel);
				HomePlanet = PlanetGenerator->GeneratePlanet(HomePlanetModel, BP_PlanetClass, GetWorld());
				
				// Validate HomePlanet was created successfully
				if (!HomePlanet)
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to generate HomePlanet!"));
					return;
				}
				
				PlanetGenerator->GeneratePlanetAtmosphere(HomePlanet, PlanetAtmosphereModel);
				// Detailed terrain is created lazily by the WorldScape streaming
				// subsystem. Generating it here left an unconditional root at startup.
				
				HomePlanet->AstroName = AGravityPlayerController::GenerateUniqueName("Planet");
			}

			APlanetOrbit* NewHomePlanetOrbit = GeneratedHomeStarSystem->MainStar->PlanetarySystem->
			                                                            PlanetOrbitsList[StartPlanetNumber - 1];
			if (NewHomePlanetOrbit && NewHomePlanetOrbit->Planet)
			{
				NewHomePlanetOrbit->TriggerClearChildren();

				// Attach a planet from the editor to the orbit actor
				if (HomePlanet)
				{
					HomePlanet->AttachToActor(NewHomePlanetOrbit,
					                          FAttachmentTransformRules::KeepRelativeTransform);

					// Get the local position of the current planet and set the local position for your planet based on the stored offset
					const FVector OldPlanetLocalPosition = NewHomePlanetOrbit->Planet->GetActorLocation() -
						NewHomePlanetOrbit->GetActorLocation();
					HomePlanet->SetActorLocation(
						NewHomePlanetOrbit->GetActorLocation() + OldPlanetLocalPosition);

					// Create HomePlanetModel for operations
					const TSharedPtr<FPlanetModel> HomePlanetModel = PlanetGenerator->CreatePlanetModelFromGeneratedWorld(GeneratedWorldModel);
					HomePlanetModel->RadiusKM = HomePlanetModel->Radius;
					PlanetarySystemGenerator->GeneratePlanetMoonsList(PlanetGenerator, MoonGenerator,
					                                                  HomePlanetModel, HomePlanetModel->Radius,
					                                                  GeneratedWorldModel->MoonsAmount);

					AddGeneratedWorldModelData();
					
					SpawnPlanetMoons(HomePlanetModel);

					SpawnStartInteractiveActors(HomePlanetModel);

					if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
					{
						if (AGravityPlayerController* MainController = Cast<AGravityPlayerController>(PC))
						{
							MainController->
								SaveNewWorld(GeneratedWorldModel->AstroGenerationLevel, GeneratedWorldModel);
						}
					}
				}
			}
		}
	}
}

void AAstroGenerator::GenerateStarSystemByModel()
{
	UE_LOG(LogTemp, Warning, TEXT("=== GenerateStarSystemByModel START ==="));
	
	if (CheckGeneratorsFails()) 
	{
		UE_LOG(LogTemp, Error, TEXT("CheckGeneratorsFails() returned true - generators failed!"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("Generate Star System!"));

		FTransform HomeSystemTransform;
		FVector HomeSystemSpawnLocation;
		ComputeHomeSystemPosition(HomeSystemTransform, HomeSystemSpawnLocation);

		/*RandomPosition:
		get random index from hism array indexes
		get index model
		from hism index to star model map get random pair*/

		// Create a new star system
		TSharedPtr<FStarSystemModel> StarSystemModel;
		int AmountOfStars;
		ComputeStarAmount(StarSystemModel, AmountOfStars);

		FClusterStarSystemRecord* HomeClusterRecord = PendingHomeCluster.IsValid()
			? PendingHomeCluster->FindPotentialSystemMutable(PendingHomeClusterInstanceIndex)
			: nullptr;
		if (HomeClusterRecord)
		{
			StarSystemModel = MakeShared<FStarSystemModel>(HomeClusterRecord->SystemModel);
			StarSystemModel->Location = HomeSystemSpawnLocation;
			AmountOfStars = FMath::Max(1, StarSystemModel->AmountOfStars);
		}

		AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass, HomeSystemTransform);
		if (!NewStarSystem)
		{
			UE_LOG(LogTemp, Error, TEXT("NewStarSystem failed!"));
			return;
		}
		NewStarSystem->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		NewStarSystem->SetActorLocation(HomeSystemSpawnLocation);
		StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

		FVector LastStarLocation{0};
		for (int StarNumber = 0; StarNumber < AmountOfStars; StarNumber++)
		{
			TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();

			if (HomeClusterRecord && StarNumber == 0 && bRandomHomeStar)
			{
				*StarModel = HomeClusterRecord->PrimaryStarModel;
				StarModel->Location = HomeSystemSpawnLocation;
			}
			else if (bRandomHomeStar)
			{
				StarGenerator->GenerateRandomStarModel(StarModel);
			}
			else
			{
				StarModel->StellarType = HomeStarStellarType;
				StarModel->SpectralClass = HomeStarSpectralClass;
				StarGenerator->GenerateStarModel(StarModel);
			}

			PlanetarySystemModel = MakeShared<FPlanetarySystemModel>();

			if (bRandomHomeSystemType)
			{
				PlanetarySystemGenerator->GeneratePlanetarySystemModelByStar(
					PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);
			}
			else
			{
				PlanetarySystemModel->AmountOfPlanets = PlanetsAmount;
				PlanetarySystemModel->PlanetarySystemType = HomeSystemPlanetaryType;
				PlanetarySystemModel->OrbitDistributionType = HomeSystemOrbitDistributionType;
				PlanetarySystemGenerator->GenerateCustomPlanetarySystemModel(
					PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);

				// The menu preview must be driven by the user's model. The legacy
				// generator only consumed PlanetType/Radius/Moons in the separate
				// starter-planet path, which is deliberately disabled for previews.
				// Replace the selected home-planet model in-place so changing a Slate
				// control has an immediate, visible result without spawning gameplay
				// infrastructure or WorldScape terrain in the menu level.
				if (bIsPreviewGeneration && GeneratedWorldModel && PlanetarySystemModel->PlanetsList.Num() > 0)
				{
					const int32 PreviewHomeIndex = FMath::Clamp(
						StartPlanetNumber - 1, 0, PlanetarySystemModel->PlanetsList.Num() - 1);
					TSharedPtr<FPlanetData>& HomeData = PlanetarySystemModel->PlanetsList[PreviewHomeIndex];
					if (!HomeData.IsValid())
					{
						HomeData = MakeShared<FPlanetData>();
					}
					const double ExistingOrbitRadius = HomeData->OrbitRadius;
					HomeData->PlanetModel = PlanetGenerator->CreatePlanetModelFromGeneratedWorld(GeneratedWorldModel);
					HomeData->PlanetOrder = PreviewHomeIndex + 1;
					HomeData->OrbitRadius = ExistingOrbitRadius;
					if (HomeData->PlanetModel.IsValid())
					{
						HomeData->PlanetModel->OrbitDistance = ExistingOrbitRadius;
						PlanetarySystemGenerator->GeneratePlanetMoonsList(
							PlanetGenerator, MoonGenerator, HomeData->PlanetModel,
							HomeData->PlanetModel->Radius, GeneratedWorldModel->MoonsAmount);
					}
				}
			}

			AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass);
			APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);

			if (StarNumber == 0)
			{
				NewStarSystem->MainStar = NewStar;
			}

			if (!NewStar || !NewPlanetarySystem)
			{
				UE_LOG(LogTemp, Warning, TEXT("Star Falied!"));
				return;
			}

			// Set Star full-scale
			// TODO: PlanetarySystemGenerator->ConnectStar()
			StarGenerator->ApplyModel(NewStar, StarModel);
			PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetarySystemModel);
			const FVector SystemCenter = NewStarSystem->GetActorLocation();
			NewStar->SetActorLocation(SystemCenter);
			NewPlanetarySystem->SetActorLocation(SystemCenter);
			NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
			NewStar->StarRadiusKM = StarModel->Radius * 696340;
			NewStar->SetPlanetarySystem(NewPlanetarySystem);
			NewPlanetarySystem->SetStar(NewStar);
			NewStarSystem->AddNewStar(NewStar);
			NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
			StarGenerator->ApplySpectralMaterial(NewStar, StarModel);
			NewStar->FullSpectralName = NewStar->GenerateFullSpectralName();
			NewPlanetarySystem->SetStarFullSpectralName(NewStar->FullSpectralName);
			const FString SpectralIdentity = NewStar->FullSpectralName.IsNone()
				? TEXT("Star") : NewStar->FullSpectralName.ToString();
			NewStar->AstroName = AGravityPlayerController::GenerateUniqueName(SpectralIdentity);
			if (StarNumber == 0)
			{
				HomeStar = NewStar;
			}

			// Generate planets for each star
			FVector LastPlanetLocation{0};
			int32 PlanetIndex = 0;
			for (const TSharedPtr<FPlanetData> FPlanetData : PlanetarySystemModel->PlanetsList)
			{
				APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(
					BP_PlanetOrbitClass, NewPlanetarySystem->GetActorLocation(), FRotator::ZeroRotator);
				NewPlanetOrbit->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanetarySystem->PlanetOrbitsList.Add(NewPlanetOrbit);

				// Planet Model and generation
				TSharedPtr<FPlanetModel> PlanetModel = FPlanetData->PlanetModel;
				APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);

				PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
				NewPlanet->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Planet"));
				NewStar->AddPlanet(NewPlanet);
				NewPlanet->SetParentStar(NewStar);

				// Set planet full-scale
				NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
				const FVector OrbitOffset(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
				const FVector NewLocation = NewPlanetOrbit->GetActorLocation()
					+ NewPlanetOrbit->GetActorQuat().RotateVector(OrbitOffset);
				NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
				NewPlanet->SetActorLocation(NewLocation);
				NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanetarySystem->PlanetsActorsList.Add(NewPlanet);
				NewPlanetOrbit->Planet = NewPlanet;
				if (StarNumber == 0 && PlanetIndex == FMath::Clamp(
					StartPlanetNumber - 1, 0, PlanetarySystemModel->PlanetsList.Num() - 1))
				{
					HomePlanet = NewPlanet;
				}

				// Generate Moons
				double DiameterOfLastMoon = 0;
				FVector LastMoonLocation;
				for (const TSharedPtr<FMoonData> MoonData : PlanetModel->MoonsList)
				{
					const double KM_TO_UE_UNIT_SCALE = 100000;
					APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(
						BP_PlanetOrbitClass, NewPlanet->GetActorLocation(), FRotator::ZeroRotator);
					NewMoonOrbit->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);
					NewPlanet->MoonOrbitsList.Add(NewMoonOrbit);

					FVector MoonLocation = NewPlanet->GetActorLocation();
					AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
					NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
					NewPlanet->AddMoon(NewMoon);
					NewMoon->SetParentPlanet(NewPlanet);

					MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
					MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);

					// Set moon full-scale
					double MoonRadius = MoonData->MoonModel->Radius;
					NewMoon->RadiusKM = MoonRadius * 6371;
					NewMoon->SetActorScale3D(FVector(MoonRadius * 12742000));

					FVector Origin, BoxExtent;
					double SphereRadius;
					NewMoon->GetActorBounds(false, Origin, BoxExtent);
					SphereRadius = BoxExtent.GetMax();
					NewMoon->AffectionRadiusKM = SphereRadius / 100000.0;

					NewMoon->AddActorLocalOffset(FVector(
						0, ((PlanetModel->RadiusKM + (MoonData->OrbitRadius * PlanetModel->RadiusKM)) *
							KM_TO_UE_UNIT_SCALE) * 1, 0));
					NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);

					DiameterOfLastMoon = MoonRadius * 2;
					LastMoonLocation = NewMoon->GetActorLocation();

					NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(World, NewMoon->RadiusKM, NewMoon);
				}

				if (DiameterOfLastMoon == 0)
				{
					NewPlanet->PlanetaryZone->SetSphereRadius(100);
					NewPlanet->AffectionRadiusKM = 100 * NewPlanet->GetActorScale3D().X / 100000;
				}
				else
				{
					FVector PlanetLocation = NewPlanet->GetActorLocation();
					FVector LastMoonOuterEdgeLocation = LastMoonLocation + FVector(0, DiameterOfLastMoon * 6371, 0);
					double SphereRadius = FVector::Dist(PlanetLocation, LastMoonOuterEdgeLocation);
					SphereRadius /= NewPlanet->GetActorScale3D().X;
					SphereRadius *= 1.5;
					NewPlanet->PlanetaryZone->SetSphereRadius(SphereRadius);
					NewPlanet->AffectionRadiusKM = SphereRadius * NewPlanet->GetActorScale3D().X / 100000;
				}
				LastPlanetLocation = NewPlanet->GetActorLocation();
				LastStarLocation = LastPlanetLocation * 1.1;

				NewPlanet->OrbitHeight = (NewPlanet->GravityCollisionZone->GetScaledSphereRadius() / 100000) -
					NewPlanet->RadiusKM;

				NewPlanet->PlanetaryEnvironmentGenerator->InitEnviroment(NewPlanet, World);
				++PlanetIndex;
			}

			// Place Orbits
			if (bOrbitRotationCheck)
			{
				UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem->PlanetsActorsList: %d"),
				       NewPlanetarySystem->PlanetsActorsList.Num());
				for (int i = 1; i < NewPlanetarySystem->PlanetsActorsList.Num(); i++)
				{
					APlanet* CurrentPlanet = NewPlanetarySystem->PlanetsActorsList[i];
					APlanet* PreviousPlanet = NewPlanetarySystem->PlanetsActorsList[i - 1];

					double CurrentPlanetLocation = CurrentPlanet->GetActorLocation().X;
					double PreviousPlanetLocation = PreviousPlanet->GetActorLocation().X;
					double SumOfAffectionZones = (CurrentPlanet->AffectionRadiusKM + PreviousPlanet->AffectionRadiusKM)
						* 1000000; // Converting to the same unit as locations
					double DistanceBetweenPlanets = CurrentPlanetLocation - PreviousPlanetLocation;

					if (DistanceBetweenPlanets <= SumOfAffectionZones)
					{
						double OrbitCoeff = CurrentPlanet->AffectionRadiusKM * 100000;
						OrbitCoeff *= 1.25;
						switch (NewPlanetarySystem->OrbitDistributionType)
						{
						case EOrbitDistributionType::Uniform:
							{
								OrbitCoeff = NewStar->RadiusKM / 2 * 100000 * (CurrentPlanet->Radius);
								break;
							}
						case EOrbitDistributionType::Chaotic:
							{
								OrbitCoeff = OrbitCoeff * FMath::RandRange(1, 10) * 2;
								break;
							}
						default:
							break;
						}

						// Shift it by the AffectionRadiusKM value of the current planet
						double NewLocationX = (PreviousPlanetLocation + PreviousPlanet->AffectionRadiusKM * 100000) + ((
							CurrentPlanet->AffectionRadiusKM * 100000) + OrbitCoeff);

						FVector NewLocation = CurrentPlanet->GetActorLocation();
						NewLocation.X = NewLocationX;
						CurrentPlanet->SetActorLocation(NewLocation);
						LastPlanetLocation = NewLocation;

						FVector OldLocation = CurrentPlanet->GetActorLocation();
						double OldLocationX = OldLocation.X;
						CurrentPlanet->SetActorLocation(NewLocation);

						UE_LOG(LogTemp, Warning,
						       TEXT("Planet %d: Moved from %f to %f, Distance: %f, SumOfAffectionZones: %f"), i,
						       OldLocationX, NewLocationX, DistanceBetweenPlanets, SumOfAffectionZones);
					}

					// Check Moons Orbits
					UE_LOG(LogTemp, Warning, TEXT("CurrentPlanet->Moons.Num(): %d"), CurrentPlanet->Moons.Num())
					for (int j = 1; j < CurrentPlanet->Moons.Num(); j++)
					{
						AMoon* CurrentMoon = CurrentPlanet->Moons[j];
						AMoon* PreviousMoon = CurrentPlanet->Moons[j - 1];

						double CurrentMoonLocation = CurrentMoon->GetActorLocation().Y;
						double PreviousMoonLocation = PreviousMoon->GetActorLocation().Y;
						double MoonSumOfAffectionZones = (CurrentMoon->AffectionRadiusKM + PreviousMoon->
							AffectionRadiusKM) * 100000; // Converting to the same unit as locations
						double DistanceBetweenMoons = CurrentMoonLocation - PreviousMoonLocation;

						if (DistanceBetweenMoons <= MoonSumOfAffectionZones)
						{
							double OrbitCoeff = CurrentMoon->AffectionRadiusKM * 100000;
							OrbitCoeff *= 1.1;
							double NewLocationY = (PreviousMoonLocation + PreviousMoon->AffectionRadiusKM * 100000)
								+ ((CurrentMoon->AffectionRadiusKM * 100000) + OrbitCoeff);
							CurrentMoon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
							FVector NewLocation = CurrentMoon->GetActorLocation();
							NewLocation.Y = NewLocationY;
							CurrentMoon->SetActorLocation(NewLocation);
							CurrentMoon->
								AttachToActor(CurrentPlanet, FAttachmentTransformRules::KeepWorldTransform);
							UE_LOG(LogTemp, Warning,
							       TEXT("Moon %d: Moved from %f to %f, Distance: %f, SumOfAffectionZones: %f"), j,
							       PreviousMoonLocation, CurrentMoonLocation, DistanceBetweenMoons,
							       MoonSumOfAffectionZones);
						}
					}
				}
			}

			if (bNeedOrbitRotation)
			{
				RotatePlanetOrbits(NewPlanetarySystem);
			}

			double StarSphereRadius;
			if (LastPlanetLocation.IsZero())
			{
				StarSphereRadius = NewStar->GetRadius() * NewStar->GetActorScale3D().X * 2;
				StarSphereRadius /= 1000000;
				NewStar->PlanetarySystemZone->SetSphereRadius(StarSphereRadius);
				NewStar->StarAffectionZoneRadius = StarSphereRadius * 1.5;
			}
			else
			{
				StarSphereRadius = FVector::Dist(NewStar->GetActorLocation(), LastPlanetLocation);
				StarSphereRadius /= NewStar->GetActorScale3D().X;
				NewStar->PlanetarySystemZone->SetSphereRadius(StarSphereRadius * 1.5);
				NewStar->StarAffectionZoneRadius = StarSphereRadius;
			}
			NewStar->CalculateAffectionRadius();
			NewStarSystem->AddNewStar(NewStar);
			NewStarSystem->StarSystemRadius = NewStar->StarAffectionZoneRadius;
		}

		double StarSystemSphereRadius = FVector::Dist(NewStarSystem->GetActorLocation(), LastStarLocation);
		StarSystemSphereRadius /= NewStarSystem->GetActorScale3D().X;
		StarSystemSphereRadius *= 1.6;
		NewStarSystem->StarSystemZone->SetSphereRadius(StarSystemSphereRadius);
		NewStarSystem->StarSystemRadius = StarSystemSphereRadius;
		GeneratedHomeStarSystem = NewStarSystem;
		UE_LOG(LogTemp, Warning, TEXT("GeneratedHomeStarSystem set to: %s"), *GeneratedHomeStarSystem->GetName());
		NewStarSystem->CalculateAffectionRadius();

		if (HomeClusterRecord && PendingHomeCluster.IsValid())
		{
			HomeClusterRecord->bMaterialized = true;
			HomeClusterRecord->MaterializedSystem = NewStarSystem;
			if (UHierarchicalInstancedStaticMeshComponent* Hism = PendingHomeCluster->StarMeshInstances)
			{
				FTransform HiddenTransform;
				if (Hism->GetInstanceTransform(PendingHomeClusterInstanceIndex, HiddenTransform, false))
				{
					HiddenTransform.SetScale3D(FVector::ZeroVector);
					Hism->UpdateInstanceTransform(
						PendingHomeClusterInstanceIndex, HiddenTransform, false, true, true);
					Hism->BuildTreeIfOutdated(true, true);
				}
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Cluster] Home system %s materialized from HISM instance %d"),
				*HomeClusterRecord->StableId.ToString(EGuidFormats::DigitsWithHyphensLower),
				PendingHomeClusterInstanceIndex);
		}

		if (NewStarSystem->StarSystemRadius == 0)
		{
			if (NewStarSystem->MainStar && NewStarSystem->MainStar->PlanetarySystemZone)
			{
				double SphereRadius = NewStarSystem->MainStar->PlanetarySystemZone->GetScaledSphereRadius() * 1.25;
				NewStarSystem->StarSystemRadius = SphereRadius;
				NewStarSystem->StarSystemZone->SetSphereRadius(SphereRadius);
			}
			else
			{
				if (!NewStarSystem->MainStar)
				{
					UE_LOG(LogTemp, Error, TEXT("NewStarSystem->MainStar is null"));
				}
				else if (!NewStarSystem->MainStar->PlanetarySystemZone)
				{
					UE_LOG(LogTemp, Error, TEXT("NewStarSystem->MainStar->PlanetarySystemZone is null"));
				}
			}
		}

		if (bGenerateFullScaledWorld)
		{
			/*
			 	method 1: non-adaptive
				if center = spawn at 000
				if random = spawn at random index
				if zones = spawn in distance from center
				check overlapped hism, if overlapped, (re)move it
			*/

			double HomeSystemRadiusScaled = NewStarSystem->StarSystemRadius * StarSystemDeadZone;
			FCollisionShape MySphere = FCollisionShape::MakeSphere(HomeSystemRadiusScaled);
			TArray<FOverlapResult> Overlaps;

			// An overlap check
			FVector HomeSystemLocation = NewStarSystem->GetActorLocation();
			bool bIsOverlap = World->OverlapMultiByChannel(Overlaps, HomeSystemLocation, FQuat::Identity,
			                                               ECC_Visibility,
			                                               MySphere);

			if (bIsOverlap)
			{
				TArray<int32> InstancesToRemove;
				UHierarchicalInstancedStaticMeshComponent* OverlappingHism = nullptr;
				int TotalOverlaps = Overlaps.Num();
				UE_LOG(LogTemp, Warning, TEXT("Total number of overlaps: %d"), TotalOverlaps);

				for (auto& Result : Overlaps)
				{
					if (UHierarchicalInstancedStaticMeshComponent* Hism = Cast<
						UHierarchicalInstancedStaticMeshComponent>(
						Result.Component.Get()))
					{
						OverlappingHism = Hism;
						int32 InstanceIndex = Result.ItemIndex;
						InstancesToRemove.Add(InstanceIndex);
					}
				}
				// Sort the array of indices in reverse order
				InstancesToRemove.Sort([](const int32& A, const int32& B) { return A > B; });
				for (auto Index : InstancesToRemove)
				{
					OverlappingHism->RemoveInstance(Index);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No overlaped stars!"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Falied to get World!"));
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== GenerateStarSystemByModel END ==="));
}

void AAstroGenerator::SetGeneratedWorld(UGeneratedWorld* InGeneratedWorld)
{
	this->GeneratedWorldModel = InGeneratedWorld;
}

void AAstroGenerator::DisplayNewGeneratedWorld()
{
	this->GeneratedWorldModel->PrintAllValues();
}

void AAstroGenerator::InitGenerationLevel()
{
	bool bGeneratedHomeSystemAsPrimaryLevel = false;

	switch (AstroGenerationLevel)
	{
	case EAstroGenerationLevel::GalaxiesCluster:
		GenerateGalaxiesCluster();
		break;
	case EAstroGenerationLevel::Galaxy:
		GenerateGalaxy();
		break;
	case EAstroGenerationLevel::StarCluster:
		GenerateStarCluster();
		break;
	case EAstroGenerationLevel::StarSystem:
		GenerateHomeStarSystem();
		bGeneratedHomeSystemAsPrimaryLevel = true;
		break;
	case EAstroGenerationLevel::PlanetSystem:
		GeneratePlanetSystem();
		bGeneratedHomeSystemAsPrimaryLevel = true;
		break;
	case EAstroGenerationLevel::SinglePlanet:
		GenerateSinglePlanet();
		bGeneratedHomeSystemAsPrimaryLevel = true;
		break;
	default:
		GenerateRandomWorld();
		break;
	}

	if (bGenerateHomeSystem && !bGeneratedHomeSystemAsPrimaryLevel)
	{
		GenerateHomeStarSystem();
	}
}

void AAstroGenerator::GenerateGalaxy()
{
	TSharedPtr<FGalaxyModel> GalaxyModel = MakeShared<FGalaxyModel>();

	if (bGenerateRandomGalaxy)
	{
		GalaxyGenerator->GenerateRandomGalaxyModel(GalaxyModel);
	}
	else
	{
		GalaxyModel->GalaxyClass = GalaxyGlass;
		GalaxyModel->GalaxyType = GalaxyType;
		GalaxyModel->StarsCount = bIsPreviewGeneration
			? FMath::Min(GalaxyStarCount, PreviewMaxInstances)
			: GalaxyStarCount;
		GalaxyModel->StarsDensity = GalaxyStarDensity;
		GalaxyModel->GalaxySize = GalaxySize;
	}

	UWorld* World = GetWorld();

	if (World)
	{
		AGalaxy* NewGalaxy = World->SpawnActor<AGalaxy>(BP_GalaxyClass);
		GalaxyGenerator->GenerateGalaxyOctreeStars(StarGenerator, NewGalaxy, GalaxyModel);
		NewGalaxy->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		GeneratedGalaxy = NewGalaxy;

		if (bGenerateFullScaledWorld)
		{
			this->SetActorScale3D(FVector(FullScaleValue));
		}
	}
}


void AAstroGenerator::ShowPlanetsList(TArray<TSharedPtr<FPlanetData>> PlanetDataMap)
{
	for (const TSharedPtr<FPlanetData>& PlanetDataPtr : PlanetDataMap)
	{
		if (PlanetDataPtr.IsValid())
		{
			FPlanetData PlanetData = *(PlanetDataPtr.Get());
			UE_LOG(LogTemp, Warning, TEXT("    Planet Order: %d"), PlanetData.PlanetOrder);
			UE_LOG(LogTemp, Warning, TEXT("     Orbit Radius: %f"), PlanetData.OrbitRadius);

			// �������� ������ �������
			TSharedPtr<FPlanetModel> PlanetModel = PlanetData.PlanetModel;

			// ����� ������ ������ �������
			if (PlanetModel.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("     Planet Type: %s"),
				       *UEnum::GetValueAsString(PlanetModel->PlanetType));
				UE_LOG(LogTemp, Warning, TEXT("     Planet Zone: %s"),
				       *UEnum::GetValueAsString(PlanetModel->PlanetZone));
				UE_LOG(LogTemp, Warning, TEXT("     Temperature: %d"), PlanetModel->Temperature);
				UE_LOG(LogTemp, Warning, TEXT("     Planet Density: %f"), PlanetModel->PlanetDensity);
				UE_LOG(LogTemp, Warning, TEXT("     Planet Gravity Strength: %f"),
				       PlanetModel->PlanetGravityStrength);
				UE_LOG(LogTemp, Warning, TEXT("     Amount of Moons: %d"), PlanetModel->AmountOfMoons);

				// ����� ���������� � ���������
				TArray<TSharedPtr<FMoonData>> MoonsList = PlanetModel->MoonsList;
				for (int32 i = 0; i < MoonsList.Num(); i++)
				{
					if (MoonsList[i].IsValid())
					{
						FMoonData MoonData = *(MoonsList[i].Get());

						// ������� ������ ��������
						UE_LOG(LogTemp, Warning, TEXT("         Moon Order: %d"), MoonData.MoonOrder);
						UE_LOG(LogTemp, Warning, TEXT("             Moon Orbit Radius: %f"), MoonData.OrbitRadius);

						// �������� ������ Moon
						TSharedPtr<FMoonModel> MoonModel = MoonData.MoonModel;

						// ����� ������ ������ Moon
						if (MoonModel.IsValid())
						{
							UE_LOG(LogTemp, Warning, TEXT("             Moon Type: %s"),
							       *UEnum::GetValueAsString(MoonModel->Type));
							UE_LOG(LogTemp, Warning, TEXT("             Moon Density: %f"), MoonModel->MoonDensity);
							UE_LOG(LogTemp, Warning, TEXT("             Moon Gravity: %f"), MoonModel->MoonGravity);
						}
					}
				}
			}
		}
	}
}

void AAstroGenerator::SpawnMoons(UWorld* World, APlanet* Planet, const int32 NumberOfMoons)
{
	if (!Planet || NumberOfMoons <= 0 || !BP_MoonClass || !BP_PlanetOrbitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid parameters for spawning moons."));
		return;
	}

	const double KM_TO_CM = 100000.0;
	const double SCALE_FACTOR = 1.0 / 50.0;

	// Planet radius in UE units
	const double PlanetRadiusUE = Planet->PlanetRadiusKM * KM_TO_CM * SCALE_FACTOR;
	const double MinOrbitDistanceUE = PlanetRadiusUE * 1.2;
	const double MaxOrbitDistanceUE = PlanetRadiusUE * 10;

	// Moon generation
	for (int32 i = 0; i < NumberOfMoons; ++i)
	{
		// Create an orbit for the moon
		if (APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(
			BP_PlanetOrbitClass, Planet->GetActorLocation(),
			FRotator::ZeroRotator))
		{
			NewMoonOrbit->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);
			Planet->MoonOrbitsList.Add(NewMoonOrbit);

			// Spawn the moon
			FVector MoonLocation = Planet->GetActorLocation();
			if (AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator))
			{
				NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
				Planet->AddMoon(NewMoon);
				NewMoon->SetParentPlanet(Planet);

				// Set the scale of the moon
				const double MoonRadiusKm = FMath::RandRange(100.0, 1000.0); // Moon radius in kilometers
				NewMoon->RadiusKM = MoonRadiusKm;
				const double MoonRadiusInCm = MoonRadiusKm * KM_TO_CM * SCALE_FACTOR;
				NewMoon->SetActorScale3D(FVector(MoonRadiusInCm));

				// Set the orbital distance
				const double OrbitDistanceUE = FMath::RandRange(MinOrbitDistanceUE, MaxOrbitDistanceUE);
				const FVector OffsetLocation = FVector(0, OrbitDistanceUE, 0);
				// Orbit of the moon at a distance from the planet
				NewMoon->AddActorLocalOffset(OffsetLocation);
				NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
				NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(World, NewMoon->RadiusKM, NewMoon);
			}
		}
	}
}

void AAstroGenerator::DestroyActorTree(AActor* Root)
{
	if (!IsValid(Root)) return;

	// 1) Собираем всех прямых "детей": как прикреплённых, так и из ChildActorComponent
	TArray<AActor*> Children;
	Root->GetAttachedActors(Children); // прямые прикреплённые

	TArray<UChildActorComponent*> ChildComps;
	Root->GetComponents<UChildActorComponent>(ChildComps);
	for (UChildActorComponent* CAC : ChildComps)
	{
		if (AActor* Child = CAC ? CAC->GetChildActor() : nullptr)
		{
			Children.AddUnique(Child);
		}
	}

	// 2) Рекурсивно удаляем "листья"
	for (AActor* Child : Children)
	{
		DestroyActorTree(Child);
	}

	// 3) Отцепляем и удаляем сам корень
	Root->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Root->Destroy();
}

void AAstroGenerator::SpawnPlanetMoons(const TSharedPtr<FPlanetModel>& PlanetModel)
{
	double DiameterOfLastMoon = 0;
	FVector LastMoonLocation;
	for (const TSharedPtr<FMoonData> MoonData : PlanetModel->MoonsList)
	{
		const double KM_TO_UE_UNIT_SCALE = 100000;
		APlanetOrbit* NewMoonOrbit = GetWorld()->SpawnActor<APlanetOrbit>(
			BP_PlanetOrbitClass, HomePlanet->GetActorLocation(), FRotator::ZeroRotator);
		NewMoonOrbit->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform);
		HomePlanet->MoonOrbitsList.Add(NewMoonOrbit);

		FVector MoonLocation = HomePlanet->GetActorLocation();
		AMoon* NewMoon = GetWorld()->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
		NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
		HomePlanet->AddMoon(NewMoon);
		NewMoon->SetParentPlanet(HomePlanet);

		MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
		MoonGenerator->ConnectMoonWithPlanet(NewMoon, HomePlanet);

		// set moon full-scale
		double MoonRadius = MoonData->MoonModel->Radius;
		NewMoon->RadiusKM = MoonRadius * 6371;
		NewMoon->SetActorScale3D(FVector(MoonRadius * 12742000));

		FVector Origin, BoxExtent;
		double SphereRadius;
		NewMoon->GetActorBounds(false, Origin, BoxExtent);
		SphereRadius = BoxExtent.GetMax();
		NewMoon->AffectionRadiusKM = SphereRadius / 100000.0;

		NewMoon->AddActorLocalOffset(FVector(
			0, ((PlanetModel->RadiusKM + (MoonData->OrbitRadius * PlanetModel->RadiusKM)) *
				KM_TO_UE_UNIT_SCALE) * 1, 0));
		NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);

		DiameterOfLastMoon = MoonRadius * 2;
		LastMoonLocation = NewMoon->GetActorLocation();

		SetMoonRotation(NewMoonOrbit);

		// Atmosphere metadata is independent from the streamed surface. Do not
		// allocate a WorldScape root here; the nearest-body streamer owns it.
		NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(GetWorld(), NewMoon->RadiusKM, NewMoon);
	}

	if (DiameterOfLastMoon == 0)
	{
		HomePlanet->PlanetaryZone->SetSphereRadius(100);
		HomePlanet->AffectionRadiusKM = 100 * HomePlanet->GetActorScale3D().X / 100000;
	}
	else
	{
		FVector PlanetLocation = HomePlanet->GetActorLocation();
		FVector LastMoonOuterEdgeLocation = LastMoonLocation + FVector(0, DiameterOfLastMoon * 6371, 0);
		double SphereRadius = FVector::Dist(PlanetLocation, LastMoonOuterEdgeLocation);
		SphereRadius /= HomePlanet->GetActorScale3D().X;
		SphereRadius *= 1.5;
		HomePlanet->PlanetaryZone->SetSphereRadius(SphereRadius);
		HomePlanet->AffectionRadiusKM = SphereRadius * HomePlanet->GetActorScale3D().X / 100000;
	}

	HomePlanet->OrbitHeight = (HomePlanet->GravityCollisionZone->GetScaledSphereRadius() / 100000) -
		HomePlanet->RadiusKM;

	HomePlanet->PlanetaryEnvironmentGenerator->InitEnviroment(HomePlanet, GetWorld());
}

void AAstroGenerator::ResolveSpawnLocation(const ASpaceship* NewHomeSpaceship, FVector& CharSpawnLocation)
{
	CharSpawnLocation = {0, 0, 0};
	switch (CharSpawnPlace)
	{
	case ECharSpawnPlace::PlanetOrbit:
		CharSpawnLocation = HomeSpaceStation->SpawnPoint->GetComponentLocation();
		break;
	case ECharSpawnPlace::PlanetSurface:
		break;
	case ECharSpawnPlace::MoonOrbit:
		break;
	case ECharSpawnPlace::MoonSurface:
		break;
	case ECharSpawnPlace::SpaceShip:
		{
			CharSpawnLocation = NewHomeSpaceship->GetActorLocation();
		}
		break;
	default:
		break;
	}
}

void AAstroGenerator::SpawnStartInteractiveActors(TSharedPtr<FPlanetModel> StartPlanetModel)
{
	/*
	 * SpawnModel
	 * BP_HomeSpaceHeadquarters
	 * BP_HomeSpaceStation
	 * BP_HomeSpaceship
	 * BP_CharacterClass
	 */

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = HomePlanet;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	double StationOrbitHeight = PlanetGenerator->CalculateOrbitHeight(
		HomeSpaceStationOrbitHeight, StartPlanetModel->Radius);
	FVector PlanetPosition = HomePlanet->GetActorLocation();

	HomeSpaceHeadquarters = World->SpawnActor<ASpaceHeadquarters>(
		BP_HomeSpaceHeadquarters, PlanetPosition, FRotator::ZeroRotator);
	HomeSpaceHeadquarters->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform);
	HomeSpaceHeadquarters->SetActorRelativeRotation(FRotator(0, 0, 0));

	const double EARTH_RADIUS_CM = 250000.0;
	const double SpawnOffset = StationOrbitHeight * EARTH_RADIUS_CM;
	FVector Offset = FVector(0, SpawnOffset, 0);
	HomeSpaceHeadquarters->AddActorWorldOffset(Offset);

	FVector HomeSpaceHeadquartersLocation = HomeSpaceHeadquarters->GetActorLocation();
	FRotator HomeSpaceHeadquartersRotation = HomeSpaceHeadquarters->GetActorRotation();
	HomeSpaceStation = World->SpawnActor<ASpaceStation>(
		BP_HomeSpaceStation, HomeSpaceHeadquartersLocation,
		HomeSpaceHeadquartersRotation, SpawnParams);
	HomeSpaceStation->AttachToActor(HomeSpaceHeadquarters,
	                                FAttachmentTransformRules::KeepWorldTransform);
	//double HomeStationOffset = HomeSpaceStation->GravityCollisionZone->GetScaledSphereRadius() * 2;
	double HomeStationOffset = 50000;
	HomeSpaceStation->AddActorLocalOffset(FVector(0, HomeStationOffset, 0));
	HomeSpaceStation->CalculateAffectionRadius();

	// Spawn HomeShipyard
	HomeSpaceShipyard = World->SpawnActor<ASpaceShipyard>(BP_HomeSpaceShipyard,
	                                                      HomeSpaceHeadquartersLocation,
	                                                      HomeSpaceHeadquartersRotation);
	/*double HomeSpaceShipyardLocationOffset = HomeSpaceShipyard->GravityCollisionZone->
	                                                            GetScaledSphereRadius() * 2;*/
	//double HomeSpaceShipyardLocationOffset = HomeSpaceShipyard->GetActorLocation();
	//(X=6000.000000,Y=-15360.000001,Z=-500.000000)
	HomeSpaceShipyard->AddActorLocalOffset(FVector(6000, -15360, -500));
	HomeSpaceShipyard->AttachToActor(HomeSpaceHeadquarters,
	                                 FAttachmentTransformRules::KeepWorldTransform);
	HomeSpaceShipyard->CalculateAffectionRadius();

	//Spawn HomeSpaceship
	FActorSpawnParameters SpaceshipSpawnParams;
	SpaceshipSpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FVector HomeSpaceshipLocation = HomeSpaceShipyard->SpawnPoint->GetComponentLocation();

	HomeSpaceshipLocation.Z += 1000;
	ASpaceship* NewHomeSpaceship = World->SpawnActor<ASpaceship>(
		BP_HomeSpaceship, HomeSpaceshipLocation, HomeSpaceShipyard->GetActorRotation(),
		SpaceshipSpawnParams);

	if (NewHomeSpaceship && GeneratedHomeStarSystem)
	{
		NewHomeSpaceship->AttachToActor(HomeSpaceShipyard,
		                                FAttachmentTransformRules::KeepWorldTransform);
		NewHomeSpaceship->OffsetSystem = GeneratedHomeStarSystem;

		if (NewHomeSpaceship->OnboardComputer)
		{
			NewHomeSpaceship->OnboardComputer->OffsetSystem = GeneratedHomeStarSystem;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Onboard Computer is nullptr!"));
		}
	}
	else
	{
		if (!NewHomeSpaceship)
		{
			UE_LOG(LogTemp, Error, TEXT("NewHomeSpaceship is nullptr!"));
		}

		if (!GeneratedHomeStarSystem)
		{
			UE_LOG(LogTemp, Error, TEXT("GeneratedHomeStarSystem is nullptr!"));
		}
	}

	if (APawn* PlayerCharacter = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		FVector CharSpawnLocation;
		ResolveSpawnLocation(NewHomeSpaceship, CharSpawnLocation);

		UE_LOG(LogTemp, Warning, TEXT("CharSpawnLocation: %s"), *CharSpawnLocation.ToString());
		bool bTeleportSuccess = PlayerCharacter->SetActorLocation(CharSpawnLocation, false);
		UE_LOG(LogTemp, Warning, TEXT("Teleport success: %s"),
		       bTeleportSuccess ? TEXT("True") : TEXT("False"));

		// Relocate spawned world to 000
		FVector PlayerLocation = HomeSpaceHeadquarters->GetActorLocation();
		FVector GeneratorLocation = this->GetActorLocation();
		FVector NewGeneratorLocation = GeneratorLocation - PlayerLocation;
		this->SetActorLocation(NewGeneratorLocation, false);
		
		FVector SpawnLocation = HomeSpaceHeadquarters->GetStartPointPosition();
		
		PlayerCharacter->SetActorLocation(SpawnLocation, false);
		PlayerCharacter->SetActorRotation(HomeSpaceShipyard->GetActorRotation());
		PlayerCharacter->AddActorLocalRotation(FRotator(0, 180 , 0));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter is null!"));
	}
}

void AAstroGenerator::SetMoonRotation(APlanetOrbit* NewMoonOrbit)
{
	NewMoonOrbit->SetActorRelativeRotation(FRotator(FMath::RandRange(-360.0, 360.0),
	                                                FMath::RandRange(-360.0, 360.0),
	                                                FMath::RandRange(-360.0, 360.0)));
}

bool AAstroGenerator::CheckGeneratorsFails()
{
	if (StarGenerator == nullptr || PlanetGenerator == nullptr || MoonGenerator == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
		return true;
	}

	if (!BP_PlanetarySystemClass || !BP_StarSystemClass || !BP_StarClass || !BP_PlanetClass || !BP_MoonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the blueprint classes is not assigned in the editor!"));
		return true;
	}
	return false;
}

FVector AAstroGenerator::GetHomeSystemSpawnLocationForGalaxy(TArray<AActor*> AttachedActors, int32 RandomIndex)
{
	FVector HomeSystemSpawnLocation{0};
	if (AGalaxy* GalaxyActor = Cast<AGalaxy>(AttachedActors[RandomIndex]))
	{
		if (UHierarchicalInstancedStaticMeshComponent* HismComponent = GalaxyActor->StarMeshInstances; HismComponent &&
			HismComponent->GetInstanceCount() > 0)
		{
			int32 RandomInstanceIndex = FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
			FTransform InstanceTransform;
			HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
			HomeSystemSpawnLocation = InstanceTransform.GetLocation();
		}
	}
	return HomeSystemSpawnLocation;
}

FVector AAstroGenerator::GetHomeSystemSpawnLocationForStarCluster(TArray<AActor*> AttachedActors, int32 RandomIndex)
{
	FVector HomeSystemSpawnLocation{0};
	if (AStarCluster* StarClusterActor = Cast<AStarCluster>(AttachedActors[RandomIndex]))
	{
		if (UHierarchicalInstancedStaticMeshComponent* HismComponent = StarClusterActor->StarMeshInstances;
			HismComponent && HismComponent->GetInstanceCount() > 0)
		{
			int32 RandomInstanceIndex = FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
			FTransform InstanceTransform;
			HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
			HomeSystemSpawnLocation = InstanceTransform.GetLocation();
		}
	}
	return HomeSystemSpawnLocation;
}

FVector AAstroGenerator::DetermineHomeSystemSpawnLocation()
{
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);

	std::unordered_map<EHomeSystemPosition, std::function<FVector()>> HomeSystemPositionMap = {
		{EHomeSystemPosition::WorldCenter, []() { return FVector(0, 0, 0); }},
		{
			EHomeSystemPosition::RandomPosition, []()
			{
				double RandomX = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomY = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomZ = FMath::RandRange(-1000000000000, 1000000000000);
				return FVector(RandomX, RandomY, RandomZ);
			}
		},
		{
			EHomeSystemPosition::DirectPosition, [this, AttachedActors]()
			{
				if (AttachedActors.Num() > 0)
				{
					int32 RandomIndex = FMath::RandRange(0, AttachedActors.Num() - 1);
					std::unordered_map<EAstroGenerationLevel, std::function<FVector()>> GenerationLevelMap = {
						{
							EAstroGenerationLevel::Galaxy,
							[this, AttachedActors, RandomIndex]()
							{
								return GetHomeSystemSpawnLocationForGalaxy(AttachedActors, RandomIndex);
							}
						},
						{
							EAstroGenerationLevel::StarCluster,
							[this, AttachedActors, RandomIndex]()
							{
								return GetHomeSystemSpawnLocationForStarCluster(AttachedActors, RandomIndex);
							}
						}
					};

					return (GenerationLevelMap.find(AstroGenerationLevel) != GenerationLevelMap.end())
						       ? GenerationLevelMap[AstroGenerationLevel]()
						       : FVector(0, 0, 0);
				}
				return FVector(0, 0, 0);
			}
		}
	};

	return (HomeSystemPositionMap.find(HomeSystemPosition) != HomeSystemPositionMap.end())
		       ? HomeSystemPositionMap[HomeSystemPosition]()
		       : FVector(0, 0, 0);
}

void AAstroGenerator::GenerateStarSystem(AStarSystem* NewStarSystem, TSharedPtr<FStarSystemModel> StarSystemModel)
{
	UWorld* World = GetWorld();

	std::unordered_map<EStarType, std::function<void()>> StarTypeMap = {
		{EStarType::SingleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = 1; }},
		{EStarType::DoubleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = 2; }},
		{EStarType::TripleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = 3; }},
		{EStarType::MultipleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = FMath::RandRange(4, 6); }}
	};

	/*StarTypeMap.find(HomeSystemStarType) != StarTypeMap.end()
		? StarTypeMap[HomeSystemStarType]()
		: StarSystemModel->AmountOfStars = 1;*/

	if (auto it = StarTypeMap.find(HomeSystemStarType); it != StarTypeMap.end())
	{
		it->second(); // вызываем колбэк
	}
	else
	{
		StarSystemModel->AmountOfStars = 1; // дефолт
	}
	

	//FVector LastStarLocation{0};
	for (int StarNumber = 0; StarNumber < StarSystemModel->AmountOfStars; StarNumber++)
	{
		const TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();

		if (bRandomHomeStar)
		{
			StarGenerator->GenerateRandomStarModel(StarModel);
		}
		else
		{
			StarModel->StellarType = HomeStarStellarType;
			StarModel->SpectralClass = HomeStarSpectralClass;
			StarGenerator->GenerateStarModel(StarModel);
		}

		PlanetarySystemModel = MakeShared<FPlanetarySystemModel>();

		if (bRandomHomeSystemType)
		{
			PlanetarySystemGenerator->GeneratePlanetarySystemModelByStar(
				PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);
		}
		else
		{
			PlanetarySystemModel->AmountOfPlanets = PlanetsAmount;
			PlanetarySystemModel->PlanetarySystemType = HomeSystemPlanetaryType;
			PlanetarySystemModel->OrbitDistributionType = HomeSystemOrbitDistributionType;
			PlanetarySystemGenerator->GenerateCustomPlanetarySystemModel(
				PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);
		}

		AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass);
		APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);

		if (StarNumber == 0)
		{
			NewStarSystem->MainStar = NewStar;
		}

		if (!NewStar || !NewPlanetarySystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("Star Falied!"));
			return;
		}

		StarGenerator->ApplyModel(NewStar, StarModel);
		PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetarySystemModel);
		const FVector SystemCenter = NewStarSystem->GetActorLocation();
		NewStar->SetActorLocation(SystemCenter);
		NewPlanetarySystem->SetActorLocation(SystemCenter);
		NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
		NewStar->StarRadiusKM = StarModel->Radius * 696340;
		NewStar->SetPlanetarySystem(NewPlanetarySystem);
		NewPlanetarySystem->SetStar(NewStar);
		NewStarSystem->AddNewStar(NewStar);
		NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
		NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
		NewStar->FullSpectralName = NewStar->GenerateFullSpectralName();
		NewPlanetarySystem->SetStarFullSpectralName(NewStar->FullSpectralName);
		const FString SpectralIdentity = NewStar->FullSpectralName.IsNone()
			? TEXT("Star") : NewStar->FullSpectralName.ToString();
		NewStar->AstroName = AGravityPlayerController::GenerateUniqueName(SpectralIdentity);

		StarGenerator->ApplySpectralMaterial(NewStar, StarModel);

		//FVector LastPlanetLocation{0};
		for (const TSharedPtr<FPlanetData>& PlanetData : PlanetarySystemModel->PlanetsList)
		{
			APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(
				BP_PlanetOrbitClass, NewPlanetarySystem->GetActorLocation(), FRotator::ZeroRotator);
			NewPlanetOrbit->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->PlanetOrbitsList.Add(NewPlanetOrbit);

			TSharedPtr<FPlanetModel> PlanetModel = PlanetData->PlanetModel;
			APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);

			PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
			NewPlanet->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Planet"));
			NewStar->AddPlanet(NewPlanet);
			NewPlanet->SetParentStar(NewStar);

			NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
			const FVector OrbitOffset(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
			const FVector NewLocation = NewPlanetOrbit->GetActorLocation()
				+ NewPlanetOrbit->GetActorQuat().RotateVector(OrbitOffset);
			NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
			NewPlanet->SetActorLocation(NewLocation);

			NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->PlanetsActorsList.Add(NewPlanet);

			NewPlanetOrbit->Planet = NewPlanet;

			/*const double KM_TO_UE_UNIT_SCALE = 100000;
			double DiameterOfLastMoon = 0;
			FVector LastMoonLocation;*/

			for (const TSharedPtr<FMoonData>& MoonData : PlanetModel->MoonsList)
			{
				APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(
					BP_PlanetOrbitClass, NewPlanet->GetActorLocation(), FRotator::ZeroRotator);
				NewMoonOrbit->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanet->MoonOrbitsList.Add(NewMoonOrbit);

				FVector MoonLocation = NewPlanet->GetActorLocation();
				AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
				NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
				NewPlanet->AddMoon(NewMoon);
				NewMoon->SetParentPlanet(NewPlanet);

				MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
				MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);

				NewMoon->SetActorScale3D(FVector(MoonData->MoonModel->Radius * 12742000));
				const FVector MoonOrbitOffset(
					MoonData->MoonModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
				NewMoon->SetActorLocation(NewMoonOrbit->GetActorLocation()
					+ NewMoonOrbit->GetActorQuat().RotateVector(MoonOrbitOffset));

				NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}
}

void AAstroGenerator::RotatePlanetOrbits(APlanetarySystem* NewPlanetarySystem)
{
	for (APlanetOrbit* PlanetOrbit : NewPlanetarySystem->PlanetOrbitsList)
	{
		const float RandomZRotation = FMath::RandRange(-360.0f, 360.0f);
		const float RandomYRotation = FMath::RandRange(-15.0f, 15.0f);
		const FRotator NewRotation = FRotator(RandomYRotation, RandomZRotation, 0);
		PlanetOrbit->AddActorLocalRotation(NewRotation);

		TArray<AActor*> AttachedActors;
		PlanetOrbit->GetAttachedActors(AttachedActors);

		for (AActor* AttachedActor : AttachedActors)
		{
			if (APlanet* Planet = Cast<APlanet>(AttachedActor))
			{
				// Iterate through the list of moons for this planet.
				for (APlanetOrbit* MoonOrbit : Planet->MoonOrbitsList)
				{
					const float RandomXRotationMoon = FMath::RandRange(-360.0f, 360.0f);
					const float RandomYRotationMoon = FMath::RandRange(-360.0f, 360.0f);
					const float RandomZRotationMoon = FMath::RandRange(-360.0f, 360.0f);
					const FRotator NewRotationMoon = FRotator(RandomXRotationMoon, RandomYRotationMoon,
					                                          RandomZRotationMoon);
					MoonOrbit->AddActorLocalRotation(NewRotationMoon);
				}
			}
		}
	}
}

void AAstroGenerator::ComputeHomeSystemPosition(FTransform& HomeSystemTransform, FVector& HomeSystemSpawnLocation)
{
	PendingHomeCluster.Reset();
	PendingHomeClusterInstanceIndex = INDEX_NONE;
	HomeSystemSpawnLocation = {0, 0, 0};
	switch (HomeSystemPosition)
	{
	case EHomeSystemPosition::WorldCenter:
		HomeSystemSpawnLocation = FVector(0, 0, 0);
		break;
	case EHomeSystemPosition::RandomPosition:
		{
			// Set HomeSystemSpawnLocation depending on your definition of center, middle, and end
			const double RandomX = FMath::RandRange(-1000000000000, 1000000000000);
			const double RandomY = FMath::RandRange(-1000000000000, 1000000000000);
			const double RandomZ = FMath::RandRange(-1000000000000, 1000000000000);
			HomeSystemSpawnLocation = FVector(RandomX, RandomY, RandomZ);
		}
		break;
	case EHomeSystemPosition::DirectPosition:
		{
			// Make sure there are actors attached
			TArray<AActor*> AttachedActors;
			GetAttachedActors(AttachedActors);
			if (AttachedActors.Num() > 0)
			{
				const int32 RandomIndex = FMath::RandRange(0, AttachedActors.Num() - 1);
				if (AstroGenerationLevel == EAstroGenerationLevel::Galaxy)
				{
					// Cast the actor to type AGalaxy
					if (const AGalaxy* GalaxyActor = Cast<AGalaxy>(AttachedActors[RandomIndex]))
					{
						// If the actor is an instance of the AGalaxy class, extract its HISM component
						if (UHierarchicalInstancedStaticMeshComponent* HismComponent = GalaxyActor->
							StarMeshInstances; HismComponent && HismComponent->GetInstanceCount() > 0)
						{
							// Get a random index from the range of available instances
							const int32 RandomInstanceIndex =
								FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
							// Extract random instance transform
							FTransform InstanceTransform;
							HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
							// Use position from instance transform as HomeSystemSpawnLocation
							HomeSystemSpawnLocation = InstanceTransform.GetLocation();
						}
					}
				}
				else if (AstroGenerationLevel == EAstroGenerationLevel::StarCluster)
				{
					AStarCluster* StarClusterActor = GeneratedStarCluster;
					if (!StarClusterActor)
					{
						for (AActor* AttachedActor : AttachedActors)
						{
							if (AStarCluster* Candidate = Cast<AStarCluster>(AttachedActor))
							{
								StarClusterActor = Candidate;
								break;
							}
						}
					}
					if (StarClusterActor)
					{
						// If the actor is an instance of the AStarCluster class, retrieve its HISM component.
						if (const UHierarchicalInstancedStaticMeshComponent* HismComponent = StarClusterActor->
							StarMeshInstances; HismComponent && HismComponent->GetInstanceCount() > 0)
						{
							// Get a random index from the range of available instances
							const int32 RandomInstanceIndex =
								FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
							if (const FClusterStarSystemRecord* Record =
								StarClusterActor->FindPotentialSystem(RandomInstanceIndex))
							{
								HomeSystemSpawnLocation = StarClusterActor->GetPotentialSystemWorldLocation(*Record);
								PendingHomeCluster = StarClusterActor;
								PendingHomeClusterInstanceIndex = RandomInstanceIndex;
							}
							else
							{
								FTransform InstanceTransform;
								HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
								HomeSystemSpawnLocation = InstanceTransform.GetLocation();
							}
						}
					}
				}
			}
		}
		break;
	default:
		break;
	}
	HomeSystemTransform.SetLocation(HomeSystemSpawnLocation);
	HomeSystemTransform.SetRotation(FRotator::ZeroRotator.Quaternion());
}

void AAstroGenerator::ComputeStarAmount(TSharedPtr<FStarSystemModel>& StarSystemModel, int& AmountOfStars)
{
	StarSystemModel = MakeShared<FStarSystemModel>();
	AmountOfStars = {0};
	switch (HomeSystemStarType)
	{
	case EStarType::SingleStar:
		AmountOfStars = 1;
		break;
	case EStarType::DoubleStar:
		AmountOfStars = 2;
		break;
	case EStarType::TripleStar:
		AmountOfStars = 3;
		break;
	case EStarType::MultipleStar:
		AmountOfStars = FMath::RandRange(4, 6);
		break;
	default:
		AmountOfStars = 1;
		break;
	}
	StarSystemModel->AmountOfStars = AmountOfStars;
}

TMap<EStarClusterType, TPair<int, int>> ClusterStarAmount =
{
	{EStarClusterType::OpenCluster, {500, 5000}},
	{EStarClusterType::GlobularCluster, {5000, 25000}},
	{EStarClusterType::Supercluster, {25000, 50000}},
	{EStarClusterType::Nebula, {10000, 20000}},
	{EStarClusterType::Unknown, {0, 0}}
};

int AAstroGenerator::GetRandomValueFromStarAmountRange(const EStarClusterType ClusterType)
{
	if (ClusterStarAmount.Contains(ClusterType))
	{
		const TPair<int, int> Range = ClusterStarAmount[ClusterType];
		return FMath::RandRange(Range.Key, Range.Value);
	}
	UE_LOG(LogTemp, Warning, TEXT("Cluster type not found in map, or range is invalid"));
	return 0;
}

void AAstroGenerator::Test_GenerateFullscaled()
{
	// ������ ��������� �����
	const double MinStarRadius = 0.1f;
	const double MaxStarRadius = 10.0f;
	const int StarCount = 100;

	// ������� �����-����� ��� ������ ����
	AAstroAnchor* AstroAnchor = GetWorld()->SpawnActor<AAstroAnchor>(BP_AstroAnchorClass);

	// ������� ����� �������� ����� � ����������� ��� � �����
	AStarCluster* StarCluster = GetWorld()->SpawnActor<AStarCluster>(BP_StarClusterClass);
	StarCluster->AttachToActor(AstroAnchor, FAttachmentTransformRules::KeepRelativeTransform);

	// ������� ������� �����
	for (int i = 0; i < StarCount; ++i)
	{
		// ���������� ��������� ���������� � ������ ��� ������
		FVector StarPosition = FMath::VRand() * FMath::FRand() * 5.0f;
		double StarRadius = FMath::RandRange(MinStarRadius, MaxStarRadius);

		// ������� �������������� ��� ������
		FTransform StarTransform;
		StarTransform.SetLocation(StarPosition * 10000);
		StarTransform.SetScale3D(FVector(StarRadius));

		// ��������� ������� ������ � HISM
		StarCluster->StarMeshInstances->AddInstance(StarTransform);
	}

	// ���������� �����-����� � ����� �������� � ������ ����������
	GeneratedWorld = AstroAnchor;
	GeneratedStarCluster = StarCluster;
}

void AAstroGenerator::GeneratePlanetSystem()
{
	GenerateStarSystemByModel();
}

void AAstroGenerator::GenerateSinglePlanet()
{
	const int32 SavedPlanetsAmount = PlanetsAmount;
	const int32 SavedStartPlanetNumber = StartPlanetNumber;
	PlanetsAmount = 1;
	StartPlanetNumber = 1;
	GenerateStarSystemByModel();
	PlanetsAmount = SavedPlanetsAmount;
	StartPlanetNumber = SavedStartPlanetNumber;
}

void AAstroGenerator::GenerateRandomWorld()
{
}

void AAstroGenerator::GenerateGalaxiesCluster()
{
}

void AAstroGenerator::IntegrateStartPlanetIntoSystem()
{
	UE_LOG(LogTemp, Warning, TEXT("=== IntegrateStartPlanetIntoSystem START ==="));
	
	if (!WSR_StartHomePlanet || !GeneratedHomeStarSystem || !BP_Headquarters)
	{
		UE_LOG(LogTemp, Error, TEXT("IntegrateStartPlanetIntoSystem: Required references are null!"));
		UE_LOG(LogTemp, Error, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("GeneratedHomeStarSystem: %s"), GeneratedHomeStarSystem ? TEXT("Valid") : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("BP_Headquarters: %s"), BP_Headquarters ? TEXT("Valid") : TEXT("NULL"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("StartHomePlanet: %s"), *WSR_StartHomePlanet->GetName());
	UE_LOG(LogTemp, Warning, TEXT("BP_Headquarters: %s"), *BP_Headquarters->GetName());
	UE_LOG(LogTemp, Warning, TEXT("GeneratedHomeStarSystem: %s"), *GeneratedHomeStarSystem->GetName());
	UE_LOG(LogTemp, Warning, TEXT("Using HomePlanet: %s"), *HomePlanet->GetName());

	// Single Play integrates an editor-authored start world. Its WorldScapeRoot is
	// already part of the level and must not be treated as another procedural body.
	// Otherwise the runtime streamer creates a second terrain surface around the
	// generated hierarchy planet as soon as play begins. Random-generation mode
	// does not enter this integration path and keeps normal nearest-body streaming.
	HomePlanet->bStreamWorldScapeSurface = false;
	HomePlanet->bGenerateByDefault = false;
	
	// Находим нужную орбиту и перемещаем домашнюю планету (иерархия от станции)
	APlanetOrbit* TargetOrbit = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList[StartPlanetNumber - 1];
	const FVector& TargetPlanetPosition = TargetOrbit->Planet->GetActorLocation();
	BP_Headquarters->SetActorLocation(TargetPlanetPosition, false, nullptr, ETeleportType::ResetPhysics);

	// Прикрепляем систему к актору планеты
	GeneratedHomeStarSystem->AttachToActor(BP_Headquarters, FAttachmentTransformRules::KeepWorldTransform);
	BP_Headquarters->SetActorLocation(FVector(0, 0, 0));
	
	// Возвращаем иерархию к стандарту
	GeneratedHomeStarSystem->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	BP_Headquarters->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform);
	HomePlanet->AttachToActor(TargetOrbit, FAttachmentTransformRules::KeepWorldTransform);

	// Удаляем дефолтные акторы планеты
	DestroyActorTree(TargetOrbit->Planet);

	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Pawn->SetActorLocation(BP_Headquarters->GetStartPointPosition());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== IntegrateStartPlanetIntoSystem END ==="));
}

void AAstroGenerator::GenerateStarSystemAndIntegratePlanet()
{
	UE_LOG(LogTemp, Warning, TEXT("=== GenerateStarSystemAndIntegratePlanet START ==="));
	UE_LOG(LogTemp, Warning, TEXT("bIntegrateStartPlanet: %s"), bIntegrateStartPlanet ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("StartPlanetNumber: %d"), StartPlanetNumber);
	
	// Check if we should integrate StartPlanet
	if (!bIntegrateStartPlanet || !WSR_StartHomePlanet)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateStarSystemAndIntegratePlanet: bIntegrateStartPlanet is false or StartHomePlanet is null!"));
		UE_LOG(LogTemp, Error, TEXT("bIntegrateStartPlanet: %s"), bIntegrateStartPlanet ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Error, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
		return;
	}
	
	// Generate the star system first
	UE_LOG(LogTemp, Warning, TEXT("Generating star system..."));
	GenerateStarSystemByModel();

	// Check if star system was generated successfully
	if (!GeneratedHomeStarSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateStarSystemAndIntegratePlanet: Failed to generate star system!"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Star system generated successfully!"));
	UE_LOG(LogTemp, Warning, TEXT("GeneratedHomeStarSystem: %s"), *GeneratedHomeStarSystem->GetName());
	UE_LOG(LogTemp, Warning, TEXT("MainStar: %s"), GeneratedHomeStarSystem->MainStar ? TEXT("Valid") : TEXT("NULL"));
	
	if (GeneratedHomeStarSystem->MainStar && GeneratedHomeStarSystem->MainStar->PlanetarySystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlanetarySystem: %s"), *GeneratedHomeStarSystem->MainStar->PlanetarySystem->GetName());
		UE_LOG(LogTemp, Warning, TEXT("Available orbits: %d"), GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList.Num());
	}
	
	// Now integrate our planet
	UE_LOG(LogTemp, Warning, TEXT("Integrating StartPlanet into generated star system"));
	IntegrateStartPlanetIntoSystem();
	
	UE_LOG(LogTemp, Warning, TEXT("=== GenerateStarSystemAndIntegratePlanet END ==="));
}

void AAstroGenerator::SetAutoStarSystemModel()
{
}

void AAstroGenerator::GenerateRandomStarSystemModel()
{
}




//TargetOrbit->Planet->Destroy();
	
	//TargetOrbit->Planet->AttachToActor(TargetOrbit, FAttachmentTransformRules::KeepWorldTransform);

	
	
	/// !!!!! не прикрепляется BP_Headquarters->AttachToActor(TargetOrbit->Planet, FAttachmentTransformRules::KeepWorldTransform);
	


	/*if (AGravityCharacterPawn* PlayerPawn = Cast<AGravityCharacterPawn>(GetWorld()->GetFirstPlayerController()->GetPawn()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Teleporting player to StartPoint (0,0,0)"));
		PlayerPawn->SetActorLocation(FVector(1000, 1000, 1000));
	}*/
	

	/*
	// Get the target orbit for the start planet
	if (StartPlanetNumber < 1 || StartPlanetNumber > GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid StartPlanetNumber %d! Available orbits: %d"), 
			StartPlanetNumber, GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList.Num());
		return;
	}

	APlanetOrbit* TargetOrbit = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList[StartPlanetNumber - 1];
	if (!TargetOrbit)
	{
		UE_LOG(LogTemp, Error, TEXT("Target orbit is null!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Target orbit: %s"), *TargetOrbit->GetName());
	UE_LOG(LogTemp, Warning, TEXT("Target orbit position: %s"), *TargetOrbit->GetActorLocation().ToString());

	// Clear existing planet from orbit if any
	if (TargetOrbit->Planet)
	{
		UE_LOG(LogTemp, Warning, TEXT("Clearing existing planet from orbit"));
		TargetOrbit->TriggerClearChildren();
	}

	// Set HomePlanet mobility to Movable if needed
	if (HomePlanet->GetRootComponent()->Mobility == EComponentMobility::Static)
	{
		UE_LOG(LogTemp, Warning, TEXT("Setting HomePlanet mobility to Movable"));
		HomePlanet->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	}

	// Move HomePlanet to orbit position
	FVector OrbitPosition = TargetOrbit->GetActorLocation();
	UE_LOG(LogTemp, Warning, TEXT("Moving HomePlanet to orbit position: %s"), *OrbitPosition.ToString());
	
	HomePlanet->SetActorLocation(OrbitPosition, false, nullptr, ETeleportType::TeleportPhysics);

	// Attach HomePlanet to the orbit
	UE_LOG(LogTemp, Warning, TEXT("Attaching HomePlanet to orbit"));
	bool bAttachResult = HomePlanet->AttachToActor(TargetOrbit, FAttachmentTransformRules::KeepRelativeTransform);
	UE_LOG(LogTemp, Warning, TEXT("Attach result: %s"), bAttachResult ? TEXT("SUCCESS") : TEXT("FAILED"));

	// Set the planet reference in the orbit
	TargetOrbit->Planet = HomePlanet;
	if (TargetOrbit->Planet)
	{
		UE_LOG(LogTemp, Warning, TEXT("Planet reference set in orbit"));
	}

	// Get StartPoint position from Headquarters
	FVector StartPointPosition = BP_Headquarters->GetStartPointPosition();
	UE_LOG(LogTemp, Warning, TEXT("StartPoint position: %s"), *StartPointPosition.ToString());

	// Get StarSystem center position
	FVector StarSystemPosition = GeneratedHomeStarSystem->GetActorLocation();
	UE_LOG(LogTemp, Warning, TEXT("StarSystem position: %s"), *StarSystemPosition.ToString());

	// Calculate offset to move StartPoint to (0,0,0)
	FVector OffsetToCenter = FVector::ZeroVector - StartPointPosition;
	UE_LOG(LogTemp, Warning, TEXT("Offset to center StartPoint: %s"), *OffsetToCenter.ToString());

	// Move the entire StarSystem by the offset (this will move all planets and orbits)
	FVector NewStarSystemPosition = StarSystemPosition + OffsetToCenter;
	UE_LOG(LogTemp, Warning, TEXT("Moving StarSystem from %s to %s"), *StarSystemPosition.ToString(), *NewStarSystemPosition.ToString());
	
	GeneratedHomeStarSystem->SetActorLocation(NewStarSystemPosition);
	
	// Also move the BP_Headquarters by the same offset to keep StartPoint at (0,0,0)
	FVector HeadquartersCurrentPosition = BP_Headquarters->GetActorLocation();
	FVector NewHeadquartersPosition = HeadquartersCurrentPosition + OffsetToCenter;
	UE_LOG(LogTemp, Warning, TEXT("Moving BP_Headquarters from %s to %s"), *HeadquartersCurrentPosition.ToString(), *NewHeadquartersPosition.ToString());
	
	BP_Headquarters->SetActorLocation(NewHeadquartersPosition);

	// Teleport player to StartPoint (which is now at 0,0,0)
	AGravityCharacterPawn* PlayerPawn = Cast<AGravityCharacterPawn>(GetWorld()->GetFirstPlayerController()->GetPawn());
	if (PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Teleporting player to StartPoint (0,0,0)"));
		PlayerPawn->SetActorLocation(FVector::ZeroVector);
		
		// Verify final positions
		FVector FinalPlayerPosition = PlayerPawn->GetActorLocation();
		FVector FinalStarSystemPosition = GeneratedHomeStarSystem->GetActorLocation();
		FVector FinalStartPointPosition = BP_Headquarters->GetStartPointPosition();
		FVector FinalHeadquartersPosition = BP_Headquarters->GetActorLocation();
		FVector FinalPlanetPosition = HomePlanet->GetActorLocation();
		
		UE_LOG(LogTemp, Warning, TEXT("Final player position: %s"), *FinalPlayerPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final StarSystem position: %s"), *FinalStarSystemPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final StartPoint position: %s"), *FinalStartPointPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final Headquarters position: %s"), *FinalHeadquartersPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final Planet position: %s"), *FinalPlanetPosition.ToString());
		
		// Check if StartPoint is actually at (0,0,0)
		FVector StartPointDistance = FinalStartPointPosition - FVector::ZeroVector;
		float StartPointDistanceMagnitude = StartPointDistance.Size();
		UE_LOG(LogTemp, Warning, TEXT("StartPoint distance from origin: %f"), StartPointDistanceMagnitude);
		
		// Check if planet is attached to orbit
		AActor* PlanetParent = HomePlanet->GetAttachParentActor();
		UE_LOG(LogTemp, Warning, TEXT("Planet attach parent: %s"), PlanetParent ? *PlanetParent->GetName() : TEXT("NULL"));
		
		UE_LOG(LogTemp, Warning, TEXT("Integration completed successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerPawn not found!"));
	}*/
