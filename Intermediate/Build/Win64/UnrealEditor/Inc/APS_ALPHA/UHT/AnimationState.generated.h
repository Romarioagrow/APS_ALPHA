// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "AnimationState.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_AnimationState_generated_h
#error "AnimationState.generated.h already included, missing '#pragma once' in AnimationState.h"
#endif
#define APS_ALPHA_AnimationState_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AnimationState_h


#define FOREACH_ENUM_EANIMATIONSTATE(op) \
	op(EAnimationState::Idle) \
	op(EAnimationState::Walk) \
	op(EAnimationState::Run) \
	op(EAnimationState::Jumping) \
	op(EAnimationState::Falling) \
	op(EAnimationState::Floating) \
	op(EAnimationState::OnGround) 

enum class EAnimationState : uint8;
template<> struct TIsUEnumClass<EAnimationState> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EAnimationState>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
