#include "StarSystemGenerator.h"
#include "StarSystem.h"
// Fill out your copyright notice in the Description page of Project Settings.

//#include "StarSystemGenerator.h"
//
//#include "PlanetarySystemGenerationModel.h"
//#include "StarSystem.h"
//#include "StarGenerationModel.h"
//#include "PlanetarySystem.h"
//#include "PlanetGenerationModel.h"
//#include "MoonGenerationModel.h"
//
//UStarSystemGenerator::UStarSystemGenerator()
//{
//    // ������� ����� ���������� �����������
//    /*StarGenerator = NewObject<UStarGenerator>(this);
//    PlanetGenerator = NewObject<UPlanetGenerator>(this);
//    MoonGenerator = NewObject<UMoonGenerator>(this);*/
//
//    /*StarGenerator = NewObject<UStarGenerator>(this, TEXT("StarGenerator"));
//    PlanetGenerator = NewObject<UPlanetGenerator>(this, TEXT("PlanetGenerator"));
//    MoonGenerator = NewObject<UMoonGenerator>(this, TEXT("MoonGenerator"));*/
//    /*StarGenerator = CreateDefaultSubobject<UStarGenerator>(TEXT("StarGenerator"));
//    PlanetGenerator = CreateDefaultSubobject<UPlanetGenerator>(TEXT("PlanetGenerator"));
//    MoonGenerator = CreateDefaultSubobject<UMoonGenerator>(TEXT("MoonGenerator"));*/
//    
//    StarGenerator = CreateDefaultSubobject<UStarGenerator>(TEXT("UStarGenerator"));
//    PlanetGenerator = CreateDefaultSubobject<UPlanetGenerator>(TEXT("UPlanetGenerator"));
//    MoonGenerator = CreateDefaultSubobject<UMoonGenerator>(TEXT("UMoonGenerator"));
//}



//void UStarSystemGenerator::GenerateHomeStarSystem(FStarSystemGenerationModel StarSystemModel, UWorld* World)
//{
//    GenerateRandomStarSystem(StarSystemModel, World);
//}

//void UStarSystemGenerator::GenerateRandomStarSystem(FStarSystemGenerationModel StarSystemModel, UWorld* World)
//{
//    //UWorld* World = GetWorld();
//
//    /*if (!StarGenerator || !PlanetGenerator || !MoonGenerator) {
//        UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
//        return;
//    }*/
//
//
//    if (World) // ���������, ���������� �� ���
//    {
//        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(); // ������� ����� �������� �������
//        if (!NewStarSystem) // ���������, ������� �� ������� �������� �������
//        {
//            // ������������ ������
//            UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
//            return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������
//        }
//
//        StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);
//        int AmountOfStars = StarSystemModel.AmountOfStars;
//        //NewStarSystem->SetStarsAmount(AmountOfStars);
//
//        // ��������� ����� ��� ������ ����������� �������
//        for (int i = 0; i < AmountOfStars; i++)
//        {
//            FPlanetarySystemGenerationModel PlanetraySystemModel = GenerateRandomPlanetraySystemModel(); // Add StarModel and calculate based on it  
//
//			// Create Planetray System
//			APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>();
//            if (!NewPlanetarySystem) // ���������, ������� �� ������� ����������� �������
//            {
//				// ������������ ������
//				UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
//                return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������
//
//			}
//            PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);
//
//            FStarGenerationModel StarModel = StarGenerator->GenerateRandomStarModel();
//            AStar* NewStar = World->SpawnActor<AStar>(); //StarGenerator->GenerateStar(StarModel);
//            if (!NewPlanetarySystem) // ���������, ������� �� ������� ����������� �������
//            {
//                // ������������ ������
//                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
//                return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������
//
//            }
//            StarGenerator->ApplyModel(NewStar, StarModel);
//			//NewPlanetarySystem->SetStar(NewStar);
//
//			int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
//			// ��������� ������ ��� ������ ������
//            for (int j = 0; j < AmountOfPlanets; j++)
//            {
//				FPlanetGenerationModel PlanetModel = PlanetGenerator->GenerateRandomPlanetModel();
//                APlanet* NewPlanet = World->SpawnActor<APlanet>(); //PlanetGenerator->GeneratePlanet(PlanetModel);
//				
//                if (!NewPlanetarySystem) // ���������, ������� �� ������� ����������� �������
//                {
//                    // ������������ ������
//                    UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
//                    return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������
//
//                }
//                PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
//                NewStar->AddPlanet(NewPlanet);
//
//                // Generate Moons
//                FMoonGenerationModel MoonModel = MoonGenerator->GenerateRandomMoonModel();
//                int AmountOfMoons = PlanetModel.AmountOfMoons;
//                for (int k = 0; k < AmountOfMoons; k++)
//                {
//					AMoon* NewMoon = World->SpawnActor<AMoon>(); //MoonGenerator->GenerateMoon(MoonModel);
//                    if (!NewMoon) // ���������, ������� �� ������� ����������� �������
//                    {
//						// ������������ ������
//						UE_LOG(LogTemp, Warning, TEXT("NewMoon Falied!"));
//                        return; // ��������� ���������� �������, ���� �� ����� ������� �������� �������
//
//					}
//					MoonGenerator->ApplyModel(NewMoon, MoonModel);
//					NewPlanet->AddMoon(NewMoon);
//				}
//
//			}
//			NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
//		}
//    }
//}
//
//FStarSystemGenerationModel UStarSystemGenerator::GenerateHomeStarSystemModel()
//{
//    return FStarSystemGenerationModel();
//}
//
//void UStarSystemGenerator::CreatePlanetarySystem(FPlanetarySystemGenerationModel PlanetarySystemModel)
//{
//}
//
//
//FStarSystemGenerationModel UStarSystemGenerator::GenerateRandomStarSystemModel()
//{
//    FStarSystemGenerationModel StarSystemModel;
//	StarSystemModel.AmountOfStars = FMath::RandRange(1, 3);
//	StarSystemModel.DistanceBetweenStars = FMath::RandRange(0.1, 10.0);
//	return StarSystemModel;
//}
//
//FPlanetarySystemGenerationModel UStarSystemGenerator::GenerateRandomPlanetraySystemModel()
//{
//    FPlanetarySystemGenerationModel PlanetarySystemModel;
//    PlanetarySystemModel.AmountOfPlanets = FMath::RandRange(1, 10);
//    PlanetarySystemModel.DistanceBetweenPlanets = FMath::RandRange(0.1, 10.0);
//    return PlanetarySystemModel;
//    
//}

