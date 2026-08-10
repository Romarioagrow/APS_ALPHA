#include "PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
APlanetarySurfaceGenerator::APlanetarySurfaceGenerator()
{
	// Normally dormant. Tick is enabled only while an owned WorldScape root is
	// draining background generation jobs before a safe deferred destroy.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RuntimeSurfaceGeneratorRoot"));

    /*MoonLikeNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/MoonLike.MoonLike"));
    LavaWorldNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/LavaWorld.LavaWorld"));
    SelenaeNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    EarthLikeNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/EarthLike.EarthLike"));
    EarthNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/EarthNoise.EarthNoise"));
    IceWorldNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/IceWorld.IceWorld"));

    MI_Terra = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Terra.MI_Terra"));
    MI_Selenae = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Selenae.MI_Selenae"));
    MI_Magma = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Magma.MI_Magma"));
    MI_Planetary_Ocean = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/MI_Planetary_Ocean.MI_Planetary_Ocean"));
    M_Lava_WorldScape = LoadObject<UMaterial>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Lava_WorldScape.M_Lava_WorldScape"));*/

}

// Called when the game starts or when spawned
void APlanetarySurfaceGenerator::BeginPlay()
{
	Super::BeginPlay();
    //MoonLikeNoise = StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/WorldScape/Ressources/Noise/MoonLike.MoonLike"));
    //MoonLikeNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/WorldScape/Ressources/Noise/MoonLike.MoonLike")));


    //MoonLikeNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/MoonLike.MoonLike")));
    //LavaWorldNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/LavaWorld.LavaWorld")));
    //SelenaeNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/Selenae.Selenae")));
    //SelenaeMetalNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/SelenaeMetal.SelenaeMetal")));
    //EarthLikeNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/EarthLike.EarthLike")));
    //EarthNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/EarthNoise.EarthNoise")));
    //TerraNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/TerraNoise.TerraNoise")));
    //IceWorldNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/IceWorldNoise.IceWorldNoise")));
    //TerraDesert = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/TerraDesert.TerraDesert")));
    //TerraForestNoise = Cast<UWorldScapeNoiseClass>(StaticLoadObject(UWorldScapeNoiseClass::StaticClass(), nullptr, TEXT("/Game/APS/WSC/TerraForestNoise.TerraForestNoise")));

    //// Materials
    //MI_Terra = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, TEXT("/Game/APS/WSC/WorldScapeMaterials/MaterialInstances/MI_Terra.MI_Terra")));
    //MI_Selenae = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, TEXT("/Game/APS/WSC/WorldScapeMaterials/MaterialInstances/MI_Selenae.MI_Selenae")));
    //MI_Magma = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, TEXT("/Game/APS/WSC/WorldScapeMaterials/MaterialInstances/MI_Magma.MI_Magma")));
    //MI_Planetary_Ocean = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, TEXT("/Game/APS/WSC/WorldScapeMaterials/Ocean/MI_Planetary_Ocean.MI_Planetary_Ocean")));
    //MI_Lava_Ocean = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, TEXT("/Game/APS/WSC/WorldScapeMaterials/Ocean/MI_Lava_Ocean.MI_Lava_Ocean")));



    //MoonLikeNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/MoonLike.MoonLike"));
    //LavaWorldNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/LavaWorld.LavaWorld"));
    //SelenaeNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    //SelenaeMetalNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    //EarthLikeNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    //EarthNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    //TerraNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    //IceWorldNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    //TerraDesert = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    //TerraForestNoise = LoadObject<UWorldScapeNoiseClass>(nullptr, TEXT("/WorldScape/Ressources/Noise/Selenae.Selenae"));
    ////...
    //MI_Terra = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Terra.MI_Terra"));
    //MI_Selenae = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Selenae.MI_Selenae"));
    //MI_Magma = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Selenae.MI_Selenae"));
    //MI_Planetary_Ocean = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/MI_Planetary_Ocean.MI_Planetary_Ocean"));
    //MI_Lava_Ocean = LoadObject<UMaterialInstance>(nullptr, TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Selenae.MI_Selenae"));

    if (false)
    {
        InitWorldScape(GetWorld());
    }
}

