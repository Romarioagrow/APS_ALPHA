// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetaryEnvironmentGenerator.h"
#include "Planet.h"
#include "Moon.h"

// Sets default values
APlanetaryEnvironmentGenerator::APlanetaryEnvironmentGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APlanetaryEnvironmentGenerator::BeginPlay()
{
	Super::BeginPlay();


    if (false)
    {
        InitWorldScape(GetWorld());
    }
}

// Called every frame
void APlanetaryEnvironmentGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    

    /// if pawn.distance < planet-affectDistance * WSCScale -> Spawn planet environment
    /// if pawn.distance > planet-affectDistance * WSCScale -> Destroy planet environment
}

void APlanetaryEnvironmentGenerator::InitEnviroment(APlanet* NewPlanet, UWorld* World)
{
    InitAtmoScape(World, NewPlanet->RadiusKM, NewPlanet);

    // Generate Atmosphere Enviroment
    {
        // �������� �� ��������� (�����)
//        FAmbientParameters AmbientParams;
        /*AmbientParams.Opacity = 1.0;
        AmbientParams.MultiScattering = 1;
        AmbientParams.RayleighHeight = 8;
        AmbientParams.MieHeight = 1.2;
        AmbientParams.MiePhase = 0.5;
        AmbientParams.OzoneContribution = 0.5;
        AmbientParams.RayleighColor = FColor::White; 
        AmbientParams.MieColor = FColor::White;
        AmbientParams.AbsorptionColor = FColor::White;*/


        // Base parameters can be influenced by the planet radius
        //float RadiusFactor = NewPlanet->RadiusKM / 6371; //EARTH_RADIUS_KM;  // EARTH_RADIUS_KM is a constant representing the Earth's radius in kilometers.
        //AmbientParams.Opacity = FMath::Clamp(1.0f * RadiusFactor, 0.1f, 100.0f);
        //AmbientParams.MultiScattering = FMath::Clamp(1.0f * RadiusFactor, 0.1f, 100.0f);

        //// Below parameters are based on Earth's atmosphere. 
        //// To bring in more diversity, you could also apply some randomization or relation to planet's physical characteristics
        //// These are just for illustration and you should adjust it according to your project's need.
        //AmbientParams.RayleighHeight = FMath::Clamp(8.0f * RadiusFactor, 0.1f, 80.0f);
        //AmbientParams.MieHeight = FMath::Clamp(1.2f * RadiusFactor, 0.01f, 15.0f);
        //AmbientParams.MiePhase = FMath::Clamp(0.5f * RadiusFactor, -0.935f, 0.935f);
        //AmbientParams.OzoneContribution = FMath::Clamp(0.5f * RadiusFactor, 0.0f, 1.0f);




        /*EPlanetType PlanetType = NewPlanet->PlanetType;
        switch (PlanetType)
        {
        case EPlanetType::Terrestrial:
            break;
        case EPlanetType::Rocky:
            break;
        case EPlanetType::Greenhouse:
            break;
        case EPlanetType::Melted:
            break;
        case EPlanetType::HotGiant:
            break;
        case EPlanetType::GasGiant:
            break;
        case EPlanetType::IceGiant:
            break;
        case EPlanetType::Dwarf:
            break;
        case EPlanetType::Exoplanet:
            break;
        case EPlanetType::Rogue:
            break;
        case EPlanetType::Ocean:
            break;
        case EPlanetType::Water:
            break;
        case EPlanetType::Desert:
            break;
        case EPlanetType::Forest:
            break;
        case EPlanetType::Volcanic:
            break;
        case EPlanetType::Ice:
            break;
        case EPlanetType::Frozen:
            break;
        case EPlanetType::Ammonia:
            break;
        case EPlanetType::Iron:
            break;
        case EPlanetType::Carbon:
            break;
        case EPlanetType::SuperEarth:
            break;
        case EPlanetType::Lava:
            break;
        case EPlanetType::Metallic:
            break;
        case EPlanetType::Unknown:
            break;
        default:
            break;
        }*/
    }

    if (NewPlanet->IsNotGasGiant())
    {
        GenerateWorldscapeSurfaceByModel(World, NewPlanet);
    }
}

