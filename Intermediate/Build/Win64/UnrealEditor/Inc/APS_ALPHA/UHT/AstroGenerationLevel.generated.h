// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "AstroGenerationLevel.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_AstroGenerationLevel_generated_h
#error "AstroGenerationLevel.generated.h already included, missing '#pragma once' in AstroGenerationLevel.h"
#endif
#define APS_ALPHA_AstroGenerationLevel_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h


#define FOREACH_ENUM_EASTROGENERATIONLEVEL(op) \
	op(EAstroGenerationLevel::Galaxy) \
	op(EAstroGenerationLevel::Cluster) \
	op(EAstroGenerationLevel::StarSystem) \
	op(EAstroGenerationLevel::PlanetSystem) \
	op(EAstroGenerationLevel::SinglePlanet) \
	op(EAstroGenerationLevel::Random) 

enum class EAstroGenerationLevel : uint8;
template<> struct TIsUEnumClass<EAstroGenerationLevel> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EAstroGenerationLevel>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
