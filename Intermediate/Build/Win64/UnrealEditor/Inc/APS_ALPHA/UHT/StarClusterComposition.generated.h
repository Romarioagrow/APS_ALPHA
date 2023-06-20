// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "StarClusterComposition.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_StarClusterComposition_generated_h
#error "StarClusterComposition.generated.h already included, missing '#pragma once' in StarClusterComposition.h"
#endif
#define APS_ALPHA_StarClusterComposition_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterComposition_h


#define FOREACH_ENUM_ESTARCLUSTERCOMPOSITION(op) \
	op(EStarClusterComposition::AllSpectral) \
	op(EStarClusterComposition::OnlyBlue) \
	op(EStarClusterComposition::MostlyBlue) \
	op(EStarClusterComposition::BlueWhite) \
	op(EStarClusterComposition::OnlyWhite) \
	op(EStarClusterComposition::MostlyWhite) \
	op(EStarClusterComposition::WhiteYellow) \
	op(EStarClusterComposition::OnlyYellow) \
	op(EStarClusterComposition::MostlyYellow) \
	op(EStarClusterComposition::YellowOrange) \
	op(EStarClusterComposition::OnlyOrange) \
	op(EStarClusterComposition::MostlyOrange) \
	op(EStarClusterComposition::OrangeRed) \
	op(EStarClusterComposition::OnlyRed) \
	op(EStarClusterComposition::MostlyRed) \
	op(EStarClusterComposition::Unknown) 

enum class EStarClusterComposition : uint8;
template<> struct TIsUEnumClass<EStarClusterComposition> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterComposition>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
