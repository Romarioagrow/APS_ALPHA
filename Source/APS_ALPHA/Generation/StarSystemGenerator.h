#pragma once

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "StarSystemGenerator.generated.h"

class AStarSystem;
struct FStarSystemModel;
struct FStarModel;

UCLASS()
class APS_ALPHA_API UStarSystemGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UStarSystemGenerator();

	void ApplyModel(AStarSystem* StarSystem, TSharedPtr<FStarSystemModel> StarSystemGenerationModel);

	void GenerateCustomHomeSystemModel(TSharedPtr<FStarSystemModel> StarSystemModel);

	void GenerateRandomStarSystemModel(TSharedPtr<FStarSystemModel> StarSystemModel);

	/** Builds a deterministic, actor-free system summary for a generated cluster star. */
	void GeneratePotentialStarSystemModel(FStarSystemModel& StarSystemModel,
		const FStarModel& PrimaryStarModel, int32 GenerationSeed) const;
};
