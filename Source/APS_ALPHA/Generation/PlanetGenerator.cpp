// Fill out your copyright notice in the Description page of Project Settings.

#include "PlanetGenerator.h"
//#include "MoonGenerator.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
//#include "Star.h"

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

                //        // ≈сли вы хотите прикрепить его к планете
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
