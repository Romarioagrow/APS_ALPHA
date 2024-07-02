// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/*#include "Moon.h"
#include "MoonGenerationModel.h"
#include "PlanetGenerationModel.h"*/

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Structs/MoonGenerationModel.h"
#include "MoonGenerator.generated.h"

class APlanet;
struct FPlanetModel;
class AMoon;
/**
 * 
 */
UCLASS()
class APS_ALPHA_API UMoonGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UMoonGenerator();

public:
    void ConnectMoonWithPlanet(AMoon* NewMoon, APlanet* NewPlanet);

    double CalculateGravitationalForce(double MassPlanet, double MassMoon, double Distance);

    double CalculateRandomMoonDensity(EMoonType MoonType);

	void ApplyModel(AMoon* Moon, TSharedPtr<FMoonModel> MoonGenerationModel);

	FMoonModel GenerateMoonModel();

	FMoonModel GenerateRandomMoonModel();

	EMoonType GenerateMoonType(TSharedPtr<FPlanetModel> PlanetModel);

    double CalculateRandomMoonMass();

	double CalculateMoonRadius(double MoonDensity, double MoonMass);

public:
    const TMap<EMoonType, TPair<double, double>> MoonDensityRanges = 
    {
        {EMoonType::Rocky, {2.5, 3.0}}, // ��/��^3
        {EMoonType::Icy, {0.9, 1.0}},
        {EMoonType::Iron, {5.8, 7.0}},
        {EMoonType::Volcanic, {2.5, 3.0}},
        {EMoonType::Gas, {0.1, 1.0}},
        {EMoonType::Ocean, {1.0, 1.5}},
        {EMoonType::Continental, {2.0, 2.5}},
        {EMoonType::Desert, {2.0, 2.5}},
        {EMoonType::TidallyLocked, {1.0, 3.0}}, // ��� ����� �������� �� ������� ����
        {EMoonType::Peculiar, {0.1, 5.0}}, // ����� ���� �����, � ����������� �� ������������
        {EMoonType::CapturedAsteroid, {1.0, 2.0}},
        {EMoonType::Unknown, {0.1, 5.0}} // ����� ���� �����
    };
};

