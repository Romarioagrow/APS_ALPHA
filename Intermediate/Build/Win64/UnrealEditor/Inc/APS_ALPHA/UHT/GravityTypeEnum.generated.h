// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GravityTypeEnum.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_GravityTypeEnum_generated_h
#error "GravityTypeEnum.generated.h already included, missing '#pragma once' in GravityTypeEnum.h"
#endif
#define APS_ALPHA_GravityTypeEnum_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityTypeEnum_h


#define FOREACH_ENUM_EGRAVITYTYPE(op) \
	op(EGravityType::ZeroG) \
	op(EGravityType::OnStation) \
	op(EGravityType::OnPlanet) \
	op(EGravityType::OnShip) 

enum class EGravityType : uint8;
template<> struct TIsUEnumClass<EGravityType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EGravityType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
