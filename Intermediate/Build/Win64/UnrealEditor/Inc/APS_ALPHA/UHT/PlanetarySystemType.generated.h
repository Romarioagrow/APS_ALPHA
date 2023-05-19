// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "PlanetarySystemType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_PlanetarySystemType_generated_h
#error "PlanetarySystemType.generated.h already included, missing '#pragma once' in PlanetarySystemType.h"
#endif
#define APS_ALPHA_PlanetarySystemType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemType_h


#define FOREACH_ENUM_EPLANETARYSYSTEMTYPE(op) \
	op(EPlanetarySystemType::NoPlanetSystem) \
	op(EPlanetarySystemType::SmallSystem) \
	op(EPlanetarySystemType::LargeSystem) \
	op(EPlanetarySystemType::ChaoticSystem) \
	op(EPlanetarySystemType::DenseSystem) 

enum class EPlanetarySystemType : uint8;
template<> struct TIsUEnumClass<EPlanetarySystemType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EPlanetarySystemType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
