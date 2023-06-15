#include "AstroGenerator.h"
#include "StarSystem.h"
#include "PlanetOrbit.h"
#include "MoonGenerationModel.h"
#include "StarCluster.h"
#include "Galaxy.h"
#include "Octree.h"

AAstroGenerator::AAstroGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAstroGenerator::BeginPlay()
{
	Super::BeginPlay();

    InitAstroGenerators();

    InitGenerationLevel();
}

void AAstroGenerator::InitGenerationLevel()
{
    switch (AstroGenerationLevel)
    {
    case EAstroGenerationLevel::GalaxiesCluster:
        GenerateGalaxiesCluster();
        break;
    case EAstroGenerationLevel::Galaxy:
        GenerateGalaxy();
        break;
    case EAstroGenerationLevel::StarCluster:
        GenerateStarCluster();
        break;
    case EAstroGenerationLevel::StarSystem:
        GenerateStarSystem();
        break;
    case EAstroGenerationLevel::PlanetSystem:
        GeneratePlanetSystem();
        break;
    case EAstroGenerationLevel::SinglePlanet:
        GenerateSinglePlanet();
        break;
    case EAstroGenerationLevel::Random:
        GenerateRandomWorld();
        break;
    default:
        GenerateRandomWorld();
        break;
    }

    if (bGenerateHomeSystem)
    {
        GenerateStarSystem();
    }
}

void AAstroGenerator::GenerateGalaxiesCluster()
{

}

void AAstroGenerator::GenerateGalaxy()
{
    FGalaxyModel GalaxyModel;
    if (bGenerateRandomGalaxy)
    {
        GalaxyModel = GalaxyGenerator->GenerateRandomGalaxyModel();
    }
    else
    {
        GalaxyModel.GalaxyClass = GalaxyGlass;
        GalaxyModel.GalaxyType = GalaxyType;
        GalaxyModel.StarsCount = GalaxyStarCount;
        GalaxyModel.StarsDensity = GalaxyStarDensity;
        GalaxyModel.GalaxySize = GalaxySize;
    }

    UWorld* World = GetWorld();

    if (World)
    {
        AGalaxy* NewGalaxy = World->SpawnActor<AGalaxy>(BP_GalaxyClass);
        GalaxyGenerator->GenerateGalaxyOctreeStars(StarGenerator, NewGalaxy, GalaxyModel);
        NewGalaxy->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

        GeneratedGalaxy = NewGalaxy;

        if (bGenerateFullScaledWorld)
        {
            this->SetActorScale3D(FVector(1000000000, 1000000000, 1000000000)); /// TODO: Const FullScaleCoefficient
        }
    }
}

void AAstroGenerator::GeneratePlanetSystem()
{

}

void AAstroGenerator::GenerateSinglePlanet()
{

}

void AAstroGenerator::GenerateRandomWorld()
{

}

void AAstroGenerator::InitAstroGenerators()
{
    // Init generators
    GalaxyGenerator = NewObject<UGalaxyGenerator>();
    StarClusterGenerator = NewObject<UStarClusterGenerator>();
    StarSystemGenerator = NewObject<UStarSystemGenerator>();
    PlanetarySystemGenerator = NewObject<UPlanetarySystemGenerator>();
    StarGenerator = NewObject<UStarGenerator>();
    PlanetGenerator = NewObject<UPlanetGenerator>();
    MoonGenerator = NewObject<UMoonGenerator>();

    if (GalaxyGenerator == nullptr 
        || StarClusterGenerator == nullptr 
        || StarSystemGenerator == nullptr 
        || PlanetarySystemGenerator == nullptr 
        || StarGenerator == nullptr 
        || PlanetGenerator == nullptr 
        || MoonGenerator == nullptr)
    {
		UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
		return;
	}
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("All generators OK!"));
    }
}

void AAstroGenerator::GenerateHomeStarSystem()
{



}

