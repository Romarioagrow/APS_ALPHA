// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "StarClusterSize.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_StarClusterSize_generated_h
#error "StarClusterSize.generated.h already included, missing '#pragma once' in StarClusterSize.h"
#endif
#define APS_ALPHA_StarClusterSize_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterSize_h


#define FOREACH_ENUM_ESTARCLUSTERSIZE(op) \
	op(EStarClusterSize::Tiny) \
	op(EStarClusterSize::Small) \
	op(EStarClusterSize::Medium) \
	op(EStarClusterSize::Large) \
	op(EStarClusterSize::Giant) \
	op(EStarClusterSize::Unknown) 

enum class EStarClusterSize : uint8;
template<> struct TIsUEnumClass<EStarClusterSize> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterSize>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
