// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Moon.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "MoonGenerator.generated.h"

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
	// ������� ��������� ����
	AMoon* GenerateMoon(); // FMoonParams Params
};
