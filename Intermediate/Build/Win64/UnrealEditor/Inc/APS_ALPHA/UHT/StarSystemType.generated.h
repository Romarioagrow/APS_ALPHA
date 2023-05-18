// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "StarSystemType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_StarSystemType_generated_h
#error "StarSystemType.generated.h already included, missing '#pragma once' in StarSystemType.h"
#endif
#define APS_ALPHA_StarSystemType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemType_h


#define FOREACH_ENUM_ESTARSYSTEMTYPE(op) \
	op(EStarSystemType::SingleStar) \
	op(EStarSystemType::DoubleStar) \
	op(EStarSystemType::TrippleStar) \
	op(EStarSystemType::MultipleStar) 

enum class EStarSystemType : uint8;
template<> struct TIsUEnumClass<EStarSystemType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStarSystemType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
