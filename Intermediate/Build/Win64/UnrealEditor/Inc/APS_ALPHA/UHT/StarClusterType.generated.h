// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "StarClusterType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_StarClusterType_generated_h
#error "StarClusterType.generated.h already included, missing '#pragma once' in StarClusterType.h"
#endif
#define APS_ALPHA_StarClusterType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterType_h


#define FOREACH_ENUM_ESTARCLUSTERTYPE(op) \
	op(EStarClusterType::OpenCluster) \
	op(EStarClusterType::GlobularCluster) \
	op(EStarClusterType::Supercluster) \
	op(EStarClusterType::Nebula) \
	op(EStarClusterType::Unknown) 

enum class EStarClusterType : uint8;
template<> struct TIsUEnumClass<EStarClusterType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
