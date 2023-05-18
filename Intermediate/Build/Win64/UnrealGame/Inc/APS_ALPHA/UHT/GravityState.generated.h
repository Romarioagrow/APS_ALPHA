// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GravityState.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_GravityState_generated_h
#error "GravityState.generated.h already included, missing '#pragma once' in GravityState.h"
#endif
#define APS_ALPHA_GravityState_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityState_h


#define FOREACH_ENUM_EGRAVITYSTATE(op) \
	op(EGravityState::LowG) \
	op(EGravityState::Attracting) \
	op(EGravityState::Jumping) \
	op(EGravityState::Falling) \
	op(EGravityState::Attracted) 

enum class EGravityState : uint8;
template<> struct TIsUEnumClass<EGravityState> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EGravityState>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
