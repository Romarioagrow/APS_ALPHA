// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedWorld.h"
#include "StarSystem.h"
#include "MoonGenerationModel.h"

// Sets default values
AGeneratedWorld::AGeneratedWorld()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AGeneratedWorld::BeginPlay()
{
	Super::BeginPlay();

    // Init generators
    StarSystemGenerator = NewObject<UStarSystemGenerator>();
    StarGenerator = NewObject<UStarGenerator>();
    PlanetGenerator = NewObject<UPlanetGenerator>();
    MoonGenerator = NewObject<UMoonGenerator>();

	GenerateRandomStarSystem();
}


void AGeneratedWorld::GenerateRandomStarSystem()
{
    if (StarGenerator == nullptr || PlanetGenerator == nullptr || MoonGenerator == nullptr) {
        UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
        return;
    }
    
    if (!BP_PlanetarySystemClass || !BP_StarSystemClass || !BP_StarClass || !BP_PlanetClass || !BP_MoonClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("One of the blueprint classes is not assigned in the editor!"));
        return;
    }

    UWorld* World = GetWorld();
    

    if (World) // Проверяем, существует ли мир
    {
        FStarSystemGenerationModel StarSystemModel = StarSystemGenerator->GenerateRandomStarSystemModel();
        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass); // Создаем новую звездную систему
        if (!NewStarSystem) // Проверяем, успешно ли создана звездная система
        {
            // Обрабатываем ошибку
            UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
            return; // Завершаем выполнение функции, если не можем создать звездную систему
        }
        /// StarSystemGenerator->ApplyModel();
        int AmountOfStars = StarSystemModel.AmountOfStars;
        NewStarSystem->SetStarsAmount(AmountOfStars);

        // Генерация звезд для каждой планетарной системы
        for (int i = 0; i < AmountOfStars; i++)
        {
            FStarGenerationModel StarModel = StarGenerator->GenerateRandomStarModel();
            FPlanetarySystemGenerationModel PlanetraySystemModel = GenerateRandomPlanetraySystemModel(); // Add StarModel and calculate based on it  

            // Create Planetray System
            APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);
            if (!NewPlanetarySystem) // Проверяем, успешно ли создана планетарная система
            {
                // Обрабатываем ошибку
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // Завершаем выполнение функции, если не можем создать звездную систему

            }
            /// PlanetarySystemGenerator->ApplyModel();


            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); //StarGenerator->GenerateStar(StarModel);
            if (!NewPlanetarySystem) // Проверяем, успешно ли создана планетарная система
            {
                // Обрабатываем ошибку
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // Завершаем выполнение функции, если не можем создать звездную систему

            }
            /// StarGenerator->ApplyModel();
            NewPlanetarySystem->SetStar(NewStar);
            // Прикрепить NewStar к NewPlanetarySystem
            NewStar->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepRelativeTransform);
            UE_LOG(LogTemp, Warning, TEXT("Star is attached to PlanetarySystem"));

            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
            // Генерация планет для каждой звезды
            for (int j = 0; j < AmountOfPlanets; j++)
            {
                FPlanetGenerationModel PlanetModel = PlanetGenerator->GenerateRandomPlanetModel();
                APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass); //PlanetGenerator->GeneratePlanet(PlanetModel);

                if (!NewPlanetarySystem) // Проверяем, успешно ли создана планетарная система
                {
                    // Обрабатываем ошибку
                    UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                    return; // Завершаем выполнение функции, если не можем создать звездную систему

                }

                /// PlanetGenerator->ApplyModel();
                NewStar->AddPlanet(NewPlanet);
                // Прикрепить NewPlanet к NewStar
                NewPlanet->AttachToActor(NewStar, FAttachmentTransformRules::KeepRelativeTransform);
                UE_LOG(LogTemp, Warning, TEXT("Planet is attached to Star"));

                // Generate Moons
                FMoonGenerationModel MoonModel = MoonGenerator->GenerateRandomMoonModel();
                int AmountOfMoons = PlanetModel.AmountOfMoons;

                for (int k = 0; k < 3; k++)
                {
                    AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass); //MoonGenerator->GenerateMoon(MoonModel);
                    if (!NewMoon) // Проверяем, успешно ли создана планетарная система
                    {
                        // Обрабатываем ошибку
                        UE_LOG(LogTemp, Warning, TEXT("NewMoon Falied!"));
                        return; // Завершаем выполнение функции, если не можем создать звездную систему

                    }
                    /// MoonGenerator->ApplyModel();

                    NewPlanet->AddMoon(NewMoon);
                    // Прикрепить NewMoon к NewPlanet
                    NewMoon->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepRelativeTransform);
                    UE_LOG(LogTemp, Warning, TEXT("Moon is attached to Planet"));
                }

            }
            NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
            // Прикрепить NewPlanetarySystem к NewStarSystem
            NewPlanetarySystem->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepRelativeTransform);
            UE_LOG(LogTemp, Warning, TEXT("PlanetarySystem is attached to StarSystem"));
        }
    }
}

FPlanetarySystemGenerationModel AGeneratedWorld::GenerateRandomPlanetraySystemModel()
{
        FPlanetarySystemGenerationModel PlanetarySystemModel;
    PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(1, 10);
    PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.1, 10.0);
    return PlanetarySystemModel;
}