void AAstroGenerator::GenerateStarSystem()
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
        FVector LastStarLocation { 0 };
        for (int StarNumber = 0; StarNumber < AmountOfStars; StarNumber++)
        {
            FStarModel StarModel = StarGenerator->GenerateRandomStarModel();

            FPlanetarySystemGenerationModel PlanetraySystemModel = 
                PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(StarModel, PlanetGenerator, MoonGenerator);

            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); 
            APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);
            if (!NewStar || !NewPlanetarySystem)
            {
                UE_LOG(LogTemp, Warning, TEXT("Star Falied!"));
                return; 
            }

            // Set Star full-scale
            /// TODO: PlanetarySystemGenerator->ConnectStar()
            StarGenerator->ApplyModel(NewStar, StarModel);
            PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);
            NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
            NewStar->SetActorScale3D(FVector(StarModel.Radius * 813684224.0));
            NewStar->StarRadiusKM = StarModel.Radius * 696340;
            NewStar->SetPlanetarySystem(NewPlanetarySystem);
            NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
            
            // Apply material luminocity multiplier and emissive light color
            StarGenerator->ApplySpectralMaterial(NewStar, StarModel);

            // √енераци€ планет дл€ каждой звезды
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
                LastStarLocation = LastPlanetLocation * 1.1; 
            }

            double StarSphereRadius;
            if (LastPlanetLocation.IsZero())
            {
                StarSphereRadius = NewStar->GetRadius() * NewStar->GetActorScale3D().X * 2;
                StarSphereRadius /= 1000000;
                NewStar->PlanetarySystemZone->SetSphereRadius(StarSphereRadius);
                NewStar->StarAffectionZoneRadius = StarSphereRadius * 1.2;// * NewStar->GetActorScale3D().X;

            }
            else
            {
                StarSphereRadius = FVector::Dist(NewStar->GetActorLocation(), LastPlanetLocation);
                StarSphereRadius /= NewStar->GetActorScale3D().X;
                NewStar->PlanetarySystemZone->SetSphereRadius(StarSphereRadius * 1.1);
                NewStar->StarAffectionZoneRadius = StarSphereRadius;

            }
            NewStarSystem->AddNewStar(NewStar);
            NewStarSystem->StarSystemRadius = NewStar->StarAffectionZoneRadius;
        }

        double StarSystemSphereRadius = FVector::Dist(NewStarSystem->GetActorLocation(), LastStarLocation);
        StarSystemSphereRadius /= NewStarSystem->GetActorScale3D().X;
        NewStarSystem->StarSystemZone->SetSphereRadius(StarSystemSphereRadius * 1.1);
        NewStarSystem->StarSystemRadius = StarSystemSphereRadius;

        if (bGenerateFullScaledWorld )
        {
            /* method 1: non-adaptive
                if center = spawn at 000
                if random = spawn at random index
                if zones = spawn in distance from center

                check overlapped hism, if overlapped, (re)move it
            */

            
            double HomeSystemrRadiusScaled = NewStarSystem->StarSystemRadius * StarSystemDeadZone;
            FCollisionShape MySphere = FCollisionShape::MakeSphere(HomeSystemrRadiusScaled);
            TArray<FOverlapResult> Overlaps;

            // выполните проверку перекрыти€
            FVector HomeSystemLocation = NewStarSystem->GetActorLocation();
            bool isOverlap = World->OverlapMultiByChannel(Overlaps, HomeSystemLocation, FQuat::Identity, ECC_Visibility, MySphere);

            float DebugDuration = 60.0f;
            DrawDebugSphere(
                GetWorld(),
                HomeSystemLocation,
                HomeSystemrRadiusScaled,
                50,
                FColor::Purple,
                true,
                DebugDuration
            );

            TArray<int32> InstancesToRemove;
            UHierarchicalInstancedStaticMeshComponent* OverlappingHISM = nullptr;

            if (isOverlap)
            {
                int TotalOverlaps = Overlaps.Num();
                UE_LOG(LogTemp, Warning, TEXT("Total number of overlaps: %d"), TotalOverlaps);

                for (auto& Result : Overlaps)
                {
                    UHierarchicalInstancedStaticMeshComponent* HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(Result.Component.Get());
                    if (HISM)
                    {
                        OverlappingHISM = HISM;
                        int32 InstanceIndex = Result.ItemIndex;
                        InstancesToRemove.Add(InstanceIndex);
                    }
                }
                // сортируем массив индексов в обратном пор€дке
                InstancesToRemove.Sort([](const int32& A, const int32& B) { return A > B; });
                for (auto Index : InstancesToRemove)
                {
                    OverlappingHISM->RemoveInstance(Index);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No overlaped stars!"));
            }
        }
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

int AAstroGenerator::GetRandomValueFromStarAmountRange(EStarClusterType ClusterType)
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
        UWorld* World = GetWorld();

        FStarClusterModel StarClusterModel;
        if (bGenerateRandomCluster)
        {
            StarClusterModel = StarClusterGenerator->GetRandomStarClusterModel();
        }
        else
        {
            StarClusterModel.StarClusterSize = StarClusterSize;
            StarClusterModel.StarClusterType = StarClusterType;
            StarClusterModel.StarClusterPopulation = StarClusterPopulation;
            StarClusterModel.StarClusterComposition = StarClusterComposition;
        }

        EStarClusterType ClusterType = StarClusterModel.StarClusterType;
        AStarCluster* NewStarCluster = World->SpawnActor<AStarCluster>(BP_StarClusterClass);
        NewStarCluster->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        /// Calculate Cluster Params
        NewStarCluster->StarAmount = StarClusterGenerator->GetStarsAmountByRange(StarClusterModel.StarClusterSize);
        NewStarCluster->StarDensity = StarClusterGenerator->GetStarClusterDensityByRange();
        NewStarCluster->ClusterBounds = StarClusterGenerator->GetStarClusterBoundsByRange(ClusterType);
        NewStarCluster->ClusterType = ClusterType;
        NewStarCluster->StarClusterComposition = StarClusterModel.StarClusterComposition;
        NewStarCluster->StarClusterPopulation = StarClusterModel.StarClusterPopulation;
        NewStarCluster->StarClusterSize = StarClusterModel.StarClusterSize;

        UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), NewStarCluster->StarAmount);
        UE_LOG(LogTemp, Warning, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
        UE_LOG(LogTemp, Warning, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
        UE_LOG(LogTemp, Warning, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

        for (size_t i = 0; i < NewStarCluster->StarAmount; i++)
        {
            // —оздаем модель звезды
            FStarModel NewStarModel;

            if (bGenerateRandomCluster)
            {
                NewStarModel = StarGenerator->GenerateRandomStarModel();
            }
            else
            {
                NewStarModel = StarGenerator->GenerateStarModelByProbability(StarClusterModel);
            }

            // ѕозиционируем звезду в кластере
            FVector StarPosition = StarClusterGenerator->CalculateStarPosition(i, NewStarCluster, NewStarModel);
            NewStarCluster->AddStarToClusterModel(StarPosition, NewStarModel);

            // —оздаем инстанс звезды и добавл€ем его в HISM компонент
            FTransform StarTransform(StarPosition);
            StarTransform.SetScale3D(FVector(NewStarModel.Radius));
            int32 StarInstIndex = NewStarCluster->StarMeshInstances->AddInstance(StarTransform, true);
            FLinearColor ColorValue = StarGenerator->GetStarColor(NewStarModel.SpectralClass, NewStarModel.SpectralSubclass);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);

            double StarEmission = StarGenerator->CalculateEmission(NewStarModel.Luminosity * 25);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);
        }

        GeneratedStarCluster = NewStarCluster;

        if (bGenerateFullScaledWorld)
        {
            NewStarCluster->SetActorScale3D(FVector(1000000000, 1000000000, 1000000000));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StarClusterGenerator is not implemented yet!"));
    }
}


