// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Star.h"
#include "StarGenerationModel.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "StarGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UStarGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UStarGenerator();

public:
	// ������� ��������� ������
	//AStar* GenerateStar(FStarGenerationModel StarModel); // FStarParams Params

	void ApplyModel(AAstroActor* AstroActor, FGenerationModel GenerationModel);//override;

	FStarGenerationModel GenerateRandomStarModel();

};
