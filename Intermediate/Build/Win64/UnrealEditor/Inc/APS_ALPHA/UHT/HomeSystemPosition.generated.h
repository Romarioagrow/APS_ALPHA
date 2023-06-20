// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "HomeSystemPosition.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_HomeSystemPosition_generated_h
#error "HomeSystemPosition.generated.h already included, missing '#pragma once' in HomeSystemPosition.h"
#endif
#define APS_ALPHA_HomeSystemPosition_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_HomeSystemPosition_h


#define FOREACH_ENUM_EHOMESYSTEMPOSITION(op) \
	op(EHomeSystemPosition::WorldCenter) \
	op(EHomeSystemPosition::RandomPosition) \
	op(EHomeSystemPosition::DirectPosition) 

enum class EHomeSystemPosition : uint8;
template<> struct TIsUEnumClass<EHomeSystemPosition> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EHomeSystemPosition>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
