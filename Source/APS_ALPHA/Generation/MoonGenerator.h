#pragma once

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Structs/MoonGenerationModel.h"
#include "MoonGenerator.generated.h"

class AMoon;
class APlanet;
struct FPlanetModel;

UCLASS()
class APS_ALPHA_API UMoonGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UMoonGenerator();

    void ConnectMoonWithPlanet(AMoon* NewMoon, APlanet* NewPlanet);

    double CalculateGravitationalForce(double MassPlanet, double MassMoon, double Distance);

    double CalculateRandomMoonDensity(EMoonType MoonType, FRandomStream* Random = nullptr);

	void ApplyModel(AMoon* Moon, TSharedPtr<FMoonModel> MoonGenerationModel);

	/** Resolve the visual surface independently from gameplay habitability. */
	static EPlanetType ResolveSurfaceType(const FMoonModel& Model);

	FMoonModel GenerateMoonModel();

	FMoonModel GenerateRandomMoonModel();

	EMoonType GenerateMoonType(TSharedPtr<FPlanetModel> PlanetModel, FRandomStream* Random = nullptr);

    double CalculateRandomMoonMass(FRandomStream* Random = nullptr);

	double CalculateMoonRadius(double MoonDensity, double MoonMass);

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

