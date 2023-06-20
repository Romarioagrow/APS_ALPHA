// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "OrbitDistributionType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_OrbitDistributionType_generated_h
#error "OrbitDistributionType.generated.h already included, missing '#pragma once' in OrbitDistributionType.h"
#endif
#define APS_ALPHA_OrbitDistributionType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitDistributionType_h


#define FOREACH_ENUM_EORBITDISTRIBUTIONTYPE(op) \
	op(EOrbitDistributionType::Uniform) \
	op(EOrbitDistributionType::Gaussian) \
	op(EOrbitDistributionType::Chaotic) \
	op(EOrbitDistributionType::InnerOuter) \
	op(EOrbitDistributionType::Dense) \
	op(EOrbitDistributionType::Count) 

enum class EOrbitDistributionType : uint8;
template<> struct TIsUEnumClass<EOrbitDistributionType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EOrbitDistributionType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
