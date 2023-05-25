// Fill out your copyright notice in the Description page of Project Settings.


#include "GeneratedWorld.h"
#include "StarSystem.h"
#include "MoonGenerationModel.h"

// Sets default values
AStarClusterGenerator::AStarClusterGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AStarClusterGenerator::BeginPlay()
{
	Super::BeginPlay();

    // Init generators
    StarSystemGenerator = NewObject<UStarSystemGenerator>();
    PlanetarySystemGenerator = NewObject<UPlanetarySystemGenerator>();
    StarGenerator = NewObject<UStarGenerator>();
    PlanetGenerator = NewObject<UPlanetGenerator>();
    MoonGenerator = NewObject<UMoonGenerator>();

	GenerateRandomStarSystem();
}


void AStarClusterGenerator::GenerateRandomStarSystem()
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
        // Создаем новую звездную систему
        FStarSystemGenerationModel StarSystemModel = StarSystemGenerator->GenerateRandomStarSystemModel();
        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass); 
        if (!NewStarSystem) 
        {
            UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
            return; // Завершаем выполнение функции, если не можем создать звездную систему
        }
        StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

        // Генерация звезд для каждой планетарной системы
        int AmountOfStars = StarSystemModel.AmountOfStars;
        for (int i = 0; i < AmountOfStars; i++)
        {
            FStarGenerationModel StarModel = StarGenerator->GenerateRandomStarModel();
            FPlanetarySystemGenerationModel PlanetraySystemModel = 
                PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(StarModel, PlanetGenerator, MoonGenerator);

            // Create Planetray System
            APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);
            if (!NewPlanetarySystem) 
            {
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // Завершаем выполнение функции, если не можем создать звездную систему
            }
            PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);

            // Проверяем, успешно ли создана планетарная система
            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); 
            if (!NewPlanetarySystem) 
            {
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // Завершаем выполнение функции, если не можем создать звездную систему

            }
            StarGenerator->ApplyModel(NewStar, StarModel);
            NewPlanetarySystem->SetStar(NewStar);
            NewStar->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepRelativeTransform);

            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
            // Генерация планет для каждой звезды


            // Planetary bodies generation
            for (const FPlanetData FPlanetData : PlanetraySystemModel.PlanetsList)
            {

                // Planet Model and generation
                FPlanetGenerationModel PlanetModel = FPlanetData.PlanetModel; 
                APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);
                PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
                PlanetGenerator->ConnectPlanetWithStar(NewPlanet, NewStar);
                /*NewStar->AddPlanet(NewPlanet);
                NewPlanet->AttachToActor(NewStar, FAttachmentTransformRules::KeepRelativeTransform);*/

                // generate moons model for planet 
                //int AmountOfMoons = PlanetModel.AmountOfMoons;

                // Generate Moons
                UE_LOG(LogTemp, Warning, TEXT("PlanetModel.MoonsList %d"), PlanetModel.MoonsList.Num());

                for (const FMoonData FMoonData : PlanetModel.MoonsList)
                {
					FMoonGenerationModel MoonModel = FMoonData.MoonModel; 
					AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass);
					MoonGenerator->ApplyModel(NewMoon, MoonModel);
                    MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);
                    /*NewPlanet->AddMoon(NewMoon);
					NewMoon->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepRelativeTransform);*/
				}

            }
            NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
            // Прикрепить NewPlanetarySystem к NewStarSystem
            NewPlanetarySystem->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepRelativeTransform);
        }
    }
}

//FPlanetarySystemGenerationModel AStarClusterGenerator::GenerateRandomPlanetraySystemModel()
//{
//    FPlanetarySystemGenerationModel PlanetarySystemModel;
//    PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(1, 10);
//    PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.1, 10.0);
//    return PlanetarySystemModel;
//}


