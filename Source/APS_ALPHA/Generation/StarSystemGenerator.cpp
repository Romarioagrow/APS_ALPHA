#include "StarSystemGenerator.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"

namespace APSStarSystemGeneration
{
	void GenerateMultiplicity(FStarSystemModel& Model, FRandomStream& Stream)
	{
		const int32 RandomValue = Stream.RandRange(0, 99);
		if (RandomValue < 80)
		{
			Model.StarSystemType = EStarType::SingleStar;
			Model.AmountOfStars = 1;
		}
		else if (RandomValue < 90)
		{
			Model.StarSystemType = EStarType::DoubleStar;
			Model.AmountOfStars = 2;
		}
		else if (RandomValue < 98)
		{
			Model.StarSystemType = EStarType::TripleStar;
			Model.AmountOfStars = 3;
		}
		else
		{
			Model.StarSystemType = EStarType::MultipleStar;
			Model.AmountOfStars = Stream.RandRange(4, 6);
		}
	}

	int32 GetMaximumPotentialPlanets(ESpectralClass SpectralClass)
	{
		switch (SpectralClass)
		{
		case ESpectralClass::O:
		case ESpectralClass::B: return 4;
		case ESpectralClass::A:
		case ESpectralClass::F: return 8;
		case ESpectralClass::G:
		case ESpectralClass::K: return 12;
		case ESpectralClass::M: return 10;
		case ESpectralClass::L:
		case ESpectralClass::T:
		case ESpectralClass::Y: return 4;
		default: return 0;
		}
	}
}

UStarSystemGenerator::UStarSystemGenerator()
{
}

void UStarSystemGenerator::ApplyModel(AStarSystem* StarSystem, TSharedPtr<FStarSystemModel> StarSystemGenerationModel)
{
	if (!StarSystem || !StarSystemGenerationModel)
	{
		return;
	}
	StarSystem->SetStarsAmount(StarSystemGenerationModel->AmountOfStars);
	StarSystem->SetStarSystemType(StarSystemGenerationModel->StarSystemType);
	StarSystem->StableSystemId = StarSystemGenerationModel->StableId;
	StarSystem->GenerationSeed = StarSystemGenerationModel->GenerationSeed;
	StarSystem->bMaterializedFromCluster = StarSystemGenerationModel->StableId.IsValid();
}

void UStarSystemGenerator::GenerateCustomHomeSystemModel(TSharedPtr<FStarSystemModel> StarSystemModel)
{
	//StarSystemModel
}

void UStarSystemGenerator::GenerateRandomStarSystemModel(TSharedPtr<FStarSystemModel> StarSystemModel)
{
	if (!StarSystemModel)
	{
		return;
	}
	// ��������� ����� � ��������� �� 0 �� 100
	int32 RandomValue = FMath::RandRange(0, 99);

	// ���� ��� ������� ���� �������
	int32 SingleStarWeight = 80; // 80%
	//int32 SingleStarWeight = 80; // 80%
	int32 DoubleStarWeight = 10; // 10%
	int32 TripleStarWeight = 8; // 8%
	int32 MultipleStarWeight = 2; // 2%

	if (RandomValue < SingleStarWeight)
		{
			StarSystemModel->StarSystemType = EStarType::SingleStar;
			StarSystemModel->AmountOfStars = 1;
		}
		else if (RandomValue < SingleStarWeight + DoubleStarWeight)
		{
			StarSystemModel->StarSystemType = EStarType::DoubleStar;
			StarSystemModel->AmountOfStars = 2;
		}
		else if (RandomValue < SingleStarWeight + DoubleStarWeight + TripleStarWeight)
		{
			StarSystemModel->StarSystemType = EStarType::TripleStar;
			StarSystemModel->AmountOfStars = 3;
		}
		else
		{
			StarSystemModel->StarSystemType = EStarType::MultipleStar;
			StarSystemModel->AmountOfStars = FMath::RandRange(4, 6);
		}

	//return StarSystemModel;
}

void UStarSystemGenerator::GeneratePotentialStarSystemModel(FStarSystemModel& StarSystemModel,
	const FStarModel& PrimaryStarModel, int32 GenerationSeed) const
{
	StarSystemModel.GenerationSeed = GenerationSeed == 0 ? 1 : GenerationSeed;
	FRandomStream Stream(StarSystemModel.GenerationSeed);
	APSStarSystemGeneration::GenerateMultiplicity(StarSystemModel, Stream);

	const int32 MaximumPlanetCount = APSStarSystemGeneration::GetMaximumPotentialPlanets(
		PrimaryStarModel.SpectralClass);
	StarSystemModel.PotentialPlanetCount = MaximumPlanetCount > 0
		? Stream.RandRange(0, MaximumPlanetCount) : 0;
	StarSystemModel.bHasPlanetarySystem = StarSystemModel.PotentialPlanetCount > 0;
}