// Called every frame
void APlanetarySurfaceGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TryFinalizeSurfaceProfileApply();
	TryFinalizeWorldScapeUnload();
}

void APlanetarySurfaceGenerator::InitEnviroment(APlanet* NewPlanet, UWorld* World)
{
    /// TO DO: ONE COMMON SWITCH FOR EnviromentModel

    InitAtmoScape(World, NewPlanet->RadiusKM, NewPlanet);

    // Generate Atmosphere Enviroment
    {
        // Значения по умолчанию (Земля)
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
        //GenerateWorldscapeSurfaceByModel(World, NewPlanet);
    }
}

void APlanetarySurfaceGenerator::InitAtmoScape(UWorld* World, double PlanetaryRadiusKM, APlanetaryBody* NewPlanetaryBody)
{
	if (!IsValid(World) || !IsValid(NewPlanetaryBody))
	{
		return;
	}

	// InitEnvironment is reached by both generated and editor-authored integration
	// paths. Re-entering it must update the same shell, not stack another opaque
	// atmosphere over the first one.
	if (!IsValid(PlanetAtmosphere))
	{
		PlanetAtmosphere = World->SpawnActor<AAtmoScape>(AAtmoScape::StaticClass(), FTransform());
	}

    if (PlanetAtmosphere)
    {
		PlanetAtmosphere->SetActorHiddenInGame(false);
        
        
        // Установка параметров и свойств для объекта Atmosphere.
		const double SafeRadiusKm = FMath::Max(PlanetaryRadiusKM, 1.0);
		PlanetAtmosphere->PlanetRadius = FMath::Max(SafeRadiusKm - 1.0, 0.5); // Atm dead zone.
        PlanetAtmosphere->bKeepRelativeScale = false;
        PlanetAtmosphere->AtmosphereHeight = NewPlanetaryBody->AtmosphereHeight;
        PlanetAtmosphere->SetActorLocation(NewPlanetaryBody->GetActorLocation());
        PlanetAtmosphere->SetActorRotation(NewPlanetaryBody->GetActorRotation());
        PlanetAtmosphere->AttachToActor(NewPlanetaryBody, FAttachmentTransformRules::KeepWorldTransform);


        float RadiusFactor = PlanetaryRadiusKM / 6371;//* NewPlanetaryBody->Mass; //EARTH_RADIUS_KM;  // EARTH_RADIUS_KM is a constant representing the Earth's radius in kilometers.
        AmbientParams.Opacity = FMath::Clamp(10.0f * RadiusFactor, 0.5f, 80.0f);
        AmbientParams.MultiScatering = FMath::Clamp(10.0f * RadiusFactor, 0.5f, 80.0f);

        // Below parameters are based on Earth's atmosphere. 
        // To bring in more diversity, you could also apply some randomization or relation to planet's physical characteristics
        // These are just for illustration and you should adjust it according to your project's need.
        AmbientParams.RayleighHeight = FMath::Clamp(8.0f * RadiusFactor, 4.1f, 80.0f);
        AmbientParams.MieHeight = FMath::Clamp(1.2f * RadiusFactor, 0.01f, 15.0f);
        AmbientParams.MiePhase = FMath::Clamp(0.5f * RadiusFactor, -0.935f, 0.935f);
        AmbientParams.OzoneContribution = FMath::Clamp(0.5f * RadiusFactor, 0.0f, 1.0f);

        float AtmCoeff = 1.0f;
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
            case EPlanetType::Metal:
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
		// AtmoScape otherwise keeps its constructor scale until the first tick. On a
		// full-scale body that produces an invisible first frame (and can briefly use
		// the old relative-radius coefficients). Apply the physical km->cm scale now.
		PlanetAtmosphere->UpdateScale();

		// AtmoScape's separate absorption sphere uses a modulate material and is not
		// managed by the plugin's inside/outside visibility switch. At full scale it
		// therefore becomes a second, planet-sized dark cap across the sky. The main
		// atmosphere material already consumes the absorption coefficients, so retain
		// the physical scattering shell and explicitly suppress the duplicate layer.
		//
		// The inside-view outer-airglow material is another screen-space sphere pass.
		// Its flipped ray/sphere mask and warm InsideColor produce a hard circular cap
		// when the ground camera changes tangent frame. It is never selected by
		// AtmoScape's outside-camera branch, and menu preview independently selects only
		// the proper space shell, so suppress it only on a full-scale gameplay body.
		const bool bFullScaleGameplayBody = World->IsGameWorld()
			&& FMath::IsNearlyEqual(NewPlanetaryBody->WorldScapePresentationScale, 1.0);
		TInlineComponentArray<UStaticMeshComponent*> AtmosphereMeshes;
		PlanetAtmosphere->GetComponents(AtmosphereMeshes);
		for (UStaticMeshComponent* AtmosphereMesh : AtmosphereMeshes)
		{
			if (!IsValid(AtmosphereMesh))
			{
				continue;
			}
			const FString ComponentName = AtmosphereMesh->GetName();
			const bool bDuplicateAbsorption = ComponentName.Contains(
				TEXT("PlanetaryAbsorptionMesh"));
			const bool bGroundOuterAirglow = bFullScaleGameplayBody
				&& ComponentName.Contains(TEXT("PlanetarOutterMesh"));
			if (!bDuplicateAbsorption && !bGroundOuterAirglow)
			{
				continue;
			}
			AtmosphereMesh->SetVisibility(false, true);
			AtmosphereMesh->SetHiddenInGame(true, true);
			AtmosphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AtmosphereMesh->SetGenerateOverlapEvents(false);
		}
    }
}


