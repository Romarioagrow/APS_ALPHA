#include "StarSystemGenerator.h"
#include "StarSystem.h"

UStarSystemGenerator::UStarSystemGenerator()
{
}

void UStarSystemGenerator::ApplyModel(AStarSystem* StarSystem, FStarSystemGenerationModel StarSystemGenerationModel)
{
	StarSystem->SetStarsAmount(StarSystemGenerationModel.AmountOfStars);
	StarSystem->SetStarSystemType(StarSystemGenerationModel.StarSystemType);
}

FStarSystemGenerationModel UStarSystemGenerator::GenerateRandomStarSystemModel()
{
	FStarSystemGenerationModel StarSystemModel;

	// Случайное число в диапазоне от 0 до 100
	int32 RandomValue = FMath::RandRange(0, 100);

	// Веса для каждого типа системы
	int32 SingleStarWeight = 80; // 80%
	int32 DoubleStarWeight = 10; // 10%
	int32 TripleStarWeight = 8; // 8%
	int32 MultipleStarWeight = 2; // 2%

	if (RandomValue < SingleStarWeight)
	{
		StarSystemModel.StarSystemType = EStarSystemType::SingleStar;
		StarSystemModel.AmountOfStars = 1;
	}
	else if (RandomValue < SingleStarWeight + DoubleStarWeight)
	{
		StarSystemModel.StarSystemType = EStarSystemType::DoubleStar;
		StarSystemModel.AmountOfStars = 2;
	}
	else if (RandomValue < SingleStarWeight + DoubleStarWeight + TripleStarWeight)
	{
		StarSystemModel.StarSystemType = EStarSystemType::TripleStar;
		StarSystemModel.AmountOfStars = 3;
	}
	else
	{
		StarSystemModel.StarSystemType = EStarSystemType::MultipleStar;
		StarSystemModel.AmountOfStars = FMath::RandRange(4, 6);
	}

	return StarSystemModel;
}
