#include "PlanetGenerator.h"

#include "PlanetaryAtmosphere.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Structs/PlanetAtmosphereModel.h"

namespace
{
	constexpr double PlanetGeneratorEarthRadiusKm = 6371.0;

	double GetFallbackPlanetDensity(const EPlanetType PlanetType)
	{
		switch (PlanetType)
		{
		case EPlanetType::GasGiant:
		case EPlanetType::HotGiant:
			return 1.2;
		case EPlanetType::IceGiant:
			return 2.2;
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Ammonia:
			return 2.8;
		case EPlanetType::Metal:
		case EPlanetType::Metallic:
			return 7.0;
		default:
			return 5.0;
		}
	}

	void EnsurePlanetPhysicalProperties(FPlanetModel& Model)
	{
		if (!FMath::IsFinite(Model.Radius) || Model.Radius <= UE_DOUBLE_SMALL_NUMBER)
		{
			Model.Radius = static_cast<float>(FMath::Max(
				static_cast<double>(Model.RadiusKM) / PlanetGeneratorEarthRadiusKm, 0.01));
		}
		if (!FMath::IsFinite(Model.PlanetDensity)
			|| Model.PlanetDensity <= UE_DOUBLE_SMALL_NUMBER)
		{
			Model.PlanetDensity = GetFallbackPlanetDensity(Model.PlanetType);
		}
		if (!FMath::IsFinite(Model.Mass) || Model.Mass <= UE_DOUBLE_SMALL_NUMBER)
		{
			Model.Mass = Model.PlanetDensity * (4.0 / 3.0) * UE_PI
				* FMath::Pow(static_cast<double>(Model.Radius), 3.0);
		}
		if (!FMath::IsFinite(Model.PlanetGravityStrength)
			|| Model.PlanetGravityStrength <= UE_DOUBLE_SMALL_NUMBER)
		{
			Model.PlanetGravityStrength = Model.Mass
				/ FMath::Square(FMath::Max(static_cast<double>(Model.Radius), 0.01));
		}
	}
}

UPlanetGenerator::UPlanetGenerator()
{
}

TArray<FMoonData> UPlanetGenerator::GenerateMoonsList(FPlanetModel PlanetModel)
{
	return TArray<FMoonData>();
}

double UPlanetGenerator::CalculateOrbitHeight(EOrbitHeight OrbitHeightType, double PlanetRadius)
{
	switch (OrbitHeightType)
	{
	case EOrbitHeight::UpperAtmosphere:
		return PlanetRadius * 1.01f;
	case EOrbitHeight::LowOrbit:
		return PlanetRadius * 1.1f;
	case EOrbitHeight::Geostationary:
		return PlanetRadius * 1.2f;
	case EOrbitHeight::HighOrbit:
		return PlanetRadius * 2.0f;
	case EOrbitHeight::VeryHighOrbit:
		return PlanetRadius * 5.0f;
	default:
		return PlanetRadius;
	}
}

void UPlanetGenerator::CalculateLagrangePoints()
{
	// claculate points
	/*FVector L1_Position = FVector(NewLocation.X * (1 - pow(PlanetModel->Mass / 3, 1.0 / 3.0)), 0, 0) + MoonData->MoonOrder+1;
	                FVector L2_Position = FVector(NewLocation.X * (1 + pow(PlanetModel->Mass / 3, 1.0 / 3.0)), 0, 0) + MoonData->MoonOrder + 1;
	                FVector L3_Position = FVector(-NewLocation.X * (1 + 5 * PlanetModel->Mass / 12), 0, 0) + MoonData->MoonOrder + 1;
	                FVector L4_Position = FVector(NewLocation.X * cos(PI / 3), NewLocation.X * sin(PI / 3), 0) + MoonData->MoonOrder + 1;
	                FVector L5_Position = FVector(NewLocation.X * cos(PI / 3), -NewLocation.X * sin(PI / 3), 0) + MoonData->MoonOrder + 1;
	                PlanetModel->LagrangePoints.Add(L1_Position);
	                PlanetModel->LagrangePoints.Add(L2_Position);
	                PlanetModel->LagrangePoints.Add(L3_Position);
	                PlanetModel->LagrangePoints.Add(L4_Position);
	                PlanetModel->LagrangePoints.Add(L5_Position);*/


	/// spawn L points

	//for (const FVector& LagrangePoint : PlanetModel->LagrangePoints)
	//{
	//    UClass* BPAstroActorClass = LoadClass<AAstroActor>(nullptr, TEXT("/Game/APS/Core/BP_AstroActor.BP_AstroActor_C"));
	//    if (BPAstroActorClass)
	//    {
	//        FActorSpawnParameters SpawnParams;
	//        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//        AAstroActor* NewAstroActor = GetWorld()->SpawnActor<AAstroActor>(BPAstroActorClass, LagrangePoint, FRotator::ZeroRotator, SpawnParams);

	//        // Если вы хотите прикрепить его к планете
	//        if (NewAstroActor && NewPlanet)
	//        {
	//            NewAstroActor->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);
	//        }
	//        else
	//        {
	//            UE_LOG(LogTemp, Warning, TEXT("Lagrange point actor or planet is null"));
	//        }
	//    }
	//}
}

