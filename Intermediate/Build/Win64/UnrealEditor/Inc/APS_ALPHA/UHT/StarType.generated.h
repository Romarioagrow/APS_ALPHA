// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "StarType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_StarType_generated_h
#error "StarType.generated.h already included, missing '#pragma once' in StarType.h"
#endif
#define APS_ALPHA_StarType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarType_h


#define FOREACH_ENUM_ESTARTYPE(op) \
	op(EStarType::MainSequence) \
	op(EStarType::Giant) \
	op(EStarType::SuperGiant) \
	op(EStarType::WhiteDwarf) \
	op(EStarType::Neutron) \
	op(EStarType::BrownDwarf) \
	op(EStarType::RedDwarf) \
	op(EStarType::SubDwarf) \
	op(EStarType::HyperGiant) \
	op(EStarType::Protostar) \
	op(EStarType::Pulsar) \
	op(EStarType::BlackHole) 

enum class EStarType : uint8;
template<> struct TIsUEnumClass<EStarType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStarType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
