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


#define FOREACH_ENUM_ESTELLARTYPE(op) \
	op(EStellarType::HyperGiant) \
	op(EStellarType::SuperGiant) \
	op(EStellarType::BrightGiant) \
	op(EStellarType::Giant) \
	op(EStellarType::SubGiant) \
	op(EStellarType::MainSequence) \
	op(EStellarType::SubDwarf) \
	op(EStellarType::WhiteDwarf) \
	op(EStellarType::BrownDwarf) \
	op(EStellarType::Neutron) \
	op(EStellarType::Protostar) \
	op(EStellarType::Pulsar) \
	op(EStellarType::BlackHole) \
	op(EStellarType::Unknown) 

enum class EStellarType : uint8;
template<> struct TIsUEnumClass<EStellarType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStellarType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
