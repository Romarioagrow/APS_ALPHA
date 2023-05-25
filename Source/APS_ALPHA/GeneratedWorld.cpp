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

    if (World) 
    {
        // ������� ����� �������� �������
        FStarSystemGenerationModel StarSystemModel = StarSystemGenerator->GenerateRandomStarSystemModel();
        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass); 
        if (!NewStarSystem) 
        {
            UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
            return; 
        }
        StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

        // ��������� ����� ��� ������ ����������� �������
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
                return; 
            }
            PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);

            // ���������, ������� �� ������� ����������� �������
            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); 
            if (!NewPlanetarySystem) 
            {
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; 
            }
            StarGenerator->ApplyModel(NewStar, StarModel);

            /// TODO: PlanetarySystemGenerator->ConnectStar()
            NewPlanetarySystem->SetStar(NewStar);
            NewStar->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepRelativeTransform);

            // ��������� ������ ��� ������ ������
            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
            for (const FPlanetData FPlanetData : PlanetraySystemModel.PlanetsList)
            {
                // Planet Model and generation
                FPlanetGenerationModel PlanetModel = FPlanetData.PlanetModel; 
                APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);
                PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
                PlanetGenerator->ConnectPlanetWithStar(NewPlanet, NewStar);

                // Generate Moons
                for (const FMoonData FMoonData : PlanetModel.MoonsList) /// TODO: Ref to pointers FMoonData 
                {
					AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass);
					MoonGenerator->ApplyModel(NewMoon, FMoonData.MoonModel);
                    MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);
				}
            }
            /// TODO: StarSystemGenerator->ConnectPlanetarySystem()
            NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
            NewPlanetarySystem->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepRelativeTransform);
        }
    }
}



