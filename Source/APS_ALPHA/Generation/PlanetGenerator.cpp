#include "PlanetGenerator.h"

#include "PlanetaryAtmosphere.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Structs/PlanetAtmosphereModel.h"

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
		const double RadiusInCm = PlanetModel->Radius * KM_TO_CM * SCALE_FACTOR;
		NewPlanet->SetActorScale3D(FVector(RadiusInCm));
		NewPlanet->PlanetRadiusKM = PlanetModel->Radius; 

		
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
	PlanetModel->Radius = GeneratedWorld->PlanetRadius;

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
		PlanetAtmosphere->PlanetRadius = PlanetAtmosphereMode->AtmosphereRadiusKm;
		PlanetAtmosphere->AtmosphereHeight = PlanetAtmosphereMode->AtmosphereHeight;
		PlanetAtmosphere->AtmosphereOpacity = PlanetAtmosphereMode->AtmosphereOpacity;
		PlanetAtmosphere->MultiScatering = PlanetAtmosphereMode->AtmosphereMultiScattering;
		PlanetAtmosphere->RayleighHeight = PlanetAtmosphereMode->AtmosphereRayleighScattering;
		//PlanetAtmosphere->Ray = PlanetAtmosphereMode->AtmosphereColor;
		PlanetAtmosphere->RayleighScattering = PlanetAtmosphereMode->AtmosphereColor;
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
}
