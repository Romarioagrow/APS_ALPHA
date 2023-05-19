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
    

    if (World) // ���������, ���������� �� ���
    {
        // ������� ����� �������� �������
        FStarSystemGenerationModel StarSystemModel = StarSystemGenerator->GenerateRandomStarSystemModel();
        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass); 
        // ���������, ������� �� ������� �������� �������
        if (!NewStarSystem) 
        {
            // ������������ ������
            UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
            return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������
        }
        StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

        int AmountOfStars = StarSystemModel.AmountOfStars;
        // ��������� ����� ��� ������ ����������� �������
        for (int i = 0; i < AmountOfStars; i++)
        {
            
            FStarGenerationModel StarModel = StarGenerator->GenerateRandomStarModel();
            FPlanetarySystemGenerationModel PlanetraySystemModel = PlanetarySystemGenerator->GenerateRandomPlanetraySystemModelByStar(StarModel); // Add StarModel and calculate based on it  

            // Create Planetray System
            APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);
            // ���������, ������� �� ������� ����������� �������
            if (!NewPlanetarySystem) 
            {
                // ������������ ������
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

            }
            PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);

            // ���������, ������� �� ������� ����������� �������
            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); //StarGenerator->GenerateStar(StarModel);
            if (!NewPlanetarySystem) 
            {
                // ������������ ������
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

            }
            StarGenerator->ApplyModel(NewStar, StarModel);
            NewPlanetarySystem->SetStar(NewStar);
            NewStar->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepRelativeTransform);

            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
            // ��������� ������ ��� ������ ������
            for (int j = 0; j < AmountOfPlanets; j++)
            {
                FPlanetGenerationModel PlanetModel = PlanetGenerator->GenerateRandomPlanetModel();
                APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass); //PlanetGenerator->GeneratePlanet(PlanetModel);

                if (!NewPlanetarySystem) // ���������, ������� �� ������� ����������� �������
                {
                    // ������������ ������
                    UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                    return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

                }
                PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
                NewStar->AddPlanet(NewPlanet);
                NewPlanet->AttachToActor(NewStar, FAttachmentTransformRules::KeepRelativeTransform);

                // Generate Moons
                FMoonGenerationModel MoonModel = MoonGenerator->GenerateRandomMoonModel();
                int AmountOfMoons = PlanetModel.AmountOfMoons;
                for (int k = 0; k < 3; k++)
                {
                    AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass); //MoonGenerator->GenerateMoon(MoonModel);
                    if (!NewMoon) // ���������, ������� �� ������� ����������� �������
                    {
                        // ������������ ������
                        UE_LOG(LogTemp, Warning, TEXT("NewMoon Falied!"));
                        return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

                    }
                    MoonGenerator->ApplyModel(NewMoon, MoonModel);
                    NewPlanet->AddMoon(NewMoon);
                    // ���������� NewMoon � NewPlanet
                    NewMoon->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepRelativeTransform);
                }

            }
            NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
            // ���������� NewPlanetarySystem � NewStarSystem
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


