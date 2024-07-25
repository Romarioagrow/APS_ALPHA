#include "AstroGenerator.h"
#include <Kismet/GameplayStatics.h>
#include "PlanetaryEnvironmentGenerator.h"
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

AAstroGenerator::AAstroGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAstroGenerator::SetGeneratedWorld(UGeneratedWorld* InGeneratedWorld)
{
	this->NewGeneratedWorld = InGeneratedWorld;
}

void AAstroGenerator::DisplayNewGeneratedWorld()
{
	this->NewGeneratedWorld->PrintAllValues();
}

void AAstroGenerator::ApplyWorldModel()
{
	if (!NewGeneratedWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("NewGeneratedWorld is nullptr"));
		return;
	}

	// Copy values from NewGeneratedWorld to AAstroGenerator
	bGenerateFullScaledWorld = NewGeneratedWorld->bGenerateFullScaledWorld;
	bGenerateHomeSystem = NewGeneratedWorld->bGenerateHomeSystem;
	bStartWithHomePlanet = NewGeneratedWorld->bStartWithHomePlanet;
	bRandomHomeSystem = NewGeneratedWorld->bRandomHomeSystem;
	bRandomHomeSystemType = NewGeneratedWorld->bRandomHomeSystemType;
	bRandomHomeStar = NewGeneratedWorld->bRandomHomeStar;
	bRandomStartPlanetNumber = NewGeneratedWorld->bRandomStartPlanetNumber;
	AstroGenerationLevel = NewGeneratedWorld->AstroGenerationLevel;
	GalaxyType = NewGeneratedWorld->GalaxyType;
	GalaxyGlass = NewGeneratedWorld->GalaxyClass;
	StarClusterSize = NewGeneratedWorld->StarClusterSize;
	StarClusterType = NewGeneratedWorld->StarClusterType;
	StarClusterPopulation = NewGeneratedWorld->StarClusterPopulation;
	StarClusterComposition = NewGeneratedWorld->StarClusterComposition;
	HomeSystemStarType = NewGeneratedWorld->StarType;
	HomeStarStellarType = NewGeneratedWorld->StellarType;
	HomeStarSpectralClass = NewGeneratedWorld->SpectralClass;
	HomeSystemPlanetaryType = NewGeneratedWorld->PlanetarySystemType;
	HomeSystemOrbitDistributionType = NewGeneratedWorld->OrbitDistributionType;
	HomeSystemPosition = NewGeneratedWorld->HomeSystemPosition;
	GalaxySize = NewGeneratedWorld->GalaxySize;
	GalaxyStarCount = NewGeneratedWorld->GalaxyStarCount;
	PlanetsAmount = NewGeneratedWorld->PlanetsAmount;
	StartPlanetNumber = NewGeneratedWorld->StartPlanetIndex;
	GalaxyStarDensity = NewGeneratedWorld->GalaxyStarDensity;
	HomePlanetarySystem = NewGeneratedWorld->HomePlanetarySystem;
	HomePlanet = NewGeneratedWorld->HomePlanet;
	//PlanetRadius = NewGeneratedWorld->PlanetRadius;
	//MoonsAmount = NewGeneratedWorld->MoonsAmount;
}

void AAstroGenerator::GenerateWorldByModel()
{
	InitAstroGenerators();

	ApplyWorldModel();

	GenerateStarCluster();
}

void AAstroGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoGeneration)
	{
		InitAstroGenerators();

		InitGenerationLevel();
	}
}

void AAstroGenerator::Test_GenerateFullscaled()
{
	// Задаем параметры звезд
	const double MinStarRadius = 0.1f;
	const double MaxStarRadius = 10.0f;
	const int StarCount = 100;

	// Создаем актор-якорь для нашего мира
	AAstroAnchor* AstroAnchor = GetWorld()->SpawnActor<AAstroAnchor>(BP_AstroAnchorClass);

	// Создаем актор кластера звезд и прикрепляем его к якорю
	AStarCluster* StarCluster = GetWorld()->SpawnActor<AStarCluster>(BP_StarClusterClass);
	StarCluster->AttachToActor(AstroAnchor, FAttachmentTransformRules::KeepRelativeTransform);

	// Создаем кластер звезд
	for (int i = 0; i < StarCount; ++i)
	{
		// Генерируем случайные координаты и радиус для звезды
		FVector StarPosition = FMath::VRand() * FMath::FRand() * 5.0f;
		double StarRadius = FMath::RandRange(MinStarRadius, MaxStarRadius);

		// Создаем преобразование для звезды
		FTransform StarTransform;
		StarTransform.SetLocation(StarPosition * 10000);
		StarTransform.SetScale3D(FVector(StarRadius));

		// Добавляем инстанс звезды в HISM
		StarCluster->StarMeshInstances->AddInstance(StarTransform);
	}

	// Запоминаем актор-якорь и актор кластера в классе генератора
	GeneratedWorld = AstroAnchor;
	GeneratedStarCluster = StarCluster;
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
		GenerateStarSystem();
		break;
	case EAstroGenerationLevel::PlanetSystem:
		GeneratePlanetSystem();
		break;
	case EAstroGenerationLevel::SinglePlanet:
		GenerateSinglePlanet();
		break;
	/*case EAstroGenerationLevel::Random:
		GenerateRandomWorld();
		break;*/
	default:
		GenerateRandomWorld();
		break;
	}

	if (bGenerateHomeSystem)
	{
		GenerateHomeStarSystem();
	}
}