APlanet* UPlanetGenerator::GeneratePlanet(const TSharedPtr<FPlanetModel>& PlanetModel,
                                          const TSubclassOf<APlanet> PlanetClass, UWorld* World)
{
	if (!World || !PlanetModel || !PlanetClass)
	{
		return nullptr;
	}

	APlanet* NewPlanet = World->SpawnActor<APlanet>(PlanetClass);
	if (NewPlanet)
	{
		ApplyModel(NewPlanet, PlanetModel);
		const double RadiusInCm = PlanetModel->RadiusKM * KM_TO_CM * SCALE_FACTOR;
		NewPlanet->SetActorScale3D(FVector(RadiusInCm));
		NewPlanet->PlanetRadiusKM = FMath::RoundToInt(PlanetModel->RadiusKM);

		
	}
	return NewPlanet;
}

TSharedPtr<FPlanetModel> UPlanetGenerator::CreatePlanetModelFromGeneratedWorld(const UGeneratedWorld* GeneratedWorld)
{
	if (!GeneratedWorld)
	{
		return nullptr;
	}

	TSharedPtr<FPlanetModel> PlanetModel = MakeShared<FPlanetModel>();
	PlanetModel->PlanetType = GeneratedWorld->PlanetType;
	PlanetModel->AmountOfMoons = GeneratedWorld->MoonsAmount;
	// GeneratedWorld exposes the radius to the menu in kilometres, while the
	// procedural astronomy model stores Radius in Earth-radius units. Feeding
	// the UI value directly into Radius made a 6,750 km planet 6,750 Earth radii
	// wide and broke both the preview framing and generated system spacing.
	constexpr double EarthRadiusKm = 6371.0;
	PlanetModel->RadiusKM = FMath::Max(1.0, static_cast<double>(GeneratedWorld->PlanetRadius));
	PlanetModel->Radius = PlanetModel->RadiusKM / EarthRadiusKm;
	PlanetModel->AtmosphereHeight = FMath::Max(0.0, GeneratedWorld->AtmosphereHeight);
	PlanetModel->SurfaceSeed = FMath::Max(0, GeneratedWorld->PlanetSurfaceSeed);
	PlanetModel->SurfaceFeatureScale = FMath::Clamp(GeneratedWorld->SurfaceFeatureScale, 0.25, 4.0);
	PlanetModel->SurfaceReliefScale = FMath::Clamp(GeneratedWorld->SurfaceReliefScale, 0.25, 2.5);
	PlanetModel->SurfaceLandCoverageScale = FMath::Clamp(GeneratedWorld->SurfaceLandCoverageScale, 0.25, 2.0);
	PlanetModel->SurfaceMountainScale = FMath::Clamp(GeneratedWorld->SurfaceMountainScale, 0.0, 2.0);
	PlanetModel->SurfaceCraterScale = FMath::Clamp(GeneratedWorld->SurfaceCraterScale, 0.0, 2.0);
	PlanetModel->SurfaceRoughnessScale = FMath::Clamp(GeneratedWorld->SurfaceRoughnessScale, 0.25, 2.0);

	return PlanetModel;
}

void UPlanetGenerator::GeneratePlanetAtmosphere(APlanet* Planet, const TSharedPtr<FPlanetAtmosphereModel>& PlanetAtmosphereMode)
{
	if (!Planet || !PlanetAtmosphereMode)
	{
		return;
	}

	UWorld* World = Planet->GetWorld();
	if (!World)
	{
		return;
	}

	if (AAtmoScape* PlanetAtmosphere = World->SpawnActor<AAtmoScape>(AAtmoScape::StaticClass(), FTransform()))
	{
		PlanetAtmosphere->SetActorLocation(Planet->GetActorLocation());
		PlanetAtmosphere->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);

		// Настройка параметров атмосферы
		PlanetAtmosphere->bKeepRelativeScale = false;
		PlanetAtmosphere->PlanetRadius = FMath::Max(PlanetAtmosphereMode->AtmosphereRadiusKm - 1.0, 0.5);
		PlanetAtmosphere->AtmosphereHeight = PlanetAtmosphereMode->AtmosphereHeight;
		PlanetAtmosphere->AtmosphereOpacity = PlanetAtmosphereMode->AtmosphereOpacity;
		PlanetAtmosphere->MultiScatering = PlanetAtmosphereMode->AtmosphereMultiScattering;
		PlanetAtmosphere->RayleighHeight = PlanetAtmosphereMode->AtmosphereRayleighScattering;
		//PlanetAtmosphere->Ray = PlanetAtmosphereMode->AtmosphereColor;
		PlanetAtmosphere->RayleighScattering = PlanetAtmosphereMode->AtmosphereColor;
		PlanetAtmosphere->UpdateScale();
	}
}

