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

void UPlanetGenerator::ApplyModel(AAstroActor* AstroActor, FGenerationModel GenerationModel)
{
}
