// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
	void GenerateRandomStarCluster();
};
