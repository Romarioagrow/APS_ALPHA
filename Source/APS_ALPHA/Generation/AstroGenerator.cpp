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

void AAstroGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoGeneration)
	{
		InitAstroGenerators();

		InitGenerationLevel();
	}
}

void AAstroGenerator::GenerateWorldByModel()
{
	InitAstroGenerators();

	ApplyWorldModel();

	ApplySpawnParameters();

	GenerateStarCluster();

	GenerateHomeStarSystem();
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
	GalaxyStarDensity = GeneratedWorldModel->GalaxyStarDensity;
	HomePlanetarySystem = GeneratedWorldModel->HomePlanetarySystem;
	HomePlanet = GeneratedWorldModel->HomePlanet;
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
	NewStarCluster->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

	// Calculate Cluster Params
	NewStarCluster->StarAmount = StarClusterGenerator->GetStarsAmountByRange(StarClusterModel->StarClusterSize);
	NewStarCluster->StarDensity = StarClusterGenerator->GetStarClusterDensityByRange();
	NewStarCluster->ClusterBounds = StarClusterGenerator->GetStarClusterBoundsByRange(ClusterType);
	NewStarCluster->ClusterType = ClusterType;
	NewStarCluster->StarClusterComposition = StarClusterModel->StarClusterComposition;
	NewStarCluster->StarClusterPopulation = StarClusterModel->StarClusterPopulation;
	NewStarCluster->StarClusterSize = StarClusterModel->StarClusterSize;
	NewStarCluster->CalculateAffectionRadius();

	UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), NewStarCluster->StarAmount);
	UE_LOG(LogTemp, Warning, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
	UE_LOG(LogTemp, Warning, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
	UE_LOG(LogTemp, Warning, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

	for (size_t i = 0; i < NewStarCluster->StarAmount; i++)
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
		NewStarCluster->AddStarToClusterModel(StarPosition, NewStarModel);
		NewStarModel->Location = StarPosition;

		// Create a star instance and add it to the HISM component
		FTransform StarTransform(StarPosition);
		StarTransform.SetScale3D(FVector(NewStarModel->Radius));
		int32 StarInstIndex = NewStarCluster->StarMeshInstances->AddInstance(StarTransform, true);
		const FLinearColor ColorValue = StarGenerator->GetStarColor(NewStarModel->SpectralClass,
		                                                            NewStarModel->SpectralSubclass);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);

		const double StarEmission = StarGenerator->CalculateEmission(NewStarModel->Luminosity * 25);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);

		StarIndexModelMap.Add(StarInstIndex, NewStarModel);
	}

	GeneratedStarCluster = NewStarCluster;

	if (bGenerateFullScaledWorld)
	{
		SetActorScale3D(FVector(FullScaleValue, FullScaleValue, FullScaleValue));
	}
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
			
			// Validate PlanetaryEnvironmentGenerator before calling GenerateWorldscapeSurfaceByModel
			if (HomePlanet->PlanetaryEnvironmentGenerator)
			{
				HomePlanet->PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(GetWorld(), HomePlanet);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("PlanetaryEnvironmentGenerator is null for HomePlanet!"));
				// Try to initialize it manually
				HomePlanet->PlanetaryEnvironmentGenerator = NewObject<APlanetarySurfaceGenerator>();
				if (HomePlanet->PlanetaryEnvironmentGenerator)
				{
					HomePlanet->PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(GetWorld(), HomePlanet);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to create PlanetaryEnvironmentGenerator!"));
				}
			}
			
			HomePlanet->AstroName = AGravityPlayerController::GenerateUniqueName("Planet");

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
	if (CheckGeneratorsFails()) return;

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

		AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass, HomeSystemTransform);
		NewStarSystem->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		NewStarSystem->SetActorLocation(HomeSystemSpawnLocation);
		if (!NewStarSystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
			return;
		}
		StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

		FVector LastStarLocation{0};
		for (int StarNumber = 0; StarNumber < AmountOfStars; StarNumber++)
		{
			TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();

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

			// Set Star full-scale
			// TODO: PlanetarySystemGenerator->ConnectStar()
			StarGenerator->ApplyModel(NewStar, StarModel);
			PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetarySystemModel);
			NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
			NewStar->StarRadiusKM = StarModel->Radius * 696340;
			NewStar->SetPlanetarySystem(NewPlanetarySystem);
			NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
			StarGenerator->ApplySpectralMaterial(NewStar, StarModel);
			NewStar->AstroName = AGravityPlayerController::GenerateUniqueName("");
			NewStar->FullSpectralName = NewStar->GenerateFullSpectralName();
			HomeStar = NewStar;

			// Generate planets for each star
			FVector LastPlanetLocation{0};
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
				NewStar->AddPlanet(NewPlanet);
				NewPlanet->SetParentStar(NewStar);

				// Set planet full-scale
				NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
				FVector NewLocation = FVector(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
				NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
				NewPlanet->SetActorLocation(NewLocation);
				NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanetarySystem->PlanetsActorsList.Add(NewPlanet);
				NewPlanetOrbit->Planet = NewPlanet;

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
		NewStarSystem->CalculateAffectionRadius();

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
		GenerateStarSystemByModel();
		break;
	case EAstroGenerationLevel::PlanetSystem:
		GeneratePlanetSystem();
		break;
	case EAstroGenerationLevel::SinglePlanet:
		GenerateSinglePlanet();
		break;
	default:
		GenerateRandomWorld();
		break;
	}

	if (bGenerateHomeSystem)
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
		GalaxyModel->StarsCount = GalaxyStarCount;
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
			const double Scale = 1000000000.0;
			FVector VectorScale = FVector(1000000000, 1000000000, 1000000000);
			VectorScale = VectorScale * Scale;
			this->SetActorScale3D(VectorScale);
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

		///Generate WSC
		NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(GetWorld(), NewMoon->RadiusKM, NewMoon);
		NewMoon->PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(GetWorld(), NewMoon);
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
		// (X=2658.399507,Y=-5578.769877,Z=281.717353)

		FVector SpawnLocation = HomeSpaceShipyard->SpawnPoint->GetComponentLocation();
		
		//PlayerCharacter->SetActorLocation(FVector(2600, -5578, 281), false);
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

	StarTypeMap.find(HomeSystemStarType) != StarTypeMap.end()
		? StarTypeMap[HomeSystemStarType]()
		: StarSystemModel->AmountOfStars = 1;

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
		NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
		NewStar->StarRadiusKM = StarModel->Radius * 696340;
		NewStar->SetPlanetarySystem(NewPlanetarySystem);
		NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
		NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);

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
			NewStar->AddPlanet(NewPlanet);
			NewPlanet->SetParentStar(NewStar);

			NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
			FVector NewLocation = FVector(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
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
				NewPlanet->AddMoon(NewMoon);
				NewMoon->SetParentPlanet(NewPlanet);

				MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
				MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);

				NewMoon->SetActorScale3D(FVector(MoonData->MoonModel->Radius * 12742000));
				FVector Location = FVector(MoonData->MoonModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
				NewMoon->SetActorLocation(Location);

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
					// Cast the actor to type AStarCluster
					if (const AStarCluster* StarClusterActor = Cast<AStarCluster>(AttachedActors[RandomIndex]))
					{
						// If the actor is an instance of the AStarCluster class, retrieve its HISM component.
						if (const UHierarchicalInstancedStaticMeshComponent* HismComponent = StarClusterActor->
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
}

void AAstroGenerator::GenerateSinglePlanet()
{
}

void AAstroGenerator::GenerateRandomWorld()
{
}

void AAstroGenerator::GenerateGalaxiesCluster()
{
}

void AAstroGenerator::SetAutoStarSystemModel()
{
}

void AAstroGenerator::GenerateRandomStarSystemModel()
{
}
