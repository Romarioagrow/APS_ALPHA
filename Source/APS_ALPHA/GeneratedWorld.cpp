#include "GeneratedWorld.h"
#include "StarSystem.h"
#include "PlanetOrbit.h"
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
        // —оздаем новую звездную систему
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

            // ѕровер€ем, успешно ли создана планетарна€ система
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
            //FVector StarFullscaledRadius = StarModel.Radius * FVector(813684224.0, 813684224.0, 813684224.0);
            NewStar->SetActorScale3D(FVector(StarModel.Radius * 813684224.0));
            NewStar->StarRadiusKM = StarModel.Radius * 696340;

            // √енераци€ планет дл€ каждой звезды
            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
            for (const FPlanetData FPlanetData : PlanetraySystemModel.PlanetsList)
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
                //NewPlanet->SetActorScale3D(FVector(PlanetModel.Radius * 1000.0));
                NewPlanet->SetActorScale3D(FVector(PlanetModel.Radius * 12742000));
                FVector NewLocation = FVector(PlanetModel.OrbitDistance * 149600000000000 / 1000, 0, 0);
                NewPlanet->PlanetRadiusKM = PlanetModel.Radius * 6371;
                NewPlanet->SetActorLocation(NewLocation);
                //NewPlanet->OrbitAnchor->SetWorldLocation(NewLocation);
                // set planet orbit full-scale location

                const double KM_TO_UE_UNIT_SCALE = 100000;
                //double OrbitRadiusOfLastMoon = 0;
                double DiameterOfLastMoon = 0;
                FVector LastMoonLocation;
                // Generate Moons
                for (const FMoonData MoonData : PlanetModel.MoonsList) /// TODO: Ref to pointers FMoonData 
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
				
                    //OrbitRadiusOfLastMoon = MoonData.OrbitRadius * PlanetModel.RadiusKM;
                    DiameterOfLastMoon = MoonRadius * 2;
                    LastMoonLocation = NewMoon->GetActorLocation();
                }
 
                FVector PlanetLocation = NewPlanet->GetActorLocation();
                FVector LastMoonOuterEdgeLocation = LastMoonLocation + FVector(0, DiameterOfLastMoon * 6371, 0);
                float SphereRadius = FVector::Dist(PlanetLocation, LastMoonOuterEdgeLocation);
                SphereRadius /= NewPlanet->GetActorScale3D().X;
                NewPlanet->PlanetaryZone->SetSphereRadius(SphereRadius * 1.1);
            }
            /// TODO: StarSystemGenerator->ConnectPlanetarySystem()
            NewStarSystem->AddPlanetarySystem(NewPlanetarySystem);
            NewPlanetarySystem->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
        }


        //// Arrange astro bodies by hierarchy 
        //for (AStarSystem* GeneratedStarSystem : GeneratedStarSystems)
        //{
        //    if (GeneratedStarSystem)
        //    {
        //        GeneratedStarSystem
        //    }
        //}


    }
}



