#include "GeneratedWorld.h"
#include "StarSystem.h"
#include "PlanetOrbit.h"
#include "MoonGenerationModel.h"

// Sets default values
AStarClusterGenerator::AStarClusterGenerator()
{
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

    GenerateStarCluster();
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

        // Generate astro model
        int AmountOfStars = StarSystemModel.AmountOfStars;
        for (int StarNumber = 0; StarNumber < AmountOfStars; StarNumber++)
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
            PlanetarySystemGenerator->SetAstroLocation(StarNumber, NewPlanetarySystem);

            // ���������, ������� �� ������� ����������� �������
            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); 
            if (!NewPlanetarySystem) 
            {
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; 
            }
            StarGenerator->ApplyModel(NewStar, StarModel);

            /// TODO: PlanetarySystemGenerator->ConnectStar()
            // Set Star full-scale
            NewPlanetarySystem->SetStar(NewStar);
            NewStar->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);
            NewStar->SetActorScale3D(FVector(StarModel.Radius * 813684224.0));
            NewStar->StarRadiusKM = StarModel.Radius * 696340;

            // ��������� ������ ��� ������ ������
            FVector LastPlanetLocation{ 0 };
            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
            for (const FPlanetData& FPlanetData : PlanetraySystemModel.PlanetsList)
            {
                APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(BP_PlanetOrbit, NewStar->GetActorLocation(), FRotator::ZeroRotator);
                NewPlanetOrbit->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);

                // Planet Model and generation
                FPlanetGenerationModel PlanetModel = FPlanetData.PlanetModel; 
                APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);
                PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
                //PlanetGenerator->ConnectPlanetWithStar(NewPlanet, NewStar);
                NewStar->AddPlanet(NewPlanet);
                NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
                NewPlanet->SetParentStar(NewStar);

                // set planet full-scale
                NewPlanet->SetActorScale3D(FVector(PlanetModel.Radius * 12742000));
                FVector NewLocation = FVector(PlanetModel.OrbitDistance * 149600000000000 / 1000, 0, 0);
                NewPlanet->PlanetRadiusKM = PlanetModel.Radius * 6371;
                NewPlanet->SetActorLocation(NewLocation);
                // set planet orbit full-scale location
                NewPlanetOrbit->SetActorRelativeRotation(FRotator(FMath::RandRange(-30.0, 30.0), FMath::RandRange(-360.0, 360.0), 0));

                const double KM_TO_UE_UNIT_SCALE = 100000;
                double DiameterOfLastMoon = 0;
                FVector LastMoonLocation;
                // Generate Moons
                for (const FMoonData& MoonData : PlanetModel.MoonsList) /// TODO: Ref to pointers FMoonData 
                {
                    APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(BP_PlanetOrbit, NewPlanet->GetActorLocation(), FRotator::ZeroRotator);
                    NewMoonOrbit->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);

                    FVector MoonLocation = NewPlanet->GetActorLocation();
                    AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
                    MoonGenerator->ApplyModel(NewMoon, MoonData.MoonModel);
                    MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);
                    NewPlanet->AddMoon(NewMoon);
                    NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
                    NewMoon->SetParentPlanet(NewPlanet);

                    // set moon full-scale
                    double MoonRadius = MoonData.MoonModel.Radius;
                    NewMoon->SetActorScale3D(FVector(MoonRadius * 12742000));
                    NewMoon->AddActorLocalOffset(FVector(0, ((PlanetModel.RadiusKM + (MoonData.OrbitRadius * PlanetModel.RadiusKM)) * KM_TO_UE_UNIT_SCALE) * 1, 0));
                    NewMoonOrbit->SetActorRelativeRotation(FRotator(FMath::RandRange(-360.0, 360.0), FMath::RandRange(-360.0, 360.0), FMath::RandRange(-360.0, 360.0)));
				
                    DiameterOfLastMoon = MoonRadius * 2;
                    LastMoonLocation = NewMoon->GetActorLocation();
                }

                if (DiameterOfLastMoon == 0)
                {
                    NewPlanet->PlanetaryZone->SetSphereRadius(100);
                }
                else
                {
                    FVector PlanetLocation = NewPlanet->GetActorLocation();
                    FVector LastMoonOuterEdgeLocation = LastMoonLocation + FVector(0, DiameterOfLastMoon * 6371, 0);
                    float SphereRadius = FVector::Dist(PlanetLocation, LastMoonOuterEdgeLocation);
                    SphereRadius /= NewPlanet->GetActorScale3D().X;
                    NewPlanet->PlanetaryZone->SetSphereRadius(SphereRadius * 1.1);
                }

                LastPlanetLocation = NewPlanet->GetActorLocation();
            }
            /// TODO: StarSystemGenerator->ConnectPlanetarySystem()
            NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
            NewPlanetarySystem->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);

            if (LastPlanetLocation.IsZero())
            {
                NewStar->PlanetarySystemZone->SetSphereRadius(500);
            }
            else
            {
                float StarSphereRadius = FVector::Dist(NewStar->GetActorLocation(), LastPlanetLocation);
                StarSphereRadius /= NewStar->GetActorScale3D().X;
                NewStar->PlanetarySystemZone->SetSphereRadius(StarSphereRadius * 1.1);
            }
        }
    }
}


void AStarClusterGenerator::GenerateStarCluster()
{

}
