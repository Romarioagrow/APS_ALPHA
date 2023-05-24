// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "PlanetaryZoneType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_PlanetaryZoneType_generated_h
#error "PlanetaryZoneType.generated.h already included, missing '#pragma once' in PlanetaryZoneType.h"
#endif
#define APS_ALPHA_PlanetaryZoneType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryZoneType_h


#define FOREACH_ENUM_EPLANETARYZONETYPE(op) \
	op(EPlanetaryZoneType::DeadZone) \
	op(EPlanetaryZoneType::HabitableZone) \
	op(EPlanetaryZoneType::ColdZone) \
	op(EPlanetaryZoneType::IceZone) \
	op(EPlanetaryZoneType::WarmZone) \
	op(EPlanetaryZoneType::HotZone) \
	op(EPlanetaryZoneType::InnerPlanetZone) \
	op(EPlanetaryZoneType::OuterPlanetZone) \
	op(EPlanetaryZoneType::AsteroidBeltZone) \
	op(EPlanetaryZoneType::KuiperBeltZone) \
	op(EPlanetaryZoneType::GasGiantsZone) \
	op(EPlanetaryZoneType::Unknown) 

enum class EPlanetaryZoneType : uint8;
template<> struct TIsUEnumClass<EPlanetaryZoneType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EPlanetaryZoneType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
