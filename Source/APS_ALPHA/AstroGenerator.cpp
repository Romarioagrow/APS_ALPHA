#include "AstroGenerator.h"
#include "StarSystem.h"
#include "PlanetOrbit.h"
#include "MoonGenerationModel.h"
#include "StarCluster.h"
#include "Galaxy.h"
#include "Octree.h"
#include "SpaceStation.h"
#include <Kismet/GameplayStatics.h>
#include "GameFramework/Character.h"


AAstroGenerator::AAstroGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAstroGenerator::BeginPlay()
{
	Super::BeginPlay();

    InitAstroGenerators();

    if (bGenerateTEST_FULLSCALED)
    {
        GenerateTEST_FULLSCALED();
    }  
    else
    {
        InitGenerationLevel();
    }

}
void AAstroGenerator::GenerateTEST_FULLSCALED()
{
    // ������ ��������� �����
    const double MinStarRadius = 0.1f;
    const double MaxStarRadius = 10.0f;
    const int StarCount = 100;

    // ������� �����-����� ��� ������ ����
   // AAstroAnchor* AstroAnchor = GetWorld()->SpawnActor<AAstroAnchor>();
    AAstroAnchor* AstroAnchor = GetWorld()->SpawnActor<AAstroAnchor>(BP_AstroAnchor);

    // ������� ����� �������� ����� � ����������� ��� � �����
    AStarCluster* StarCluster = GetWorld()->SpawnActor<AStarCluster>(BP_StarClusterClass);
    StarCluster->AttachToActor(AstroAnchor, FAttachmentTransformRules::KeepRelativeTransform);

    // ������� ������� �����
    for (int i = 0; i < StarCount; ++i)
    {
        // ���������� ��������� ���������� � ������ ��� ������
        FVector StarPosition = FMath::VRand() * FMath::FRand() * 5.0f;
        double StarRadius = FMath::RandRange(MinStarRadius, MaxStarRadius);

        // ������� �������������� ��� ������
        FTransform StarTransform;
        StarTransform.SetLocation(StarPosition * 10000);
        StarTransform.SetScale3D(FVector(StarRadius));

        // ��������� ������� ������ � HISM
        StarCluster->StarMeshInstances->AddInstance(StarTransform);
        

    }
    //// ������� ������� �����
    //for (int i = 0; i < StarCount; ++i)
    //{
    //    // ���������� ��������� ���������� � ������ ��� ������
    //    FVector StarPosition = FMath::RandPointInBox(FBox(FVector(-5.0f, -5.0f, -5.0f), FVector(5.0f, 5.0f, 5.0f)));
    //    float StarRadius = FMath::RandRange(MinStarRadius, MaxStarRadius);

    //    // ������� �������������� ��� ������
    //    FTransform StarTransform;
    //    StarTransform.SetLocation(StarPosition);
    //    StarTransform.SetScale3D(FVector(StarRadius));

    //    // ��������� ������� ������ � HISM
    //    StarCluster->StarMeshInstances->AddInstance(StarTransform);
    //}

    // ���������� �����-����� � ����� �������� � ������ ����������
    GeneratedWorld = AstroAnchor;
    GeneratedStarCluster = StarCluster;
}
//void AAstroGenerator::GenerateTEST_FULLSCALED()
//{
//    // ������ ��������� �����
//    const float MinStarRadius = 0.1f;
//    const float MaxStarRadius = 10.0f;
//    const int StarCount = 100;
//
//    // ������� �����-����� ��� ������ ����
//    AAstroAnchor* AstroAnchor = GetWorld()->SpawnActor<AAstroAnchor>();
//
//    // ������� ����� �������� ����� � ����������� ��� � �����
//    AStarCluster* StarCluster = GetWorld()->SpawnActor<AStarCluster>();
//    //StarCluster->AttachToActor(AstroAnchor, FAttachmentTransformRules::KeepRelativeTransform);
//
//    // ������� ������� �����
//    for (int i = 0; i < StarCount; ++i)
//    {
//        // ���������� ��������� ���������� � ������ ��� ������
//        FVector StarPosition = FMath::RandPointInBox(FBox(FVector(-5.0f, -5.0f, -5.0f), FVector(5.0f, 5.0f, 5.0f)));
//        float StarRadius = FMath::RandRange(MinStarRadius, MaxStarRadius);
//
//        // ������� �������������� ��� ������
//        FTransform StarTransform;
//        StarTransform.SetLocation(StarPosition);
//        StarTransform.SetScale3D(FVector(StarRadius));
//
//        // ��������� ������� ������ � HISM
//        StarCluster->StarMeshInstances->AddInstance(StarTransform);
//    }
//
//    // ���������� �����-����� � ����� �������� � ������ ����������
//    GeneratedWorld = AstroAnchor;
//    GeneratedStarCluster = StarCluster;
//}

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
        GenerateHomeStarSystem();
    }
}