void APlanetaryEnvironmentGenerator::InitAtmoScape(UWorld* World, double Radius, APlanetaryBody* NewPlanetaryBody)
{
    PlanetAtmosphere = World->SpawnActor<AAtmosphere>(AAtmosphere::StaticClass(), FTransform());

    if (PlanetAtmosphere)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PlanetAtmosphere AtmoScapeInstance has been created successfully."));
        
        
        // ��������� ���������� � ������� ��� ������� Atmosphere.
        PlanetAtmosphere->PlanetRadius = Radius - 1; // Atm Dead Zone 
        PlanetAtmosphere->bKeepRelativeScale = false;
        PlanetAtmosphere->AtmosphereHeight = NewPlanetaryBody->AtmosphereHeight;
        PlanetAtmosphere->SetActorLocation(NewPlanetaryBody->GetActorLocation());
        PlanetAtmosphere->SetActorRotation(NewPlanetaryBody->GetActorRotation());
        PlanetAtmosphere->AttachToActor(NewPlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);


        float RadiusFactor = Radius / 6371;//* NewPlanetaryBody->Mass; //EARTH_RADIUS_KM;  // EARTH_RADIUS_KM is a constant representing the Earth's radius in kilometers.
        AmbientParams.Opacity = FMath::Clamp(1.0f * RadiusFactor, 0.5f, 100.0f);
        AmbientParams.MultiScatering = FMath::Clamp(1.0f * RadiusFactor, 0.5f, 100.0f);

        // Below parameters are based on Earth's atmosphere. 
        // To bring in more diversity, you could also apply some randomization or relation to planet's physical characteristics
        // These are just for illustration and you should adjust it according to your project's need.
        AmbientParams.RayleighHeight = FMath::Clamp(8.0f * RadiusFactor, 1.1f, 80.0f);
        AmbientParams.MieHeight = FMath::Clamp(1.2f * RadiusFactor, 0.01f, 15.0f);
        AmbientParams.MiePhase = FMath::Clamp(0.5f * RadiusFactor, -0.935f, 0.935f);
        AmbientParams.OzoneContribution = FMath::Clamp(0.5f * RadiusFactor, 0.0f, 1.0f);

        float AtmCoeff = 3.0f;
        PlanetAtmosphere->AtmosphereOpacity = AmbientParams.Opacity * AtmCoeff;
        PlanetAtmosphere->MultiScatering = AmbientParams.MultiScatering * AtmCoeff;
        PlanetAtmosphere->RayleighHeight = AmbientParams.RayleighHeight * AtmCoeff;
        PlanetAtmosphere->MieHeight = AmbientParams.MieHeight * AtmCoeff;
        PlanetAtmosphere->MiePhase = AmbientParams.MiePhase * AtmCoeff;
        PlanetAtmosphere->OzoneContribution = AmbientParams.OzoneContribution; //* AtmCoeff;

        FLinearColor MinColor{};// = AmbientParams->RayleighColor;  // Replace with actual values
        FLinearColor MaxColor{};// AmbientParams->RayleighColor;
        APlanet* NewPlanet = Cast<APlanet>(NewPlanetaryBody);
        if (NewPlanet)
        {

            EPlanetType PlanetType = NewPlanet->PlanetType;
            switch (PlanetType)
            {
            case EPlanetType::Terrestrial:
                MinColor = FLinearColor(0.066f, 0.5f, 0.529f, 1.0f);  // Replace with actual values
                MaxColor = FLinearColor(0.4f, 0.2f, 0.1f, 1.0f);
                break;
            case EPlanetType::Rocky:
                MinColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);  // Replace with actual values
                MaxColor = FLinearColor(0.5f, 0.3f, 0.2f, 1.0f);
                break;
            case EPlanetType::Greenhouse:
                MinColor = FLinearColor(0.6f, 0.6f, 0.1f, 1.0f);
                MaxColor = FLinearColor(0.8f, 0.8f, 0.3f, 1.0f);
                break;
            case EPlanetType::Melted:
                MinColor = FLinearColor(0.6f, 0.2f, 0.0f, 1.0f);
                MaxColor = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f);
                break;
            case EPlanetType::HotGiant:
                MinColor = FLinearColor(0.7f, 0.2f, 0.0f, 1.0f);
                MaxColor = FLinearColor(1.0f, 0.4f, 0.0f, 1.0f);
                break;
            case EPlanetType::GasGiant:
                MinColor = FLinearColor(0.5f, 0.4f, 0.3f, 1.0f);
                MaxColor = FLinearColor(0.6f, 0.5f, 0.4f, 1.0f);
                break;
            case EPlanetType::IceGiant:
                MinColor = FLinearColor(0.0f, 0.0f, 0.6f, 1.0f);
                MaxColor = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
                break;
            case EPlanetType::Dwarf:
                MinColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
                MaxColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
                break;
            case EPlanetType::Exoplanet:
                MinColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
                MaxColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
                break;
            case EPlanetType::Rogue:
                MinColor = FLinearColor(0.02f, 0.02f, 0.02f, 1.0f);  // Replace with actual values
                MaxColor = FLinearColor(0.02f, 0.02f, 0.02f, 1.0f);
                break;
            case EPlanetType::Ocean:
                MinColor = FLinearColor(0.0f, 0.0f, 0.7f, 1.0f);
                MaxColor = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
                break;
            case EPlanetType::Water:
                MinColor = FLinearColor(0.0f, 0.3f, 0.7f, 1.0f);
                MaxColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);
                break;
            case EPlanetType::Desert:
                MinColor = FLinearColor(0.7f, 0.5f, 0.2f, 1.0f);
                MaxColor = FLinearColor(1.0f, 0.7f, 0.3f, 1.0f);
                break;
            case EPlanetType::Forest:
                MinColor = FLinearColor(0.0f, 0.6f, 0.0f, 1.0f);
                MaxColor = FLinearColor(0.2f, 1.0f, 0.2f, 1.0f);
                break;
            case EPlanetType::Volcanic:
                MinColor = FLinearColor(0.7f, 0.0f, 0.0f, 1.0f);
                MaxColor = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f);
                break;
            case EPlanetType::Ice:
                MinColor = FLinearColor(0.8f, 0.8f, 1.0f, 1.0f);
                MaxColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            case EPlanetType::Frozen:
                MinColor = FLinearColor(0.7f, 0.8f, 1.0f, 1.0f);
                MaxColor = FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);
                break;
            case EPlanetType::Ammonia:
                MinColor = FLinearColor(0.7f, 0.7f, 0.0f, 1.0f);
                MaxColor = FLinearColor(0.9f, 0.9f, 0.2f, 1.0f);
                break;
            case EPlanetType::Iron:
                MinColor = FLinearColor(0.6f, 0.5f, 0.4f, 1.0f);
                MaxColor = FLinearColor(0.7f, 0.6f, 0.5f, 1.0f);
                break;
            case EPlanetType::Carbon:
                MinColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);
                MaxColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
                break;
            case EPlanetType::SuperEarth:
                MinColor = FLinearColor(0.0f, 0.4f, 0.6f, 1.0f);
                MaxColor = FLinearColor(0.2f, 0.6f, 0.8f, 1.0f);
                break;
            case EPlanetType::Lava:
                MinColor = FLinearColor(0.7f, 0.2f, 0.0f, 1.0f);
                MaxColor = FLinearColor(1.0f, 0.4f, 0.0f, 1.0f);
                break;
            case EPlanetType::Metallic:
                MinColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
                MaxColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
                break;
            case EPlanetType::Unknown:
                MinColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
                MaxColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
                break;
            default:
                // Default case is to assign the colors to blue atmosphere.
                MinColor = FLinearColor(3.800000, 13.500000, 33.099998, 0.000000);
                MaxColor = FLinearColor(3.800000, 13.500000, 33.099998, 0.000000);
                break;
            }

            //PlanetAtmosphere->RayleighScattering = MinColor;
            /*FLinearColor RandomColor;

            RandomColor.R = FMath::RandRange(MinColor.R, MaxColor.R);
            RandomColor.G = FMath::RandRange(MinColor.G, MaxColor.G);
            RandomColor.B = FMath::RandRange(MinColor.B, MaxColor.B);
            RandomColor.A = FMath::RandRange(MinColor.A, MaxColor.A);

            PlanetAtmosphere->RayleighScattering = RandomColor;*/
        }
        else
        {
            AMoon* NewMoon = Cast<AMoon>(NewPlanetaryBody);
            if (NewMoon)
            {
                EMoonType MoonType = NewMoon->MoonType;
                switch (MoonType)
                {
                case EMoonType::Continental:
                    MinColor = FLinearColor(0.4f, 0.7f, 1.0f, 1.0f);
                    MaxColor = FLinearColor(0.6f, 0.9f, 1.0f, 1.0f);
                    break;
                case EMoonType::Desert:
                    MinColor = FLinearColor(0.8f, 0.6f, 0.3f, 1.0f);
                    MaxColor = FLinearColor(1.0f, 0.8f, 0.5f, 1.0f);
                    break;
                case EMoonType::Gas:
                    MinColor = FLinearColor(0.4f, 0.6f, 0.8f, 1.0f);
                    MaxColor = FLinearColor(0.6f, 0.8f, 1.0f, 1.0f);
                    break;
                case EMoonType::Icy:
                    MinColor = FLinearColor(0.6f, 0.8f, 1.0f, 1.0f);
                    MaxColor = FLinearColor(0.8f, 0.9f, 1.0f, 1.0f);
                    break;
                case EMoonType::Iron:
                    MinColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
                    MaxColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
                    break;
                case EMoonType::Ocean:
                    MinColor = FLinearColor(0.0f, 0.3f, 0.7f, 1.0f);
                    MaxColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);
                    break;
                case EMoonType::Peculiar:
                    MinColor = FLinearColor(0.5f, 0.3f, 0.7f, 1.0f);
                    MaxColor = FLinearColor(0.7f, 0.5f, 0.9f, 1.0f);
                    break;
                case EMoonType::Volcanic:
                    MinColor = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f);
                    MaxColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
                    break;
                case EMoonType::Rocky:
                    MinColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
                    MaxColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
                    break;
                case EMoonType::Unknown:
                    MinColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
                    MaxColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
                    break;
                default:
                    // Default case is to assign the colors to blue atmosphere.
                    MinColor = FLinearColor(3.800000, 13.500000, 33.099998, 0.000000);
                    MaxColor = FLinearColor(3.800000, 13.500000, 33.099998, 0.000000);
                    break;
                }


            }
        }

        FLinearColor RandomColor;

        RandomColor.R = FMath::RandRange(MinColor.R, MaxColor.R);
        RandomColor.G = FMath::RandRange(MinColor.G, MaxColor.G);
        RandomColor.B = FMath::RandRange(MinColor.B, MaxColor.B);
        RandomColor.A = FMath::RandRange(MinColor.A, MaxColor.A);

        PlanetAtmosphere->RayleighScattering = RandomColor;
    }
}


