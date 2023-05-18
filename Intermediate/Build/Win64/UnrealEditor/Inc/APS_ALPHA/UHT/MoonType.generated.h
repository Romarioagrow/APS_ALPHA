// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "MoonType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_MoonType_generated_h
#error "MoonType.generated.h already included, missing '#pragma once' in MoonType.h"
#endif
#define APS_ALPHA_MoonType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonType_h


#define FOREACH_ENUM_EMOONTYPE(op) \
	op(EMoonType::Rocky) \
	op(EMoonType::Icy) \
	op(EMoonType::Iron) \
	op(EMoonType::Volcanic) \
	op(EMoonType::Gas) \
	op(EMoonType::Ocean) \
	op(EMoonType::Continental) \
	op(EMoonType::Desert) \
	op(EMoonType::TidallyLocked) \
	op(EMoonType::Peculiar) \
	op(EMoonType::CapturedAsteroid) 

enum class EMoonType : uint8;
template<> struct TIsUEnumClass<EMoonType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EMoonType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