void AAstroGenerator::GenerateGalaxiesCluster()
{

}

void AAstroGenerator::GenerateGalaxy()
{
    TSharedPtr<FGalaxyModel> GalaxyModel = MakeShared<FGalaxyModel>();

    if (bGenerateRandomGalaxy)
    {
        GalaxyGenerator->GenerateRandomGalaxyModel(GalaxyModel);
    }
    else
    {
        GalaxyModel->GalaxyClass = GalaxyGlass;
        GalaxyModel->GalaxyType = GalaxyType;
        GalaxyModel->StarsCount = GalaxyStarCount;
        GalaxyModel->StarsDensity = GalaxyStarDensity;
        GalaxyModel->GalaxySize = GalaxySize;
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
    /// TODO: Refactoring - GenerateStarSystem(StarSystemModel);
    GenerateStarSystem();

    if (GeneratedHomeStarSystem
        && GeneratedHomeStarSystem->MainStar
        && GeneratedHomeStarSystem->MainStar->PlanetarySystem)
    {

        TArray<TSharedPtr<FPlanetData>> PlanetDataMap = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetsList;

        UE_LOG(LogTemp, Warning, TEXT("Planet List - "));
        UE_LOG(LogTemp, Warning, TEXT("Planet Amount: %d"), PlanetDataMap.Num());

        /// ShowPlanetList();
        for (const TSharedPtr<FPlanetData>& PlanetDataPtr : PlanetDataMap)
        {
            if (PlanetDataPtr.IsValid())
            {
                FPlanetData PlanetData = *(PlanetDataPtr.Get());
                UE_LOG(LogTemp, Warning, TEXT("    Planet Order: %d"), PlanetData.PlanetOrder);
                UE_LOG(LogTemp, Warning, TEXT("     Orbit Radius: %f"), PlanetData.OrbitRadius);

                // �������� ������ �������
                TSharedPtr<FPlanetModel> PlanetModel = PlanetData.PlanetModel;

                // ����� ������ ������ �������
                if (PlanetModel.IsValid())
                {
                    UE_LOG(LogTemp, Warning, TEXT("     Planet Type: %s"), *UEnum::GetValueAsString(PlanetModel->PlanetType));
                    UE_LOG(LogTemp, Warning, TEXT("     Planet Zone: %s"), *UEnum::GetValueAsString(PlanetModel->PlanetZone));
                    UE_LOG(LogTemp, Warning, TEXT("     Temperature: %d"), PlanetModel->Temperature);
                    UE_LOG(LogTemp, Warning, TEXT("     Planet Density: %f"), PlanetModel->PlanetDensity);
                    UE_LOG(LogTemp, Warning, TEXT("     Planet Gravity Strength: %f"), PlanetModel->PlanetGravityStrength);
                    UE_LOG(LogTemp, Warning, TEXT("     Amount of Moons: %d"), PlanetModel->AmountOfMoons);

                    // ����� ���������� � ���������
                    TArray<TSharedPtr<FMoonData>> MoonsList = PlanetModel->MoonsList;
                    for (int32 i = 0; i < MoonsList.Num(); i++)
                    {
                        if (MoonsList[i].IsValid())
                        {
                            // �������� FMoonData �� ��������� ������ ������ ��������
                            FMoonData MoonData = *(MoonsList[i].Get());

                            // ������� ������ ��������
                            UE_LOG(LogTemp, Warning, TEXT("         Moon Order: %d"), MoonData.MoonOrder);
                            UE_LOG(LogTemp, Warning, TEXT("             Moon Orbit Radius: %f"), MoonData.OrbitRadius);

                            // �������� ������ Moon
                            TSharedPtr<FMoonModel> MoonModel = MoonData.MoonModel;

                            // ����� ������ ������ Moon
                            if (MoonModel.IsValid())
                            {
                                UE_LOG(LogTemp, Warning, TEXT("             Moon Type: %s"), *UEnum::GetValueAsString(MoonModel->Type));
                                UE_LOG(LogTemp, Warning, TEXT("             Moon Density: %f"), MoonModel->MoonDensity);
                                UE_LOG(LogTemp, Warning, TEXT("             Moon Gravity: %f"), MoonModel->MoonGravity);
                            }
                        }
                    }
                }
            }
        }

        if (bCharacterSpawnAtRandomPlanet)
        {
            if (PlanetDataMap.Num() > 0)
            {
                TSharedPtr<FPlanetData> StartPlanetData = PlanetDataMap[0];
                TSharedPtr<FPlanetModel> StartPlanetModel = StartPlanetData->PlanetModel;
                int HomePlanetIndex = StartPlanetData->PlanetOrder;

                // ����� �������
                UWorld* World = GetWorld();  
                if (World)
                {
                    HomePlanet = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetsActorsList[HomePlanetIndex];

                    FActorSpawnParameters SpawnParams;
                    SpawnParams.Owner = HomePlanet;
                    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    double StationOrbitHeight = PlanetGenerator->CalculateOrbitHeight(HomeSpaceStationOrbitHeight, StartPlanetModel->Radius);
                    FVector PlanetPosition = HomePlanet->GetActorLocation();
                    
                    HomeSpaceHeadquarters = World->SpawnActor<ASpaceHeadquarters>(BP_HomeSpaceHeadquarters, PlanetPosition, FRotator::ZeroRotator);
                    HomeSpaceHeadquarters->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform); /// CRASH PIE!!!
                    HomeSpaceHeadquarters->SetActorRelativeRotation(FRotator(0, 0, 0));
                    const double EARTH_RADIUS_CM = 637100000.0;
                    FVector Offset = FVector(0, 0, StationOrbitHeight * EARTH_RADIUS_CM);
                    HomeSpaceHeadquarters->AddActorWorldOffset(Offset);
                    
                    FVector HomeSpaceHeadquartersLocation = HomeSpaceHeadquarters->GetActorLocation();
                    FRotator HomeSpaceHeadquartersRotation = HomeSpaceHeadquarters->GetActorRotation();
                    HomeSpaceStation = World->SpawnActor<ASpaceStation>(BP_HomeSpaceStation, HomeSpaceHeadquartersLocation, HomeSpaceHeadquartersRotation, SpawnParams);
                    HomeSpaceStation->AttachToActor(HomeSpaceHeadquarters, FAttachmentTransformRules::KeepWorldTransform);
                    double HomeStationOffset = HomeSpaceStation->GravityCollisionZone->GetScaledSphereRadius() * 2;
                    HomeSpaceStation->AddActorLocalOffset(FVector(0, HomeStationOffset, 0));

                    /// Spawn HomeShipyard
                    HomeSpaceShipyard = World->SpawnActor<ASpaceShipyard>(BP_HomeSpaceShipyard, HomeSpaceHeadquartersLocation, HomeSpaceHeadquartersRotation);
                    double HomeSpaceShipyardLocationOffset = HomeSpaceShipyard->GravityCollisionZone->GetScaledSphereRadius() * 2;
                    HomeSpaceShipyard->AddActorLocalOffset(FVector(0, -HomeSpaceShipyardLocationOffset, 0));
                    HomeSpaceShipyard->AttachToActor(HomeSpaceHeadquarters, FAttachmentTransformRules::KeepWorldTransform);

                    //Spawn HomeSpaceship
                    FActorSpawnParameters SpaceshipSpawnParams;
                    SpaceshipSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    FVector HomeSpaceshipLocation = HomeSpaceShipyard->SpawnPoint->GetComponentLocation();

                    HomeSpaceshipLocation.Z += 1000;
                    ASpaceship* NewHomeSpaceship = World->SpawnActor<ASpaceship>(BP_HomeSpaceship, HomeSpaceshipLocation, HomeSpaceShipyard->GetActorRotation(), SpaceshipSpawnParams);
                    /*NewHomeSpaceship->AttachToActor(HomeSpaceShipyard, FAttachmentTransformRules::KeepWorldTransform);
                    NewHomeSpaceship->OffsetSystem = GeneratedHomeStarSystem;
                    NewHomeSpaceship->OnboardComputer->OffsetSystem = GeneratedHomeStarSystem;*/

                   // ASpaceship* NewHomeSpaceship = World->SpawnActor<ASpaceship>(BP_HomeSpaceship, HomeSpaceshipLocation, HomeSpaceShipyard->GetActorRotation(), SpaceshipSpawnParams);

                    if (NewHomeSpaceship && GeneratedHomeStarSystem)
                    {
                        NewHomeSpaceship->AttachToActor(HomeSpaceShipyard, FAttachmentTransformRules::KeepWorldTransform);
                        NewHomeSpaceship->OffsetSystem = GeneratedHomeStarSystem;

                        if (NewHomeSpaceship->OnboardComputer)
                        {
                            NewHomeSpaceship->OnboardComputer->OffsetSystem = GeneratedHomeStarSystem;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("Onboard Computer is nullptr!"));
                        }
                    }
                    else
                    {
                        if (!NewHomeSpaceship)
                        {
                            UE_LOG(LogTemp, Error, TEXT("NewHomeSpaceship is nullptr!"));
                        }

                        if (!GeneratedHomeStarSystem)
                        {
                            UE_LOG(LogTemp, Error, TEXT("GeneratedHomeStarSystem is nullptr!"));
                        }
                    }

                    FVector CharSpawnLocation{ 0 };
                    APawn* PlayerCharacter = UGameplayStatics::GetPlayerPawn(World, 0);
                    if (PlayerCharacter)
                    {
                        switch (CharSpawnPlace)
                        {
                        case ECharSpawnPlace::PlanetOrbit:
                            CharSpawnLocation = HomeSpaceStation->SpawnPoint->GetComponentLocation();
                            break;
                        case ECharSpawnPlace::PlanetSurface:
                            break;
                        case ECharSpawnPlace::MoonOrbit:
                            break;
                        case ECharSpawnPlace::MoonSurface:
                            break;
                        case ECharSpawnPlace::SpaceShip:
                        {
                            CharSpawnLocation = NewHomeSpaceship->GetActorLocation();
                        }
                            break;
                        default:
                            break;
                        }

                        UE_LOG(LogTemp, Warning, TEXT("CharSpawnLocation: %s"), *CharSpawnLocation.ToString());
                        bool bTeleportSucces = PlayerCharacter->SetActorLocation(CharSpawnLocation, false);
                        UE_LOG(LogTemp, Warning, TEXT("Teleport success: %s"), bTeleportSucces ? TEXT("True") : TEXT("False"));

                        /// relocate char to 000
                        {
                            // �������� ������� ��������� ������
                            FVector PlayerLocation = PlayerCharacter->GetActorLocation();
                            // �������� ������� ��������� AstroGenerator
                            FVector GeneratorLocation = this->GetActorLocation();
                            // ��������� ����� ��������� ��� AstroGenerator
                            FVector NewGeneratorLocation = GeneratorLocation - PlayerLocation;
                            // ������������� ����� ��������� ��� AstroGenerator
                            this->SetActorLocation(NewGeneratorLocation, false);
                            // ������������� ��������� ������ �� (0,0,0)
                            PlayerCharacter->SetActorLocation(FVector(0, 0, 0), false);
                        }
                    }
					else
					{
					    UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter is null!"));
					}

                    /*if (BP_CharacterClass)
                    {
						ACharacter* NewCharacter = World->SpawnActor<ACharacter>(BP_CharacterClass, CharSpawnLocation, FRotator::ZeroRotator, SpawnParams);
						NewCharacter->AttachToActor(HomeStation, FAttachmentTransformRules::KeepWorldTransform);
						NewCharacter->SetActorRelativeRotation(FRotator(0, 0, 0));
					}*/

                }
            }
        }
    }
}