void APlanetaryEnvironmentGenerator::InitWorldScape(UWorld* World)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("InitWorldScape!"));
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->bGenerateWorldScape = true;

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldScape!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }
}

void APlanetaryEnvironmentGenerator::GenerateWorldscapeSurfaceByModel(UWorld* World, APlanet* NewPlanet)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldscapeSurfaceByModel!"));
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {
        PlanetaryBody = NewPlanet;

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        double PlanetRadiusKM = NewPlanet->RadiusKM;

        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->PlanetScale = PlanetRadiusKM * 100000;
        WorldScapeRootInstance->DistanceToFreezeGeneration = PlanetRadiusKM * 1000000;


        WorldScapeRootInstance->SetActorLocation(NewPlanet->GetActorLocation());
        WorldScapeRootInstance->SetActorRotation(NewPlanet->GetActorRotation());
        WorldScapeRootInstance->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);
        //NewPlanet->Mesh
        //NewPlanet->SetupWorldScapeRoot(WorldScapeRootInstance);

        //WorldScapeRootInstance->bGenerateWorldScape = true;
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldScape!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }

}
void APlanetaryEnvironmentGenerator::GenerateWorldscapeSurfaceByModel(UWorld* World, AMoon* NewMoon)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldscapeSurfaceByModel!"));
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        PlanetaryBody = NewMoon;

        double PlanetRadiusKM = NewMoon->RadiusKM;

        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->PlanetScale = PlanetRadiusKM * 100000;
        WorldScapeRootInstance->DistanceToFreezeGeneration = PlanetRadiusKM;


        WorldScapeRootInstance->SetActorLocation(NewMoon->GetActorLocation());
        WorldScapeRootInstance->SetActorRotation(NewMoon->GetActorRotation());
        WorldScapeRootInstance->AttachToActor(NewMoon, FAttachmentTransformRules::KeepWorldTransform);
        //NewPlanet->Mesh
        //NewPlanet->SetupWorldScapeRoot(WorldScapeRootInstance);

        //WorldScapeRootInstance->bGenerateWorldScape = true;
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("GenerateWorldScape!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }

}

