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


#define FOREACH_ENUM_ESTELLARCLASS(op) \
	op(EStellarClass::HyperGiant) \
	op(EStellarClass::SuperGiant) \
	op(EStellarClass::BrightGiant) \
	op(EStellarClass::Giant) \
	op(EStellarClass::SubGiant) \
	op(EStellarClass::MainSequence) \
	op(EStellarClass::SubDwarf) \
	op(EStellarClass::WhiteDwarf) \
	op(EStellarClass::BrownDwarf) \
	op(EStellarClass::Neutron) \
	op(EStellarClass::Protostar) \
	op(EStellarClass::Pulsar) \
	op(EStellarClass::BlackHole) 

enum class EStellarClass : uint8;
template<> struct TIsUEnumClass<EStellarClass> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStellarClass>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
