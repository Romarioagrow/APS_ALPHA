// Fill out your copyright notice in the Description page of Project Settings.

#include "PlanetGenerator.h"
#include "MoonGenerator.h"
#include "Star.h"

UPlanetGenerator::UPlanetGenerator()
{
}

TArray<FMoonData> UPlanetGenerator::GenerateMoonsList(FPlanetModel PlanetModel)
{
    return TArray<FMoonData>();
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
}
