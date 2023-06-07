// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "StarClusterPopulation.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_StarClusterPopulation_generated_h
#error "StarClusterPopulation.generated.h already included, missing '#pragma once' in StarClusterPopulation.h"
#endif
#define APS_ALPHA_StarClusterPopulation_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterPopulation_h


#define FOREACH_ENUM_ESTARCLUSTERPOPULATION(op) \
	op(EStarClusterPopulation::AllSequenses) \
	op(EStarClusterPopulation::MainSequence) \
	op(EStarClusterPopulation::Giants) \
	op(EStarClusterPopulation::Dwarfs) \
	op(EStarClusterPopulation::Protostars) \
	op(EStarClusterPopulation::Unknown) 

enum class EStarClusterPopulation : uint8;
template<> struct TIsUEnumClass<EStarClusterPopulation> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterPopulation>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