void APlanetaryEnvironmentGenerator::SpawnPlanetEnvironment()
{
    if (WorldScapeRootInstance)
    {

        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("SpawnPlanetEnvironment!"));
        WorldScapeRootInstance->bGenerateWorldScape = true;
        WorldScapeRootInstance->SetActorHiddenInGame(false);    
        WorldScapeRootInstance->SetActorTickEnabled(true);
        WorldScapeRootInstance->SetActorEnableCollision(true);

        if (WorldScapeRootInstance->GetAttachParentActor() != PlanetaryBody)
        {
            WorldScapeRootInstance->SetActorLocation(FVector(0.0, 0.0, 0.0));
            WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("WorldScapeRootInstance respawned!"));
        }

        /*if (WorldScapeRootInstance->GetAttachParentActor() != PlanetaryBody)
        {
            WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);
            WorldScapeRootInstance->SetActorLocation(FVector(0.0, 0.0, 0.0));
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("WorldScapeRootInstance respawned!"));

        }*/

        //if not attached 
       /* if (WorldScapeRootInstance->GetAttachParent() != PlanetaryBody)
        {
            WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);
        }
        WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);
        WorldScapeRootInstance->SetActorLocation(FVector(0.0));*/
    }
}

void APlanetaryEnvironmentGenerator::DestroyPlanetEnvironment()
{
    if (WorldScapeRootInstance)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Magenta, TEXT("DestroyPlanetEnvironment!"));
        WorldScapeRootInstance->bGenerateWorldScape = false;
        //WorldScapeRootInstance->SetHidden(true);// = true;
        WorldScapeRootInstance->SetActorHiddenInGame(true);
        WorldScapeRootInstance->SetActorTickEnabled(false);
        WorldScapeRootInstance->SetActorEnableCollision(false);

        WorldScapeRootInstance->DetachRootComponentFromParent();
    }

}

//void APlanetEnvironmentGenerator::SetupDefaultGenerator()
//{
//    WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
//    WorldScapeRootInstance->bGenerateWorldScape = true;
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("SetupDefaultGenerator!"));
//}