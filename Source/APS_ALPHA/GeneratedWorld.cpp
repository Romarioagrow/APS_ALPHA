// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedWorld.h"
#include "StarSystem.h"

// Sets default values
AGeneratedWorld::AGeneratedWorld()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	StarSystemGenerator = NewObject<UStarSystemGenerator>();
	StarGenerator = NewObject<UStarGenerator>();
	PlanetGenerator = NewObject<UPlanetGenerator>();
	MoonGenerator = NewObject<UMoonGenerator>();
	

}

// Called when the game starts or when spawned
void AGeneratedWorld::BeginPlay()
{
	Super::BeginPlay();

	GenerateRandomStarSystem();

	// Generate Home start Star System
	//FStarSystemGenerationModel StarSystemModel = StarSystemGenerator->GenerateRandomStarSystemModel();
	//UWorld* World = GetWorld();
	//StarSystemGenerator->GenerateHomeStarSystem(StarSystemModel, World);

}

//FPlanetarySystemGenerationModel AGeneratedWorld::GenerateRandomPlanetraySystemModel()
//{
//    FPlanetarySystemGenerationModel PlanetarySystemModel;
//    PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(1, 10);
//    PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.1, 10.0);
//    return PlanetarySystemModel;
//}

void AGeneratedWorld::GenerateRandomStarSystem()
{
	FStarSystemGenerationModel StarSystemModel = StarSystemGenerator->GenerateRandomStarSystemModel();

    UWorld* World = GetWorld();

    /*if (!StarGenerator || !PlanetGenerator || !MoonGenerator) {
        UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
        return;
    }*/


    if (World) // Проверяем, существует ли мир
    {
        /*StarGenerator = CreateDefaultSubobject<UStarGenerator>(TEXT("UStarGenerator"));
        PlanetGenerator = CreateDefaultSubobject<UPlanetGenerator>(TEXT("UPlanetGenerator"));
        MoonGenerator = CreateDefaultSubobject<UMoonGenerator>(TEXT("UMoonGenerator"));*/

        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(); // Создаем новую звездную систему
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
            APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>();
            if (!NewPlanetarySystem) // Проверяем, успешно ли создана планетарная система
            {
                // Обрабатываем ошибку
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // Завершаем выполнение функции, если не можем создать звездную систему

            }
            /// PlanetarySystemGenerator->ApplyModel();


            AStar* NewStar = World->SpawnActor<AStar>(); //StarGenerator->GenerateStar(StarModel);
            if (!NewPlanetarySystem) // Проверяем, успешно ли создана планетарная система
            {
                // Обрабатываем ошибку
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // Завершаем выполнение функции, если не можем создать звездную систему

            }
            /// StarGenerator->ApplyModel();
            NewPlanetarySystem->SetStar(NewStar);
            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;

            // Генерация планет для каждой звезды
            for (int j = 0; j < AmountOfPlanets; j++)
            {
                //FPlanetGenerationModel PlanetModel = PlanetGenerator->GenerateRandomPlanetModel();
                APlanet* NewPlanet = World->SpawnActor<APlanet>(); //PlanetGenerator->GeneratePlanet(PlanetModel);

                if (!NewPlanetarySystem) // Проверяем, успешно ли создана планетарная система
                {
                    // Обрабатываем ошибку
                    UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                    return; // Завершаем выполнение функции, если не можем создать звездную систему

                }

                /// PlanetGenerator->ApplyModel();
                NewStar->AddPlanet(NewPlanet);


                // Generate Moons
                //FMoonGenerationModel MoonModel = MoonGenerator->GenerateRandomMoonModel();
                //int AmountOfMoons = PlanetModel.AmountOfMoons;

                for (int k = 0; k < 3; k++)
                {
                    AMoon* NewMoon = World->SpawnActor<AMoon>(); //MoonGenerator->GenerateMoon(MoonModel);
                    if (!NewMoon) // Проверяем, успешно ли создана планетарная система
                    {
                        // Обрабатываем ошибку
                        UE_LOG(LogTemp, Warning, TEXT("NewMoon Falied!"));
                        return; // Завершаем выполнение функции, если не можем создать звездную систему

                    }
                    /// MoonGenerator->ApplyModel();
                    NewPlanet->AddMoon(NewMoon);
                }

            }
            NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
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


