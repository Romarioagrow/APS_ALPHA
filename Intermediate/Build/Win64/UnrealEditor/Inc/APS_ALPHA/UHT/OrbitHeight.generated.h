// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "OrbitHeight.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_OrbitHeight_generated_h
#error "OrbitHeight.generated.h already included, missing '#pragma once' in OrbitHeight.h"
#endif
#define APS_ALPHA_OrbitHeight_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitHeight_h


#define FOREACH_ENUM_EORBITHEIGHT(op) \
	op(EOrbitHeight::UpperAtmosphere) \
	op(EOrbitHeight::LowOrbit) \
	op(EOrbitHeight::Geostationary) \
	op(EOrbitHeight::HighOrbit) \
	op(EOrbitHeight::VeryHighOrbit) 

enum class EOrbitHeight : uint8;
template<> struct TIsUEnumClass<EOrbitHeight> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EOrbitHeight>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
