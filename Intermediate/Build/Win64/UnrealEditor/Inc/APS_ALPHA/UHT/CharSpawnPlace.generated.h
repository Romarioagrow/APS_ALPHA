// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "CharSpawnPlace.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_CharSpawnPlace_generated_h
#error "CharSpawnPlace.generated.h already included, missing '#pragma once' in CharSpawnPlace.h"
#endif
#define APS_ALPHA_CharSpawnPlace_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CharSpawnPlace_h


#define FOREACH_ENUM_ECHARSPAWNPLACE(op) \
	op(ECharSpawnPlace::PlanetOrbit) \
	op(ECharSpawnPlace::PlanetSurface) \
	op(ECharSpawnPlace::MoonOrbit) \
	op(ECharSpawnPlace::MoonSurface) \
	op(ECharSpawnPlace::SpaceShip) 

enum class ECharSpawnPlace : uint8;
template<> struct TIsUEnumClass<ECharSpawnPlace> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<ECharSpawnPlace>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