UStarSystemGenerator::UStarSystemGenerator()
{
}

//void UStarSystemGenerator::ApplyModel(AStarSystem* StarSystem, FStarSystemGenerationModel StarSystemGenerationModel);// override;
//{
//	// NewStarSystem->SetStarsAmount(AmountOfStars);
//
//	//AStarSystem* StarSystem = Cast<AStarSystem>(AstroActor);
//	//if (StarSystem)
//	//{
//
//	//	StarSystem->SetStarsAmount(GenerationModel);
//	//	// ����������� StarSystem ��� ���������� ��������, ����������� ��� ������� �����.
//	//}
//
//}

//void ApplyModel(AStarSystem* StarSystem, FStarSystemGenerationModel StarSystemGenerationModel);
//{
//	StarSystem->SetStarsAmount(StarSystemGenerationModel.AmountOfStars);
//	StarSystem->SetStarSystemType(StarSystemGenerationModel.StarSystemType);
////	StarSystem->SetDistanceBetweenStars(StarSystemGenerationModel.DistanceBetweenStars);
//}

//void UStarSystemGenerator::ApplyModel(AStarSystem* StarSystem, FStarSystemGenerationModel StarSystemGenerationModel)
//{
//	StarSystem->SetStarsAmount(StarSystemGenerationModel.AmountOfStars);
//	StarSystem->SetStarSystemType(StarSystemGenerationModel.StarSystemType);
//}

void UStarSystemGenerator::ApplyModel(AStarSystem* StarSystem, FStarSystemGenerationModel StarSystemGenerationModel)
{
	StarSystem->SetStarsAmount(StarSystemGenerationModel.AmountOfStars);
	StarSystem->SetStarSystemType(StarSystemGenerationModel.StarSystemType);
}

FStarSystemGenerationModel UStarSystemGenerator::GenerateRandomStarSystemModel()
{
	FStarSystemGenerationModel StarSystemModel;
	//StarSystemModel.DistanceBetweenStars = FMath::RandRange(0.1, 10.0);

	// ��������� ����� � ��������� �� 0 �� 100
	int32 RandomValue = FMath::RandRange(0, 100);

	// ���� ��� ������� ���� �������
	int32 SingleStarWeight = 80; // 80%
	int32 DoubleStarWeight = 10; // 10%
	int32 TripleStarWeight = 8; // 8%
	int32 MultipleStarWeight = 2; // 2%

	if (RandomValue < SingleStarWeight)
	{
		StarSystemModel.StarSystemType = EStarSystemType::SingleStar;
		StarSystemModel.AmountOfStars = 1;
	}
	else if (RandomValue < SingleStarWeight + DoubleStarWeight)
	{
		StarSystemModel.StarSystemType = EStarSystemType::DoubleStar;
		StarSystemModel.AmountOfStars = 2;
	}
	else if (RandomValue < SingleStarWeight + DoubleStarWeight + TripleStarWeight)
	{
		StarSystemModel.StarSystemType = EStarSystemType::TripleStar;
		StarSystemModel.AmountOfStars = 3;
	}
	else
	{
		StarSystemModel.StarSystemType = EStarSystemType::MultipleStar;
		StarSystemModel.AmountOfStars = FMath::RandRange(4, 6);
	}

	return StarSystemModel;
}