void AAstroGenerator::GenerateGalaxiesCluster()
{
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

void AAstroGenerator::GeneratePlanetSystem()
{
}

void AAstroGenerator::GenerateSinglePlanet()
{
}

void AAstroGenerator::GenerateRandomWorld()
{
}

void AAstroGenerator::InitAstroGenerators()
{
	// Init generators
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

void AAstroGenerator::GenerateHomeStarSystem()
{
	// TODO: Refactoring - GenerateStarSystem(StarSystemModel);

	if (bRandomHomeSystem)
	{
		// Random Model
		// GenerateHomeSystem(RandomHomeSystem)
		GenerateStarSystem();
	}
	else
	{
		// Custom Model from Editor
		// GenerateHomeSystem(CustomHomeSystem)
		GenerateCustomHomeSystem();
	}

	if (GeneratedHomeStarSystem
		&& GeneratedHomeStarSystem->MainStar
		&& GeneratedHomeStarSystem->MainStar->PlanetarySystem)
	{
		TArray<TSharedPtr<FPlanetData>> PlanetDataMap = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetsList;

		UE_LOG(LogTemp, Warning, TEXT("Planet List - "));
		UE_LOG(LogTemp, Warning, TEXT("Planet Amount: %d"), PlanetDataMap.Num());

		/// ShowPlanetList();
		for (const TSharedPtr<FPlanetData>& PlanetDataPtr : PlanetDataMap)
		{
			if (PlanetDataPtr.IsValid())
			{
				FPlanetData PlanetData = *(PlanetDataPtr.Get());
				UE_LOG(LogTemp, Warning, TEXT("    Planet Order: %d"), PlanetData.PlanetOrder);
				UE_LOG(LogTemp, Warning, TEXT("     Orbit Radius: %f"), PlanetData.OrbitRadius);

				// Получаем модель планеты
				TSharedPtr<FPlanetModel> PlanetModel = PlanetData.PlanetModel;

				// Вывод данных модели планеты
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

					// Вывод информации о спутниках
					TArray<TSharedPtr<FMoonData>> MoonsList = PlanetModel->MoonsList;
					for (int32 i = 0; i < MoonsList.Num(); i++)
					{
						if (MoonsList[i].IsValid())
						{
							// Замените FMoonData на структуру данных вашего спутника
							FMoonData MoonData = *(MoonsList[i].Get());

							// Выводим данные спутника
							UE_LOG(LogTemp, Warning, TEXT("         Moon Order: %d"), MoonData.MoonOrder);
							UE_LOG(LogTemp, Warning, TEXT("             Moon Orbit Radius: %f"), MoonData.OrbitRadius);

							// Получаем модель Moon
							TSharedPtr<FMoonModel> MoonModel = MoonData.MoonModel;

							// Вывод данных модели Moon
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

		if (bStartWithHomePlanet)
		{
			// set HomePlanet from Editor 
			// Disable all Generated object
			// Place and attach HomePlanet To Planet Location
			// Move world to char location become 000

			if (StartHomePlanet)
			{
				GEngine->AddOnScreenDebugMessage(-1, 65.f, FColor::Red,
				                                 FString::Printf(
					                                 TEXT("StartHomePlanet: %s"), *StartHomePlanet->GetName()));

				int StartPlanetIndex = bRandomStartPlanetNumber
					                       ? FMath::RandRange(0, PlanetsAmount - 1)
					                       : StartPlanetNumber - 1;
				if (StartPlanetIndex >= PlanetsAmount || StartPlanetIndex < 0)
				{
					StartPlanetIndex = PlanetsAmount - 1;
				}

				if (GeneratedHomeStarSystem)
				{
					//APlanet* NewHomePlanet = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetsActorsList[StartPlanetIndex]; /// To method
					APlanetOrbit* NewHomePlanetOrbit = GeneratedHomeStarSystem->MainStar->PlanetarySystem->
						PlanetOrbitsList[StartPlanetIndex]; /// To method

					if (NewHomePlanetOrbit && NewHomePlanetOrbit->Planet)
					{
						// Шаг 1: Получаем локальную позицию текущей планеты.
						FVector OldPlanetLocalPosition = NewHomePlanetOrbit->Planet->GetActorLocation() -
							NewHomePlanetOrbit->GetActorLocation();

						// Шаг 2: Удаляем всех дочерних акторов орбиты.
						TArray<AActor*> AttachedOrbitalActors;
						NewHomePlanetOrbit->GetAttachedActors(AttachedOrbitalActors);

						for (AActor* Actor : AttachedOrbitalActors)
						{
							if (Actor)
							{
								Actor->Destroy();
							}
						}

						// Шаг 3: Прикрепляем вашу планету из редактора к актору орбиты. Допустим, это PlanetFromEditor.
						if (HomePlanet)
						{
							HomePlanet->AttachToActor(NewHomePlanetOrbit,
							                          FAttachmentTransformRules::KeepWorldTransform);

							// Шаг 4: Устанавливаем локальную позицию для вашей планеты на основе сохраненного смещения.
							HomePlanet->SetActorLocation(
								NewHomePlanetOrbit->GetActorLocation() + OldPlanetLocalPosition);
						}

						/// Move system to 000
						APawn* PlayerCharacter = Cast<APawn>(UGameplayStatics::GetPlayerPawn(this, 0));
						if (PlayerCharacter)
						{
							// Attach char to Start Station
							TArray<AActor*> AttachedActors;
							StartHomePlanet->GetAttachedActors(AttachedActors);
							for (AActor* Actor : AttachedActors)
							{
								if (Actor)
								{
									// Пробуем привести актора к типу ASpaceHeadquarters
									ASpaceHeadquarters* Headquarters = Cast<ASpaceHeadquarters>(Actor);

									if (Headquarters)
									{
										// Получаем текущее положение станции
										FVector HeadquartersLocation = Headquarters->GetActorLocation();
										// Получаем текущее положение AstroGenerator
										FVector GeneratorLocation = this->GetActorLocation();
										// Вычисляем новое положение для AstroGenerator
										FVector NewGeneratorLocation = GeneratorLocation - HeadquartersLocation;
										// Устанавливаем новое положение для AstroGenerator
										this->SetActorLocation(NewGeneratorLocation, false);
										// Устанавливаем положение игрока на (0,0,0)
										Headquarters->SetActorLocation(FVector(0, 0, 0), false);

										/// ApplyAstroParamsToPlanet
										////HomePlanet->ApplyNewPlanetParameters(StartHomePlanet);

										FVector StartHeadquartersLocation = Headquarters->GetStartPointPosition();
										PlayerCharacter->SetActorLocation(StartHeadquartersLocation);
										break;
									}
								}
							}
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("HomePlanetarySystem is null!!!"));
				}
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 65.f, FColor::Red, TEXT("StartHomePlanet is null"));
			}
		}
		else if (PlanetDataMap.Num() > 0)
		{
			TSharedPtr<FPlanetData> StartPlanetData = PlanetDataMap[0];
			TSharedPtr<FPlanetModel> StartPlanetModel = StartPlanetData->PlanetModel;
			int HomePlanetIndex = StartPlanetData->PlanetOrder;

			// Спавн станции
			UWorld* World = GetWorld();
			if (World)
			{
				HomePlanet = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetsActorsList[HomePlanetIndex];

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = HomePlanet;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				double StationOrbitHeight = PlanetGenerator->CalculateOrbitHeight(
					HomeSpaceStationOrbitHeight, StartPlanetModel->Radius);
				FVector PlanetPosition = HomePlanet->GetActorLocation();

				HomeSpaceHeadquarters = World->SpawnActor<ASpaceHeadquarters>(
					BP_HomeSpaceHeadquarters, PlanetPosition, FRotator::ZeroRotator);
				HomeSpaceHeadquarters->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform);
				/// CRASH PIE!!!
				HomeSpaceHeadquarters->SetActorRelativeRotation(FRotator(0, 0, 0));
				const double EARTH_RADIUS_CM = 637100000.0;
				FVector Offset = FVector(0, 0, StationOrbitHeight * EARTH_RADIUS_CM);
				HomeSpaceHeadquarters->AddActorWorldOffset(Offset);

				FVector HomeSpaceHeadquartersLocation = HomeSpaceHeadquarters->GetActorLocation();
				FRotator HomeSpaceHeadquartersRotation = HomeSpaceHeadquarters->GetActorRotation();
				HomeSpaceStation = World->SpawnActor<ASpaceStation>(BP_HomeSpaceStation, HomeSpaceHeadquartersLocation,
				                                                    HomeSpaceHeadquartersRotation, SpawnParams);
				HomeSpaceStation->AttachToActor(HomeSpaceHeadquarters, FAttachmentTransformRules::KeepWorldTransform);
				double HomeStationOffset = HomeSpaceStation->GravityCollisionZone->GetScaledSphereRadius() * 2;
				HomeSpaceStation->AddActorLocalOffset(FVector(0, HomeStationOffset, 0));
				HomeSpaceStation->CalculateAffectionRadius();

				// Spawn HomeShipyard
				HomeSpaceShipyard = World->SpawnActor<ASpaceShipyard>(BP_HomeSpaceShipyard,
				                                                      HomeSpaceHeadquartersLocation,
				                                                      HomeSpaceHeadquartersRotation);
				double HomeSpaceShipyardLocationOffset = HomeSpaceShipyard->GravityCollisionZone->
				                                                            GetScaledSphereRadius() * 2;
				HomeSpaceShipyard->AddActorLocalOffset(FVector(0, -HomeSpaceShipyardLocationOffset, 0));
				HomeSpaceShipyard->AttachToActor(HomeSpaceHeadquarters, FAttachmentTransformRules::KeepWorldTransform);
				HomeSpaceShipyard->CalculateAffectionRadius();

				//Spawn HomeSpaceship
				FActorSpawnParameters SpaceshipSpawnParams;
				SpaceshipSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				FVector HomeSpaceshipLocation = HomeSpaceShipyard->SpawnPoint->GetComponentLocation();

				HomeSpaceshipLocation.Z += 1000;
				ASpaceship* NewHomeSpaceship = World->SpawnActor<ASpaceship>(
					BP_HomeSpaceship, HomeSpaceshipLocation, HomeSpaceShipyard->GetActorRotation(),
					SpaceshipSpawnParams);

				if (NewHomeSpaceship && GeneratedHomeStarSystem)
				{
					NewHomeSpaceship->AttachToActor(HomeSpaceShipyard, FAttachmentTransformRules::KeepWorldTransform);
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

				FVector CharSpawnLocation{0};
				APawn* PlayerCharacter = UGameplayStatics::GetPlayerPawn(World, 0);
				if (PlayerCharacter)
				{
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

					UE_LOG(LogTemp, Warning, TEXT("CharSpawnLocation: %s"), *CharSpawnLocation.ToString());
					bool bTeleportSucces = PlayerCharacter->SetActorLocation(CharSpawnLocation, false);
					UE_LOG(LogTemp, Warning, TEXT("Teleport success: %s"),
					       bTeleportSucces ? TEXT("True") : TEXT("False"));

					// relocate char to 000
					{
						// Получаем текущее положение игрока
						FVector PlayerLocation = PlayerCharacter->GetActorLocation();
						// Получаем текущее положение AstroGenerator
						FVector GeneratorLocation = this->GetActorLocation();
						// Вычисляем новое положение для AstroGenerator
						FVector NewGeneratorLocation = GeneratorLocation - PlayerLocation;
						// Устанавливаем новое положение для AstroGenerator
						this->SetActorLocation(NewGeneratorLocation, false);
						// Устанавливаем положение игрока на (0,0,0)
						PlayerCharacter->SetActorLocation(FVector(0, 0, 0), false);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter is null!"));
				}
			}
		}
	}
}

// TODO: Refactoring - GenerateStarSystem(StarSystemModel);
void AAstroGenerator::GenerateStarSystem()
{
	if (StarGenerator == nullptr || PlanetGenerator == nullptr || MoonGenerator == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
		return;
	}

	if (!BP_PlanetarySystemClass || !BP_StarSystemClass || !BP_StarClass || !BP_PlanetClass || !BP_MoonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the blueprint classes is not assigned in the editor!"));
		return;
	}

	UWorld* World = GetWorld();

	if (World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Generate Star System!"));

		FTransform HomeSystemTransform;

		FVector HomeSystemSpawnLocation{0};
		switch (HomeSystemPosition)
		{
		case EHomeSystemPosition::WorldCenter:
			HomeSystemSpawnLocation = FVector(0, 0, 0);
			break;
		case EHomeSystemPosition::RandomPosition:
			{
				// Задайте HomeSystemSpawnLocation в зависимости от вашего определения центра, середины и конца.
				double RandomX = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomY = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomZ = FMath::RandRange(-1000000000000, 1000000000000);
				HomeSystemSpawnLocation = FVector(RandomX, RandomY, RandomZ); //* 1000000000;
			}
			break;
		case EHomeSystemPosition::DirectPosition:
			{
				TArray<AActor*> AttachedActors;
				GetAttachedActors(AttachedActors);
				if (AttachedActors.Num() > 0) // Убедитесь, что есть прикрепленные акторы
				{
					int32 RandomIndex = FMath::RandRange(0, AttachedActors.Num() - 1); // Получите случайный индекс
					if (AstroGenerationLevel == EAstroGenerationLevel::Galaxy)
					{
						AGalaxy* GalaxyActor = Cast<AGalaxy>(AttachedActors[RandomIndex]);
						// Приведите актора к типу AGalaxy
						if (GalaxyActor)
						{
							// Если актор является экземпляром класса AGalaxy, извлеките его HISM компонент.
							UHierarchicalInstancedStaticMeshComponent* HISMComponent = GalaxyActor->StarMeshInstances;
							if (HISMComponent && HISMComponent->GetInstanceCount() > 0)
							{
								// Получите случайный индекс из диапазона доступных инстансов
								int32 RandomInstanceIndex = FMath::RandRange(0, HISMComponent->GetInstanceCount() - 1);
								// Извлеките трансформацию случайного инстанса
								FTransform InstanceTransform;
								HISMComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
								// Используйте позицию из трансформации инстанса как HomeSystemSpawnLocation
								HomeSystemSpawnLocation = InstanceTransform.GetLocation();
							}
						}
					}
					else if (AstroGenerationLevel == EAstroGenerationLevel::StarCluster)
					{
						AStarCluster* StarClusterActor = Cast<AStarCluster>(AttachedActors[RandomIndex]);
						// Приведите актора к типу AStarCluster
						if (StarClusterActor)
						{
							// Если актор является экземпляром класса AStarCluster, извлеките его HISM компонент.
							UHierarchicalInstancedStaticMeshComponent* HISMComponent = StarClusterActor->
								StarMeshInstances;

							if (HISMComponent && HISMComponent->GetInstanceCount() > 0)
							{
								// Получите случайный индекс из диапазона доступных инстансов
								int32 RandomInstanceIndex = FMath::RandRange(0, HISMComponent->GetInstanceCount() - 1);
								// Извлеките трансформацию случайного инстанса
								FTransform InstanceTransform;
								HISMComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
								// Используйте позицию из трансформации инстанса как HomeSystemSpawnLocation
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

		/*
		RandomPosition:
		get random index from hism array indexes
		get index model
		from hism index to star model map get random pair
		*/

		// Создаем новую звездную систему
		TSharedPtr<FStarSystemModel> StarSystemModel = MakeShared<FStarSystemModel>();
		StarSystemGenerator->GenerateRandomStarSystemModel(StarSystemModel);

		AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass, HomeSystemTransform);
		NewStarSystem->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		NewStarSystem->SetActorLocation(HomeSystemSpawnLocation);

		if (!NewStarSystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
			return;
		}
		StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

		// Generate astro model
		int AmountOfStars = StarSystemModel->AmountOfStars;
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
				StarGenerator->GenerateRandomStarModel(StarModel);
			}

			TSharedPtr<FPlanetarySystemModel> PlanetraySystemModel = MakeShared<FPlanetarySystemModel>();

			if (bRandomHomeSystemType)
			{
				PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(
					PlanetraySystemModel, StarModel, PlanetGenerator, MoonGenerator);
			}
			else
			{
				PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(
					PlanetraySystemModel, StarModel, PlanetGenerator, MoonGenerator);
				//PlanetarySystemGenerator->GenerateCustomPlanetarySystem();
			}

			AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass);
			HomePlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);
			//HomePlanetarySystem = NewPlanetarySystem;


			if (StarNumber == 0)
			{
				NewStarSystem->MainStar = NewStar;
			}

			if (!NewStar || !HomePlanetarySystem)
			{
				UE_LOG(LogTemp, Warning, TEXT("Star Falied!"));
				return;
			}

			// Set Star full-scale
			// TODO: PlanetarySystemGenerator->ConnectStar()
			StarGenerator->ApplyModel(NewStar, StarModel);
			PlanetarySystemGenerator->ApplyModel(HomePlanetarySystem, PlanetraySystemModel);
			NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
			NewStar->StarRadiusKM = StarModel->Radius * 696340;
			NewStar->SetPlanetarySystem(HomePlanetarySystem);
			NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
			HomePlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);

			// Apply material luminocity multiplier and emissive light color
			StarGenerator->ApplySpectralMaterial(NewStar, StarModel);

			// Генерация планет для каждой звезды
			FVector LastPlanetLocation{0};
			int AmountOfPlanets = PlanetraySystemModel->AmountOfPlanets;
			for (const TSharedPtr<FPlanetData> FPlanetData : PlanetraySystemModel->PlanetsList)
			{
				APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(
					BP_PlanetOrbitClass, HomePlanetarySystem->GetActorLocation(), FRotator::ZeroRotator);
				NewPlanetOrbit->AttachToActor(HomePlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);

				// Planet Model and generation
				TSharedPtr<FPlanetModel> PlanetModel = FPlanetData->PlanetModel;
				APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);

				PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
				NewStar->AddPlanet(NewPlanet);
				NewPlanet->SetParentStar(NewStar);

				// set planet full-scale
				NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
				FVector NewLocation = FVector(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
				NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
				NewPlanet->SetActorLocation(NewLocation);


				NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanetOrbit->SetActorRelativeRotation(
					FRotator(FMath::RandRange(-30.0, 30.0), FMath::RandRange(-360.0, 360.0), 0));
				HomePlanetarySystem->PlanetsActorsList.Add(NewPlanet);
				HomePlanetarySystem->PlanetOrbitsList.Add(NewPlanetOrbit);

				const double KM_TO_UE_UNIT_SCALE = 100000;
				double DiameterOfLastMoon = 0;
				FVector LastMoonLocation;
				// Generate Moons
				for (const TSharedPtr<FMoonData> MoonData : PlanetModel->MoonsList)
				{
					APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(
						BP_PlanetOrbitClass, NewPlanet->GetActorLocation(), FRotator::ZeroRotator);
					NewMoonOrbit->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);

					FVector MoonLocation = NewPlanet->GetActorLocation();
					AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
					NewPlanet->AddMoon(NewMoon);
					NewMoon->SetParentPlanet(NewPlanet);

					MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
					MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);

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
					NewMoonOrbit->SetActorRelativeRotation(FRotator(FMath::RandRange(-360.0, 360.0),
					                                                FMath::RandRange(-360.0, 360.0),
					                                                FMath::RandRange(-360.0, 360.0)));

					DiameterOfLastMoon = MoonRadius * 2;
					LastMoonLocation = NewMoon->GetActorLocation();

					/// Generate WSC
					//NewMoon->PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(World, NewMoon);
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

				if (true)
				{
					/// NewPlanet->PlanetEnvironmentGenerator->GeneratePlanetEnviroment();
					NewPlanet->OrbitHeight = (NewPlanet->GravityCollisionZone->GetScaledSphereRadius() / 100000) -
						NewPlanet->RadiusKM;
					NewPlanet->PlanetaryEnvironmentGenerator->InitAtmoScape(World, NewPlanet->RadiusKM, NewPlanet);
					if (NewPlanet->IsNotGasGiant())
					{
						NewPlanet->PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(World, NewPlanet);
					}
				}
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
			double SphereRadius = NewStarSystem->MainStar->PlanetarySystemZone->GetScaledSphereRadius() * 1.5;
			NewStarSystem->StarSystemRadius = SphereRadius;
			NewStarSystem->StarSystemZone->SetSphereRadius(SphereRadius);
		}

		if (bGenerateFullScaledWorld)
		{
			/* method 1: non-adaptive
				if center = spawn at 000
				if random = spawn at random index
				if zones = spawn in distance from center

				check overlapped hism, if overlapped, (re)move it
			*/

			double HomeSystemrRadiusScaled = NewStarSystem->StarSystemRadius * StarSystemDeadZone;
			FCollisionShape MySphere = FCollisionShape::MakeSphere(HomeSystemrRadiusScaled);
			TArray<FOverlapResult> Overlaps;

			// выполните проверку перекрытия
			FVector HomeSystemLocation = NewStarSystem->GetActorLocation();
			bool isOverlap = World->OverlapMultiByChannel(Overlaps, HomeSystemLocation, FQuat::Identity, ECC_Visibility,
			                                              MySphere);

			float DebugDuration = 60.0f;
			DrawDebugSphere(
				GetWorld(),
				HomeSystemLocation,
				HomeSystemrRadiusScaled,
				50,
				FColor::Purple,
				true,
				DebugDuration
			);

			TArray<int32> InstancesToRemove;
			UHierarchicalInstancedStaticMeshComponent* OverlappingHISM = nullptr;

			if (isOverlap)
			{
				int TotalOverlaps = Overlaps.Num();
				UE_LOG(LogTemp, Warning, TEXT("Total number of overlaps: %d"), TotalOverlaps);

				for (auto& Result : Overlaps)
				{
					UHierarchicalInstancedStaticMeshComponent* HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(
						Result.Component.Get());
					if (HISM)
					{
						OverlappingHISM = HISM;
						int32 InstanceIndex = Result.ItemIndex;
						InstancesToRemove.Add(InstanceIndex);
					}
				}
				// сортируем массив индексов в обратном порядке
				InstancesToRemove.Sort([](const int32& A, const int32& B) { return A > B; });
				for (auto Index : InstancesToRemove)
				{
					OverlappingHISM->RemoveInstance(Index);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No overlaped stars!"));
			}
		}


		/// TO HOME SYSTEM
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Falied to get World!"));
	}
}

void AAstroGenerator::GenerateCustomHomeSystem()
{
	if (StarGenerator == nullptr || PlanetGenerator == nullptr || MoonGenerator == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
		return;
	}

	if (!BP_PlanetarySystemClass || !BP_StarSystemClass || !BP_StarClass || !BP_PlanetClass || !BP_MoonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the blueprint classes is not assigned in the editor!"));
		return;
	}

	UWorld* World = GetWorld();

	if (World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Generate Star System!"));

		FTransform HomeSystemTransform;

		FVector HomeSystemSpawnLocation{0};
		switch (HomeSystemPosition)
		{
		case EHomeSystemPosition::WorldCenter:
			HomeSystemSpawnLocation = FVector(0, 0, 0);
			break;
		case EHomeSystemPosition::RandomPosition:
			{
				// Задайте HomeSystemSpawnLocation в зависимости от вашего определения центра, середины и конца.
				double RandomX = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomY = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomZ = FMath::RandRange(-1000000000000, 1000000000000);
				HomeSystemSpawnLocation = FVector(RandomX, RandomY, RandomZ); //* 1000000000;
			}
			break;
		case EHomeSystemPosition::DirectPosition:
			{
				TArray<AActor*> AttachedActors;
				GetAttachedActors(AttachedActors);
				if (AttachedActors.Num() > 0) // Убедитесь, что есть прикрепленные акторы
				{
					int32 RandomIndex = FMath::RandRange(0, AttachedActors.Num() - 1); // Получите случайный индекс
					if (AstroGenerationLevel == EAstroGenerationLevel::Galaxy)
					{
						AGalaxy* GalaxyActor = Cast<AGalaxy>(AttachedActors[RandomIndex]);
						// Приведите актора к типу AGalaxy
						if (GalaxyActor)
						{
							// Если актор является экземпляром класса AGalaxy, извлеките его HISM компонент.
							UHierarchicalInstancedStaticMeshComponent* HISMComponent = GalaxyActor->StarMeshInstances;
							if (HISMComponent && HISMComponent->GetInstanceCount() > 0)
							{
								// Получите случайный индекс из диапазона доступных инстансов
								int32 RandomInstanceIndex = FMath::RandRange(0, HISMComponent->GetInstanceCount() - 1);
								// Извлеките трансформацию случайного инстанса
								FTransform InstanceTransform;
								HISMComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
								// Используйте позицию из трансформации инстанса как HomeSystemSpawnLocation
								HomeSystemSpawnLocation = InstanceTransform.GetLocation();
							}
						}
					}
					else if (AstroGenerationLevel == EAstroGenerationLevel::StarCluster)
					{
						AStarCluster* StarClusterActor = Cast<AStarCluster>(AttachedActors[RandomIndex]);
						// Приведите актора к типу AStarCluster
						if (StarClusterActor)
						{
							// Если актор является экземпляром класса AStarCluster, извлеките его HISM компонент.
							UHierarchicalInstancedStaticMeshComponent* HISMComponent = StarClusterActor->
								StarMeshInstances;

							if (HISMComponent && HISMComponent->GetInstanceCount() > 0)
							{
								// Получите случайный индекс из диапазона доступных инстансов
								int32 RandomInstanceIndex = FMath::RandRange(0, HISMComponent->GetInstanceCount() - 1);
								// Извлеките трансформацию случайного инстанса
								FTransform InstanceTransform;
								HISMComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
								// Используйте позицию из трансформации инстанса как HomeSystemSpawnLocation
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
		/*
		RandomPosition:
		get random index from hism array indexes
		get index model
		from hism index to star model map get random pair

		*/

		// Создаем новую звездную систему
		TSharedPtr<FStarSystemModel> StarSystemModel = MakeShared<FStarSystemModel>();

		int AmountOfStars{0};
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

			TSharedPtr<FPlanetarySystemModel> PlanetraySystemModel = MakeShared<FPlanetarySystemModel>();

			if (bRandomHomeSystemType)
			{
				PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(
					PlanetraySystemModel, StarModel, PlanetGenerator, MoonGenerator);
			}
			else
			{
				PlanetraySystemModel->AmountOfPlanets = PlanetsAmount;
				PlanetraySystemModel->PlanetarySystemType = HomeSystemPlanetaryType;
				PlanetraySystemModel->OrbitDistributionType = HomeSystemOrbitDistributionType;
				PlanetarySystemGenerator->GenerateCustomPlanetraySystemModel(
					PlanetraySystemModel, StarModel, PlanetGenerator, MoonGenerator);
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
			PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);
			NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
			NewStar->StarRadiusKM = StarModel->Radius * 696340;
			NewStar->SetPlanetarySystem(NewPlanetarySystem);
			NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);

			// Apply material luminocity multiplier and emissive light color
			StarGenerator->ApplySpectralMaterial(NewStar, StarModel);

			// Генерация планет для каждой звезды
			FVector LastPlanetLocation{0};
			int AmountOfPlanets = PlanetraySystemModel->AmountOfPlanets;
			for (const TSharedPtr<FPlanetData> FPlanetData : PlanetraySystemModel->PlanetsList)
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

				// set planet full-scale
				NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
				FVector NewLocation = FVector(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
				NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
				NewPlanet->SetActorLocation(NewLocation);

				NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanetarySystem->PlanetsActorsList.Add(NewPlanet);

				NewPlanetOrbit->Planet = NewPlanet;

				const double KM_TO_UE_UNIT_SCALE = 100000;
				double DiameterOfLastMoon = 0;
				FVector LastMoonLocation;
				// Generate Moons
				for (const TSharedPtr<FMoonData> MoonData : PlanetModel->MoonsList)
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

					/// Generate WSC
					NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(World, NewMoon->RadiusKM, NewMoon);
					//NewMoon->PlanetaryEnvironmentGenerator->GenerateWorldscapeSurfaceByModel(World, NewMoon);
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

				if (true)
				{
					NewPlanet->OrbitHeight = (NewPlanet->GravityCollisionZone->GetScaledSphereRadius() / 100000) -
						NewPlanet->RadiusKM;

					/// TODO: PlanetaryEnvironmentGenerator->InitEnviroment();
					NewPlanet->PlanetaryEnvironmentGenerator->InitEnviroment(NewPlanet, World);
				}
			}

			/// Place Orbits
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

						// Мы сдвигаем её на величину AffectionRadiusKM текущей планеты
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
					{
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
			}

			if (bNeedOrbitRotation)
			{
				for (APlanetOrbit* PlanetOrbit : NewPlanetarySystem->PlanetOrbitsList)
				{
					float RandomZRotation = FMath::RandRange(-360.0f, 360.0f);
					float RandomYRotation = FMath::RandRange(-15.0f, 15.0f);
					FRotator NewRotation = FRotator(RandomYRotation, RandomZRotation, 0);
					PlanetOrbit->AddActorLocalRotation(NewRotation);

					TArray<AActor*> AttachedActors;
					PlanetOrbit->GetAttachedActors(AttachedActors);

					for (AActor* AttachedActor : AttachedActors)
					{
						APlanet* Planet = Cast<APlanet>(AttachedActor);
						if (Planet)
						{
							// Iterate through the list of moons for this planet.
							for (APlanetOrbit* MoonOrbit : Planet->MoonOrbitsList)
							{
								float RandomXRotationMoon = FMath::RandRange(-360.0f, 360.0f);
								float RandomYRotationMoon = FMath::RandRange(-360.0f, 360.0f);
								float RandomZRotationMoon = FMath::RandRange(-360.0f, 360.0f);
								FRotator NewRotationMoon = FRotator(RandomXRotationMoon, RandomYRotationMoon,
								                                    RandomZRotationMoon);
								MoonOrbit->AddActorLocalRotation(NewRotationMoon);
							}
						}
					}
				}
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
			/* method 1: non-adaptive
				if center = spawn at 000
				if random = spawn at random index
				if zones = spawn in distance from center

				check overlapped hism, if overlapped, (re)move it
			*/

			double HomeSystemrRadiusScaled = NewStarSystem->StarSystemRadius * StarSystemDeadZone;
			FCollisionShape MySphere = FCollisionShape::MakeSphere(HomeSystemrRadiusScaled);
			TArray<FOverlapResult> Overlaps;

			// выполните проверку перекрытия
			FVector HomeSystemLocation = NewStarSystem->GetActorLocation();
			bool isOverlap = World->OverlapMultiByChannel(Overlaps, HomeSystemLocation, FQuat::Identity, ECC_Visibility,
			                                              MySphere);

			float DebugDuration = 60.0f;
			/*DrawDebugSphere(
				GetWorld(),
				HomeSystemLocation,
				HomeSystemrRadiusScaled,
				50,
				FColor::Purple,
				true,
				DebugDuration
			);*/

			TArray<int32> InstancesToRemove;
			UHierarchicalInstancedStaticMeshComponent* OverlappingHISM = nullptr;

			if (isOverlap)
			{
				int TotalOverlaps = Overlaps.Num();
				UE_LOG(LogTemp, Warning, TEXT("Total number of overlaps: %d"), TotalOverlaps);

				for (auto& Result : Overlaps)
				{
					UHierarchicalInstancedStaticMeshComponent* HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(
						Result.Component.Get());
					if (HISM)
					{
						OverlappingHISM = HISM;
						int32 InstanceIndex = Result.ItemIndex;
						InstancesToRemove.Add(InstanceIndex);
					}
				}
				// сортируем массив индексов в обратном порядке
				InstancesToRemove.Sort([](const int32& A, const int32& B) { return A > B; });
				for (auto Index : InstancesToRemove)
				{
					OverlappingHISM->RemoveInstance(Index);
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


TMap<EStarClusterType, TPair<int, int>> ClusterStarAmount =
{
	{EStarClusterType::OpenCluster, {500, 5000}},
	{EStarClusterType::GlobularCluster, {5000, 25000}},
	{EStarClusterType::Supercluster, {25000, 50000}},
	{EStarClusterType::Nebula, {10000, 20000}},
	{EStarClusterType::Unknown, {0, 0}}
};

int AAstroGenerator::GetRandomValueFromStarAmountRange(EStarClusterType ClusterType)
{
	if (ClusterStarAmount.Contains(ClusterType))
	{
		TPair<int, int> Range = ClusterStarAmount[ClusterType];
		return FMath::RandRange(Range.Key, Range.Value);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cluster type not found in map, or range is invalid"));
		return 0;
	}
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
		// Создаем модель звезды
		TSharedPtr<FStarModel> NewStarModel = MakeShared<FStarModel>();

		if (bGenerateRandomCluster)
		{
			StarGenerator->GenerateRandomStarModel(NewStarModel);
		}
		else
		{
			StarGenerator->GenerateStarModelByProbability(NewStarModel, StarClusterModel);
		}

		// Позиционируем звезду в кластере
		FVector StarPosition = StarClusterGenerator->CalculateStarPosition(i, NewStarCluster, NewStarModel);
		NewStarCluster->AddStarToClusterModel(StarPosition, NewStarModel);
		NewStarModel->Location = StarPosition;

		// Создаем инстанс звезды и добавляем его в HISM компонент
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
		SetActorScale3D(FVector(1000000000, 1000000000, 1000000000));
	}
}
