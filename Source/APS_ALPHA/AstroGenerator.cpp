#include "AstroGenerator.h"
#include "StarSystem.h"
#include "PlanetOrbit.h"
#include "MoonGenerationModel.h"
#include "StarCluster.h"

// Sets default values
AAstroGenerator::AAstroGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AAstroGenerator::BeginPlay()
{
	Super::BeginPlay();

    // Init generators
    StarClusterGenerator = NewObject<UStarClusterGenerator>();
    StarSystemGenerator = NewObject<UStarSystemGenerator>();
    PlanetarySystemGenerator = NewObject<UPlanetarySystemGenerator>();
    StarGenerator = NewObject<UStarGenerator>();
    PlanetGenerator = NewObject<UPlanetGenerator>();
    MoonGenerator = NewObject<UMoonGenerator>();

    if (bGenerateHomeSystem)
    {
	    GenerateRandomStarSystem();
    }

    GenerateStarCluster();
}

void AAstroGenerator::GenerateRandomStarSystem()
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
        // Создаем новую звездную систему
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
            FStarModel StarModel = StarGenerator->GenerateRandomStarModel();
            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); 

            FPlanetarySystemGenerationModel PlanetraySystemModel = 
                PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(StarModel, PlanetGenerator, MoonGenerator);

            APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);
            if (!NewPlanetarySystem) 
            {
                UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem Falied!"));
                return; 
            }

            // Set Star full-scale
            /// TODO: PlanetarySystemGenerator->ConnectStar()
            StarGenerator->ApplyModel(NewStar, StarModel);
            PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);
            //PlanetarySystemGenerator->SetAstroLocation(StarNumber, NewPlanetarySystem);
            NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
            NewStar->SetActorScale3D(FVector(StarModel.Radius * 813684224.0));
            NewStar->StarRadiusKM = StarModel.Radius * 696340;
            NewStar->SetPlanetarySystem(NewPlanetarySystem);
            NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
            
            // Apply material luminocity multiplier and emissive light color
            StarGenerator->ApplySpectralMaterial(NewStar, StarModel);

            // Генерация планет для каждой звезды
            FVector LastPlanetLocation{ 0 };
            int AmountOfPlanets = PlanetraySystemModel.AmountOfPlanets;
            for (const FPlanetData& FPlanetData : PlanetraySystemModel.PlanetsList)
            {
                APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(BP_PlanetOrbit, NewPlanetarySystem->GetActorLocation(), FRotator::ZeroRotator);
                NewPlanetOrbit->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);

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
                    double SphereRadius = FVector::Dist(PlanetLocation, LastMoonOuterEdgeLocation);
                    SphereRadius /= NewPlanet->GetActorScale3D().X;
                    NewPlanet->PlanetaryZone->SetSphereRadius(SphereRadius * 1.1);
                }

                LastPlanetLocation = NewPlanet->GetActorLocation();
            }

            if (LastPlanetLocation.IsZero())
            {
                NewStar->PlanetarySystemZone->SetSphereRadius(500);
            }
            else
            {
                double StarSphereRadius = FVector::Dist(NewStar->GetActorLocation(), LastPlanetLocation);
                NewStar->StarAffectionZoneRadius = StarSphereRadius;
                StarSphereRadius /= NewStar->GetActorScale3D().X;
                NewStar->PlanetarySystemZone->SetSphereRadius(StarSphereRadius * 1.1);
            }
            
            NewStarSystem->AddNewStar(NewStar);
        }

        
    }
}

EStarClusterType AAstroGenerator::ProvideStarClusterType()
{
	//...
    if (bGenerateRandomCluster)
    {
        return static_cast<EStarClusterType>(FMath::RandRange(0, static_cast<int>(EStarClusterType::Nebula)));
    }
    else
    {
        return StarClusterType;
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

int AAstroGenerator::GetRandomValueFromRange(EStarClusterType ClusterType)
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
    /// TODO: StarClusterGenerator->GenerateRandomStarCluster(World);
    if (true)
    {
        //...
        UWorld* World = GetWorld();

        EStarClusterType ClusterType = ProvideStarClusterType();//static_cast<EStarClusterType>(FMath::RandRange(0, static_cast<int>(EStarClusterType::Nebula)));
        AStarCluster* NewStarCluster = World->SpawnActor<AStarCluster>(BP_StarClusterClass);
        NewStarCluster->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        NewStarCluster->ClusterType = ClusterType;
        NewStarCluster->StarCount = GetRandomValueFromRange(ClusterType);//ClusterStarAmount[ClusterType];//FMath::RandRange(100, 1000);
        NewStarCluster->StarDensity = FMath::RandRange(0.1f, 1.0f);
        NewStarCluster->ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);

        UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), NewStarCluster->StarCount);
        UE_LOG(LogTemp, Warning, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
        UE_LOG(LogTemp, Warning, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
        UE_LOG(LogTemp, Warning, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

        for (size_t i = 0; i < NewStarCluster->StarCount; i++)
        {
            // Создаем модель звезды
            FStarModel NewStarModel = StarGenerator->GenerateRandomStarModel();

            // Позиционируем звезду в кластере
            FVector StarPosition = StarClusterGenerator->CalculateStarPosition(i, NewStarCluster, NewStarModel);

            // Добавляем модель звезды в список моделей звезд кластера
            NewStarCluster->AddStarToClusterModel(StarPosition, NewStarModel);


            // Создаем инстанс звезды и добавляем его в HISM компонент
            FTransform StarTransform(StarPosition);
            StarTransform.SetScale3D(FVector(NewStarModel.Radius));
            NewStarCluster->StarMeshInstances->AddInstance(StarTransform);
        }

        if (bGenerateFullScaledStarCluster)
        {
            NewStarCluster->SetActorScale3D(FVector(100000000, 100000000, 100000000));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StarClusterGenerator is not implemented yet!"));
    }
}