void APlanetarySurfaceGenerator::InitWorldScape(UWorld* World)
{
    FActorSpawnParameters SpawnParams;
    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);

    if (WorldScapeRootInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape WorldScapeRootInstance has been created successfully."));

        WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
        WorldScapeRootInstance->bGenerateWorldScape = true;

    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InitWorldScape Failed to create WorldScapeRootInstance."));
    }
}

//void APlanetaryEnvironmentGenerator::GenerateWorldscapeSurfaceByModel(UWorld* World, AMoon* NewMoon)
//{
//    FActorSpawnParameters SpawnParams;
//    WorldScapeRootInstance = World->SpawnActor<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(), FTransform(), SpawnParams);
//
//    WorldScapeRootInstance->LodResolution = 200;
//    WorldScapeRootInstance->TriangleSize = 75;
//    WorldScapeRootInstance->HeightAnchor = 50000.0;
//    WorldScapeRootInstance->WorldScapeNoise;
//
//
//}

void APlanetarySurfaceGenerator::GenerateWorldscapeSurfaceByModel(UWorld* World, APlanet* NewPlanet)
{
	// Validate input parameters
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateWorldscapeSurfaceByModel: World parameter is null!"));
		return;
	}
	
	if (!NewPlanet)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateWorldscapeSurfaceByModel: NewPlanet parameter is null!"));
		return;
	}
	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(NewPlanet->PlanetType))
	{
		return;
	}

	// Compatibility entry point for old C++ callers. Solid planets are always
	// routed through the same resolver-driven pipeline as menu preview and runtime
	// streaming.
	if (!IsValid(WorldScapeRootInstance) && !CreateRuntimeWorldScapeRoot(NewPlanet))
	{
		return;
	}
	PlanetaryBody = NewPlanet;
	ApplySurfaceProfile(NewPlanet);
}
void APlanetarySurfaceGenerator::GenerateWorldscapeSurfaceByModel(UWorld* World, AMoon* NewMoon)
{
	// Validate input parameters
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateWorldscapeSurfaceByModel: World parameter is null!"));
		return;
	}
	
	if (!NewMoon)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateWorldscapeSurfaceByModel: NewMoon parameter is null!"));
		return;
	}
	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(NewMoon->PlanetType))
	{
		return;
	}

	// Moons edited from the astronomical generator use the same EPlanetType-based
	// profile as solid planets. Never restore the legacy EarthLikeNoise/MI_Terra
	// defaults here: that would overwrite the resolver immediately after a selected
	// moon was regenerated and make its surface controls appear ineffective.
	if (!IsValid(WorldScapeRootInstance) && !CreateRuntimeWorldScapeRoot(NewMoon))
	{
		return;
	}
	PlanetaryBody = NewMoon;
	ApplySurfaceProfile(NewMoon);
}

