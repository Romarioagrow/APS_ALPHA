// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetGenerator.h"

UPlanetGenerator::UPlanetGenerator()
{
}

APlanet* UPlanetGenerator::GeneratePlanet(FPlanetGenerationModel PlanetModel)
{
    return nullptr;
}

FPlanetGenerationModel UPlanetGenerator::GenerateRandomPlanetModel()
{



    
    // 
    //     // Параметры планеты
    //     FPlanetParams Params;
    //
    //     // Параметры планеты
    //     Params.Name = "Planet";
    //     Params.Radius = 1000;
    //     Params.Mass = 1000;
    //     Params.Temperature = 1000;
    //     Params.AngularVelocity = 1000;
    //     Params.AxialTilt = 1000;
    //     Params.OrbitalPeriod = 1000;
    //     Params.OrbitalVelocity = 1000;
    //



    return FPlanetGenerationModel();
}

void UPlanetGenerator::ApplyModel(APlanet* PlanetActor, FPlanetGenerationModel PlanetGenerationModel)
{
	//this.PlanetType

	PlanetActor->SetPlanetType(PlanetGenerationModel.PlanetType);
	PlanetActor->SetPlanetZone(PlanetGenerationModel.PlanetZone);
	PlanetActor->SetPlanetDensity(PlanetGenerationModel.PlanetDensity);
	PlanetActor->SetPlanetGravityStrength(PlanetGenerationModel.PlanetGravityStrength);
	PlanetActor->SetTemperature(PlanetGenerationModel.Temperature);
	PlanetActor->SetAmountOfMoons(PlanetGenerationModel.AmountOfMoons);
	PlanetActor->SetRadius(PlanetGenerationModel.Radius);
	PlanetActor->SetMass(PlanetGenerationModel.Mass);

}