/// TODO: Refactoring - GenerateStarSystem(StarSystemModel);
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
        UE_LOG(LogTemp, Warning, TEXT("Generate Star System!"));

        FTransform HomeSystemTransform;

        FVector HomeSystemSpawnLocation{ 0 };
        switch (HomeSystemPosition)
        {
        case EHomeSystemPosition::WorldCenter:
            HomeSystemSpawnLocation = FVector(0, 0, 0);
            break;
        case EHomeSystemPosition::RandomPosition:
        {
            // ������� HomeSystemSpawnLocation � ����������� �� ������ ����������� ������, �������� � �����.
            double RandomX = FMath::RandRange(-1000, 1000);
            double RandomY = FMath::RandRange(-1000, 1000);
            double RandomZ = FMath::RandRange(-1000, 1000);
            HomeSystemSpawnLocation = FVector(RandomX, RandomY, RandomZ); //* 1000000000;
        }
            break;
        case EHomeSystemPosition::DirectPosition: 
        {
            TArray<AActor*> AttachedActors;
            GetAttachedActors(AttachedActors);
            if (AttachedActors.Num() > 0) // ���������, ��� ���� ������������� ������
            {
                int32 RandomIndex = FMath::RandRange(0, AttachedActors.Num() - 1); // �������� ��������� ������
                if (AstroGenerationLevel == EAstroGenerationLevel::Galaxy)
                {
                    AGalaxy* GalaxyActor = Cast<AGalaxy>(AttachedActors[RandomIndex]); // ��������� ������ � ���� AGalaxy
                    if (GalaxyActor)
                    {
                        // ���� ����� �������� ����������� ������ AGalaxy, ��������� ��� HISM ���������.
                        UHierarchicalInstancedStaticMeshComponent* HISMComponent = GalaxyActor->StarMeshInstances;
                        if (HISMComponent && HISMComponent->GetInstanceCount() > 0)
                        {
                            // �������� ��������� ������ �� ��������� ��������� ���������
                            int32 RandomInstanceIndex = FMath::RandRange(0, HISMComponent->GetInstanceCount() - 1);
                            // ��������� ������������� ���������� ��������
                            FTransform InstanceTransform;
                            HISMComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
                            // ����������� ������� �� ������������� �������� ��� HomeSystemSpawnLocation
                            HomeSystemSpawnLocation = InstanceTransform.GetLocation();
                        }
                    }
                }
                else if (AstroGenerationLevel == EAstroGenerationLevel::StarCluster)
                {
                    AStarCluster* StarClusterActor = Cast<AStarCluster>(AttachedActors[RandomIndex]); // ��������� ������ � ���� AStarCluster
                    if (StarClusterActor)
                    {
                        // ���� ����� �������� ����������� ������ AStarCluster, ��������� ��� HISM ���������.
                        UHierarchicalInstancedStaticMeshComponent* HISMComponent = StarClusterActor->StarMeshInstances;

                        if (HISMComponent && HISMComponent->GetInstanceCount() > 0)
                        {
                            // �������� ��������� ������ �� ��������� ��������� ���������
                            int32 RandomInstanceIndex = FMath::RandRange(0, HISMComponent->GetInstanceCount() - 1);
                            // ��������� ������������� ���������� ��������
                            FTransform InstanceTransform;
                            HISMComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
                            // ����������� ������� �� ������������� �������� ��� HomeSystemSpawnLocation
                            HomeSystemSpawnLocation = InstanceTransform.GetLocation();
                        }
                    }
                }
            }
        }
            break;
        default:
            break;
        }
        HomeSystemTransform.SetLocation(HomeSystemSpawnLocation);
        HomeSystemTransform.SetRotation(FRotator::ZeroRotator.Quaternion());
        /*
        RandomPosition:
        get random index from hism array indexes
        get index model
        from hism index to star model map get random pair
        
        */

        // ������� ����� �������� �������
        TSharedPtr<FStarSystemModel> StarSystemModel = MakeShared<FStarSystemModel>();

        if (bRandomHomeSystem)
        {
            StarSystemGenerator->GenerateRandomStarSystemModel(StarSystemModel);
        }
        else
        {
            StarSystemGenerator->GenerateRandomStarSystemModel(StarSystemModel);
        }
        
        AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass); 
        //AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass, HomeSystemTransform);
        NewStarSystem->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        NewStarSystem->SetActorLocation(HomeSystemSpawnLocation);
        
        if (!NewStarSystem) 
        {
            UE_LOG(LogTemp, Warning, TEXT("NewStarSystem Falied!"));
            return; 
        }
        StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

        // Generate astro model
        int AmountOfStars = StarSystemModel->AmountOfStars;
        FVector LastStarLocation { 0 };
        for (int StarNumber = 0; StarNumber < AmountOfStars; StarNumber++)
        {
            TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();

            if (bRandomHomeStar)
            {
                StarGenerator->GenerateRandomStarModel(StarModel);
            }
            else
            {
                StarGenerator->GenerateRandomStarModel(StarModel);
            }

            TSharedPtr<FPlanetarySystemModel> PlanetraySystemModel = MakeShared<FPlanetarySystemModel>();
            
            if (bRandomHomeSystemType)
            {
                PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(PlanetraySystemModel, StarModel, PlanetGenerator, MoonGenerator);

            }
            else
            {
                PlanetarySystemGenerator->GeneratePlanetraySystemModelByStar(PlanetraySystemModel, StarModel, PlanetGenerator, MoonGenerator);
                //PlanetarySystemGenerator->GenerateCustomPlanetarySystem();
            }

            AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass); 
            APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);

            if (StarNumber == 0)
            {
                NewStarSystem->MainStar = NewStar;
            }

            if (!NewStar || !NewPlanetarySystem)
            {
                UE_LOG(LogTemp, Warning, TEXT("Star Falied!"));
                return; 
            }

            // Set Star full-scale
            /// TODO: PlanetarySystemGenerator->ConnectStar()
            StarGenerator->ApplyModel(NewStar, StarModel);
            PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetraySystemModel);
            NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
            NewStar->StarRadiusKM = StarModel->Radius * 696340;
            NewStar->SetPlanetarySystem(NewPlanetarySystem);
            NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
            NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
            
            // Apply material luminocity multiplier and emissive light color
            StarGenerator->ApplySpectralMaterial(NewStar, StarModel);

            // ��������� ������ ��� ������ ������
            FVector LastPlanetLocation{ 0 };
            int AmountOfPlanets = PlanetraySystemModel->AmountOfPlanets;
            for (const TSharedPtr<FPlanetData> FPlanetData : PlanetraySystemModel->PlanetsList)
            {
                APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(BP_PlanetOrbit, NewPlanetarySystem->GetActorLocation(), FRotator::ZeroRotator);
                NewPlanetOrbit->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);

                // Planet Model and generation
                TSharedPtr<FPlanetModel> PlanetModel = FPlanetData->PlanetModel;
                APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);

                PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
                NewStar->AddPlanet(NewPlanet);
                NewPlanet->SetParentStar(NewStar);

                // set planet full-scale
                NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
                FVector NewLocation = FVector(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
                NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
                NewPlanet->SetActorLocation(NewLocation);
                NewPlanetOrbit->SetActorRelativeRotation(FRotator(FMath::RandRange(-30.0, 30.0), FMath::RandRange(-360.0, 360.0), 0));

                NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
                NewPlanetarySystem->PlanetsActorsList.Add(NewPlanet);

                const double KM_TO_UE_UNIT_SCALE = 100000;
                double DiameterOfLastMoon = 0;
                FVector LastMoonLocation;
                // Generate Moons
                for (const TSharedPtr<FMoonData> MoonData : PlanetModel->MoonsList) 
                {
                    APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(BP_PlanetOrbit, NewPlanet->GetActorLocation(), FRotator::ZeroRotator);
                    NewMoonOrbit->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);

                    FVector MoonLocation = NewPlanet->GetActorLocation();
                    AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
                    MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
                    MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);
                    NewPlanet->AddMoon(NewMoon);
                    NewMoon->SetParentPlanet(NewPlanet);

                    // set moon full-scale
                    double MoonRadius = MoonData->MoonModel->Radius;
                    NewMoon->SetActorScale3D(FVector(MoonRadius * 12742000));
                    NewMoon->AddActorLocalOffset(FVector(0, ((PlanetModel->RadiusKM + (MoonData->OrbitRadius * PlanetModel->RadiusKM)) * KM_TO_UE_UNIT_SCALE) * 1, 0));
                    NewMoonOrbit->SetActorRelativeRotation(FRotator(FMath::RandRange(-360.0, 360.0), FMath::RandRange(-360.0, 360.0), FMath::RandRange(-360.0, 360.0)));
                    NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
				
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
                    SphereRadius *= 1.5;
                    NewPlanet->PlanetaryZone->SetSphereRadius(SphereRadius);
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
                NewStar->StarAffectionZoneRadius = StarSphereRadius * 1.5;
            }
            else
            {
                StarSphereRadius = FVector::Dist(NewStar->GetActorLocation(), LastPlanetLocation);
                StarSphereRadius /= NewStar->GetActorScale3D().X;
                NewStar->PlanetarySystemZone->SetSphereRadius(StarSphereRadius * 1.5);
                NewStar->StarAffectionZoneRadius = StarSphereRadius;

            }
            NewStarSystem->AddNewStar(NewStar);
            NewStarSystem->StarSystemRadius = NewStar->StarAffectionZoneRadius;
        }

        double StarSystemSphereRadius = FVector::Dist(NewStarSystem->GetActorLocation(), LastStarLocation);
        StarSystemSphereRadius /= NewStarSystem->GetActorScale3D().X;
        StarSystemSphereRadius *= 1.6;
        NewStarSystem->StarSystemZone->SetSphereRadius(StarSystemSphereRadius);
        NewStarSystem->StarSystemRadius = StarSystemSphereRadius;
        GeneratedHomeStarSystem = NewStarSystem;

        if (NewStarSystem->StarSystemRadius == 0)
        {
            double SphereRadius = NewStarSystem->MainStar->PlanetarySystemZone->GetScaledSphereRadius() * 1.5;
            NewStarSystem->StarSystemRadius = SphereRadius;
            NewStarSystem->StarSystemZone->SetSphereRadius(SphereRadius);
        }

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

            // ��������� �������� ����������
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
                // ��������� ������ �������� � �������� �������
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


        /// TO HOME SYSTEM
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Falied to get World!"));
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
    if (bGenerateFullScaledStarSystem)
    {

    }
    else
    {
        UWorld* World = GetWorld();

        TSharedPtr<FStarClusterModel> StarClusterModel = MakeShared<FStarClusterModel>();

        if (bGenerateRandomCluster)
        {
            StarClusterGenerator->GetRandomStarClusterModel(StarClusterModel);
        }
        else
        {
            StarClusterModel->StarClusterSize = StarClusterSize;
            StarClusterModel->StarClusterType = StarClusterType;
            StarClusterModel->StarClusterPopulation = StarClusterPopulation;
            StarClusterModel->StarClusterComposition = StarClusterComposition;
        }
        EStarClusterType ClusterType = StarClusterModel->StarClusterType;
        AStarCluster* NewStarCluster = World->SpawnActor<AStarCluster>(BP_StarClusterClass);
        NewStarCluster->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        /// Calculate Cluster Params
        NewStarCluster->StarAmount = StarClusterGenerator->GetStarsAmountByRange(StarClusterModel->StarClusterSize);
        NewStarCluster->StarDensity = StarClusterGenerator->GetStarClusterDensityByRange();
        NewStarCluster->ClusterBounds = StarClusterGenerator->GetStarClusterBoundsByRange(ClusterType);
        NewStarCluster->ClusterType = ClusterType;
        NewStarCluster->StarClusterComposition = StarClusterModel->StarClusterComposition;
        NewStarCluster->StarClusterPopulation = StarClusterModel->StarClusterPopulation;
        NewStarCluster->StarClusterSize = StarClusterModel->StarClusterSize;

        UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), NewStarCluster->StarAmount);
        UE_LOG(LogTemp, Warning, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
        UE_LOG(LogTemp, Warning, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
        UE_LOG(LogTemp, Warning, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

        for (size_t i = 0; i < NewStarCluster->StarAmount; i++)
        {
            // ������� ������ ������
            TSharedPtr<FStarModel> NewStarModel = MakeShared<FStarModel>();

            if (bGenerateRandomCluster)
            {
                StarGenerator->GenerateRandomStarModel(NewStarModel);
            }
            else
            {
                StarGenerator->GenerateStarModelByProbability(NewStarModel, StarClusterModel);
            }

            // ������������� ������ � ��������
            FVector StarPosition = StarClusterGenerator->CalculateStarPosition(i, NewStarCluster, NewStarModel);
            NewStarCluster->AddStarToClusterModel(StarPosition, NewStarModel);
            NewStarModel->Location = StarPosition;

            // ������� ������� ������ � ��������� ��� � HISM ���������
            FTransform StarTransform(StarPosition);
            StarTransform.SetScale3D(FVector(NewStarModel->Radius));
            int32 StarInstIndex = NewStarCluster->StarMeshInstances->AddInstance(StarTransform, true);
            FLinearColor ColorValue = StarGenerator->GetStarColor(NewStarModel->SpectralClass, NewStarModel->SpectralSubclass);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);

            double StarEmission = StarGenerator->CalculateEmission(NewStarModel->Luminosity * 25);
            NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);

            StarIndexModelMap.Add(StarInstIndex, NewStarModel);
        }

        GeneratedStarCluster = NewStarCluster;

        if (bGenerateFullScaledWorld)
        {
            //NewStarCluster->
                SetActorScale3D(FVector(1000000000, 1000000000, 1000000000));
        }
    }
    /*else
    {
        UE_LOG(LogTemp, Warning, TEXT("StarClusterGenerator is not implemented yet!"));
    }*/
}


