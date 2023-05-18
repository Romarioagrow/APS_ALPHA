// Fill out your copyright notice in the Description page of Project Settings.

#include "StarSystemGenerator.h"

#include "PlanetarySystemGenerationModel.h"
#include "StarSystem.h"
#include "StarGenerationModel.h"
#include "PlanetarySystem.h"
#include "PlanetGenerationModel.h"
#include "MoonGenerationModel.h"

UStarSystemGenerator::UStarSystemGenerator()
{
    // ������� ����� ���������� �����������
    /*StarGenerator = NewObject<UStarGenerator>(this);
    PlanetGenerator = NewObject<UPlanetGenerator>(this);
    MoonGenerator = NewObject<UMoonGenerator>(this);*/

    /*StarGenerator = NewObject<UStarGenerator>(this, TEXT("StarGenerator"));
    PlanetGenerator = NewObject<UPlanetGenerator>(this, TEXT("PlanetGenerator"));
    MoonGenerator = NewObject<UMoonGenerator>(this, TEXT("MoonGenerator"));*/
    /*StarGenerator = CreateDefaultSubobject<UStarGenerator>(TEXT("StarGenerator"));
    PlanetGenerator = CreateDefaultSubobject<UPlanetGenerator>(TEXT("PlanetGenerator"));
    MoonGenerator = CreateDefaultSubobject<UMoonGenerator>(TEXT("MoonGenerator"));*/
    
    StarGenerator = CreateDefaultSubobject<UStarGenerator>(TEXT("UStarGenerator"));
    PlanetGenerator = CreateDefaultSubobject<UPlanetGenerator>(TEXT("UPlanetGenerator"));
    MoonGenerator = CreateDefaultSubobject<UMoonGenerator>(TEXT("UMoonGenerator"));
}



void UStarSystemGenerator::GenerateHomeStarSystem(FStarSystemGenerationModel StarSystemModel, UWorld* World)
{
    GenerateRandomStarSystem(StarSystemModel, World);
}

void UStarSystemGenerator::GenerateRandomStarSystem(FStarSystemGenerationModel StarSystemModel, UWorld* World)
{
    //UWorld* World = GetWorld();

    /*if (!StarGenerator || !PlanetGenerator || !MoonGenerator) {
        UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
        return;
    }*/


    if (World) // ���������, ���������� �� ���
    {
        /*StarGenerator = CreateDefaultSubobject<UStarGenerator>(TEXT("UStarGenerator"));
        PlanetGenerator = CreateDefaultSubobject<UPlanetGenerator>(TEXT("UPlanetGenerator"));
        MoonGenerator = CreateDefaultSubobject<UMoonGenerator>(TEXT("UMoonGenerator"));*/
        
        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(); // ������� ����� �������� �������
        if (!NewStarSystem) // ���������, ������� �� ������� �������� �������
        {
            // ������������ ������
            UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
            return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������
        }

        /// StarSystemGenerator->ApplyModel();
        int AmountOfStars = StarSystemModel.AmountOfStars;
        NewStarSystem->SetStarsAmount(AmountOfStars);

        // ��������� ����� ��� ������ ����������� �������
        for (int i = 0; i < AmountOfStars; i++)
        {
            FStarGenerationModel StarModel = StarGenerator->GenerateRandomStarModel();
            FPlanetarySystemGenerationModel PlanetraySystemModel = GenerateRandomPlanetraySystemModel(); // Add StarModel and calculate based on it  

			// Create Planetray System
			APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>();
            if (!NewPlanetarySystem) // ���������, ������� �� ������� ����������� �������
            {
				// ������������ ������
				UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

			}
            /// PlanetarySystemGenerator->ApplyModel();

            
            AStar* NewStar = World->SpawnActor<AStar>(); //StarGenerator->GenerateStar(StarModel);
            if (!NewPlanetarySystem) // ���������, ������� �� ������� ����������� �������
            {
                // ������������ ������
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

            }
            /// StarGenerator->ApplyModel();
			NewPlanetarySystem->SetStar(NewStar);
			int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;

			// ��������� ������ ��� ������ ������
            for (int j = 0; j < AmountOfPlanets; j++)
            {
				//FPlanetGenerationModel PlanetModel = PlanetGenerator->GenerateRandomPlanetModel();
                APlanet* NewPlanet = World->SpawnActor<APlanet>(); //PlanetGenerator->GeneratePlanet(PlanetModel);
				
                if (!NewPlanetarySystem) // ���������, ������� �� ������� ����������� �������
                {
                    // ������������ ������
                    UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                    return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

                }

                /// PlanetGenerator->ApplyModel();
                NewStar->AddPlanet(NewPlanet);


                // Generate Moons
                //FMoonGenerationModel MoonModel = MoonGenerator->GenerateRandomMoonModel();
                //int AmountOfMoons = PlanetModel.AmountOfMoons;

                for (int k = 0; k < 3; k++)
                {
					AMoon* NewMoon = World->SpawnActor<AMoon>(); //MoonGenerator->GenerateMoon(MoonModel);
                    if (!NewMoon) // ���������, ������� �� ������� ����������� �������
                    {
						// ������������ ������
						UE_LOG(LogTemp, Warning, TEXT("NewMoon Falied!"));
                        return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������

					}
					/// MoonGenerator->ApplyModel();
					NewPlanet->AddMoon(NewMoon);
				}

			}
			NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
		}
    }
}

FStarSystemGenerationModel UStarSystemGenerator::GenerateHomeStarSystemModel()
{
    return FStarSystemGenerationModel();
}

void UStarSystemGenerator::CreatePlanetarySystem(FPlanetarySystemGenerationModel PlanetarySystemModel)
{
}


FStarSystemGenerationModel UStarSystemGenerator::GenerateRandomStarSystemModel()
{
    FStarSystemGenerationModel StarSystemModel;
	StarSystemModel.AmountOfStars = FMath::RandRange(1, 3);
	StarSystemModel.DistanceBetweenStars = FMath::RandRange(0.1, 10.0);
	return StarSystemModel;
}

FPlanetarySystemGenerationModel UStarSystemGenerator::GenerateRandomPlanetraySystemModel()
{
    FPlanetarySystemGenerationModel PlanetarySystemModel;
    PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(1, 10);
    PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.1, 10.0);
    return PlanetarySystemModel;
    
}
