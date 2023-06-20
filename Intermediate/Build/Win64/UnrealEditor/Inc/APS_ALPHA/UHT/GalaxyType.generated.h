// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GalaxyType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_GalaxyType_generated_h
#error "GalaxyType.generated.h already included, missing '#pragma once' in GalaxyType.h"
#endif
#define APS_ALPHA_GalaxyType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyType_h


#define FOREACH_ENUM_EGALAXYTYPE(op) \
	op(EGalaxyType::Elliptical) \
	op(EGalaxyType::Lenticular) \
	op(EGalaxyType::Spiral) \
	op(EGalaxyType::BarredSpiral) \
	op(EGalaxyType::Irregular) \
	op(EGalaxyType::Peculiar) \
	op(EGalaxyType::Unknown) 

enum class EGalaxyType : uint8;
template<> struct TIsUEnumClass<EGalaxyType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EGalaxyType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