void APlanetarySurfaceGenerator::SpawnWorldScapeRoot()
{
    if (WorldScapeRootInstance)
    {
		if (bPendingSurfaceProfileApply)
		{
			DeferredWorldScapeRootState = EDeferredWorldScapeRootState::Active;
			HoldWorldScapeRootForProfileDrain();
			return;
		}
		// The player may return while a distant-family unload is waiting for the
		// last worker. Reactivation owns the root again and must cancel that destroy.
		bPendingWorldScapeUnload = false;
		bDestroyWorldScapeRootAfterDrain = false;
		SetActorTickEnabled(false);
		WorldScapeRootInstance->SetActorScale3D(FVector::OneVector);
        WorldScapeRootInstance->bGenerateWorldScape = true;
		WorldScapeRootInstance->bFreezeGeneration = false;
		// WorldScape's WITH_EDITOR UpdatePosition path treats a PIE viewport as an
		// editor camera and replaces bGenerateCollision with this second flag.  The
		// runtime observer override only replaces the position afterwards; it does
		// not restore the collision decision. A normal PIE PlayWorld uses the runtime
		// branch, but editor-viewport previews and automation can still enter that
		// editor path. Keep both switches authoritative for an active root, and ask
		// WorldScape to include the possessed pawn in every supported net mode.
		// Preload/frozen states still disable actor collision below their lifecycle
		// boundary, so inactive siblings and ocean presentation meshes stay non-solid.
		WorldScapeRootInstance->bGenerateCollision = true;
		WorldScapeRootInstance->bGenerateCollisionForAllPlayer = true;
#if WITH_EDITOR
		WorldScapeRootInstance->bGenerateCollisionInEditor = true;
		WorldScapeRootInstance->bStaticCollisionInEditor = false;
#endif
        WorldScapeRootInstance->SetActorHiddenInGame(false);    
        WorldScapeRootInstance->SetActorTickEnabled(true);
        WorldScapeRootInstance->SetActorEnableCollision(true);

        if (WorldScapeRootInstance->GetAttachParentActor() != PlanetaryBody)
        {
            WorldScapeRootInstance->SetActorLocation(FVector(0.0, 0.0, 0.0));
            WorldScapeRootInstance->AttachToActor(PlanetaryBody, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
		WorldScapeRootInstance->SetActorScale3D(FVector::OneVector);
		// Never call WS_ForceRegenerate here. It destroys every existing LOD
		// immediately, which is both unnecessary for a resident family and the exact
		// lifetime hazard behind LodGenerationThread::DoWork -> SetData crashes. A new
		// root has init=false and builds its base mesh on its first enabled tick; a
		// resident root only needs to resume its existing queue/LOD set.
    }
}

void APlanetarySurfaceGenerator::DestroyPlanetEnvironment()
{
	// Compatibility entry point for old Blueprint/C++ callers. Going through the
	// drain-aware path is mandatory: bGenerateWorldScape=false makes WorldScape
	// destroy its LOD components on the next tick, while background workers may
	// still be writing those exact UWorldScapeLod objects.
	UnloadWorldScapeRoot();

}

//void APlanetEnvironmentGenerator::SetupDefaultGenerator()
//{
//    WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
//    WorldScapeRootInstance->bGenerateWorldScape = true;
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("SetupDefaultGenerator!"));
//}
