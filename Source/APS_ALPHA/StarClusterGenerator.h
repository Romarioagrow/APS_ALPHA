// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "StarGenerationModel.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "StarClusterGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UStarClusterGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UStarClusterGenerator();
	
public:
	void GenerateRandomStarCluster();

	UPROPERTY(EditAnywhere, Category = "Generation Params")
		TSubclassOf<class AStarCluster> BP_StarClusterClass;
};
