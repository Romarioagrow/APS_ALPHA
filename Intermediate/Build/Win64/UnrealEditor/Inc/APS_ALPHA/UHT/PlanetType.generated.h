// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "PlanetType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_PlanetType_generated_h
#error "PlanetType.generated.h already included, missing '#pragma once' in PlanetType.h"
#endif
#define APS_ALPHA_PlanetType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetType_h


#define FOREACH_ENUM_EPLANETTYPE(op) \
	op(EPlanetType::Terrestrial) \
	op(EPlanetType::Rocky) \
	op(EPlanetType::GasGiant) \
	op(EPlanetType::IceGiant) \
	op(EPlanetType::Dwarf) \
	op(EPlanetType::Exoplanet) \
	op(EPlanetType::Rogue) \
	op(EPlanetType::Ocean) \
	op(EPlanetType::Desert) \
	op(EPlanetType::Forest) \
	op(EPlanetType::Volcanic) \
	op(EPlanetType::Ice) \
	op(EPlanetType::Ammonia) \
	op(EPlanetType::Iron) \
	op(EPlanetType::Carbon) \
	op(EPlanetType::SuperEarth) \
	op(EPlanetType::Unknown) 

enum class EPlanetType : uint8;
template<> struct TIsUEnumClass<EPlanetType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EPlanetType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