TSharedPtr<FPlanetAtmosphereModel> UPlanetGenerator::CreateAtmosphereModelFromGeneratedWorld(
	UGeneratedWorld* GeneratedWorld)
{
	if (!GeneratedWorld)
	{
		return nullptr;
	}

	TSharedPtr<FPlanetAtmosphereModel> AtmosphereModel = MakeShared<FPlanetAtmosphereModel>();
	AtmosphereModel->AtmosphereRadiusKm = GeneratedWorld->PlanetRadius; 
	AtmosphereModel->AtmosphereHeight = GeneratedWorld->AtmosphereHeight;
	AtmosphereModel->AtmosphereOpacity = GeneratedWorld->AtmosphereOpacity;
	AtmosphereModel->AtmosphereMultiScattering = GeneratedWorld->AtmosphereMultiScattering;
	AtmosphereModel->AtmosphereRayleighScattering = GeneratedWorld->AtmosphereRayleighScattering;
	AtmosphereModel->AtmosphereColor = GeneratedWorld->AtmosphereColor;

	return AtmosphereModel;}

void UPlanetGenerator::ConnectPlanetWithStar(APlanet* NewPlanet, AStar* NewStar)
{
	/*NewStar->AddPlanet(NewPlanet);
	NewPlanet->AttachToActor(NewStar, FAttachmentTransformRules::KeepRelativeTransform);
	NewPlanet->SetParentStar(NewStar);*/
}

FPlanetModel UPlanetGenerator::GenerateRandomPlanetModel()
{
	return FPlanetModel();
}

void UPlanetGenerator::ApplyModel(APlanet* PlanetActor, TSharedPtr<FPlanetModel> PlanetGenerationModel)
{
	if (!PlanetActor || !PlanetGenerationModel.IsValid())
	{
		return;
	}
	// UI-authored/single-body models legitimately start from FPlanetModel defaults.
	// Radius/type overrides used to leave density, mass and surface gravity at zero,
	// even though the generated WorldScape surface was physical. Complete only the
	// missing physical fields here, preserving values produced by the procedural path.
	EnsurePlanetPhysicalProperties(*PlanetGenerationModel);
	// Keep the actor and hierarchy data pointed at the exact model that was
	// materialized.  FillPlanetData serializes through this shared model; leaving
	// the constructor's empty placeholder here made the committed home world lose
	// its type, radius and moons even though the visible actor had those values.
	PlanetActor->PlanetData.PlanetModel = PlanetGenerationModel;
	PlanetActor->PlanetData.PlanetModelData = *PlanetGenerationModel;
	PlanetActor->SetPlanetType(PlanetGenerationModel->PlanetType);
	PlanetActor->SetPlanetZone(PlanetGenerationModel->PlanetZone);
	PlanetActor->SetPlanetDensity(PlanetGenerationModel->PlanetDensity);
	PlanetActor->SetPlanetGravityStrength(PlanetGenerationModel->PlanetGravityStrength);
	PlanetActor->SetTemperature(PlanetGenerationModel->Temperature);
	PlanetActor->SetAmountOfMoons(PlanetGenerationModel->AmountOfMoons);
	PlanetActor->SetRadius(PlanetGenerationModel->Radius);
	PlanetActor->SetMass(PlanetGenerationModel->Mass);
	PlanetActor->SetOrbitDistance(PlanetGenerationModel->OrbitDistance);
	PlanetActor->SetMoonsList(PlanetGenerationModel->MoonsList);
	PlanetActor->LagrangePoints = PlanetGenerationModel->LagrangePoints;
	PlanetActor->Orbits = PlanetGenerationModel->Orbits;
	PlanetActor->RadiusKM = PlanetGenerationModel->RadiusKM;
	PlanetActor->AffectionRadiusKM = PlanetGenerationModel->RadiusKM;
	PlanetActor->AtmosphereHeight = PlanetGenerationModel->AtmosphereHeight;
	PlanetActor->WorldScapeSeed = PlanetGenerationModel->SurfaceSeed;
	PlanetActor->SurfaceFeatureScale = PlanetGenerationModel->SurfaceFeatureScale;
	PlanetActor->SurfaceReliefScale = PlanetGenerationModel->SurfaceReliefScale;
	PlanetActor->SurfaceLandCoverageScale = PlanetGenerationModel->SurfaceLandCoverageScale;
	PlanetActor->SurfaceMountainScale = PlanetGenerationModel->SurfaceMountainScale;
	PlanetActor->SurfaceCraterScale = PlanetGenerationModel->SurfaceCraterScale;
	PlanetActor->SurfaceRoughnessScale = PlanetGenerationModel->SurfaceRoughnessScale;
}
