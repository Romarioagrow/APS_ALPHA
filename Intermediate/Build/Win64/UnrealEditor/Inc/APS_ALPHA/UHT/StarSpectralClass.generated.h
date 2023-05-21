// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "StarSpectralClass.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_StarSpectralClass_generated_h
#error "StarSpectralClass.generated.h already included, missing '#pragma once' in StarSpectralClass.h"
#endif
#define APS_ALPHA_StarSpectralClass_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h


#define FOREACH_ENUM_ESPECTRALCLASS(op) \
	op(ESpectralClass::O) \
	op(ESpectralClass::B) \
	op(ESpectralClass::A) \
	op(ESpectralClass::F) \
	op(ESpectralClass::G) \
	op(ESpectralClass::K) \
	op(ESpectralClass::M) \
	op(ESpectralClass::L) \
	op(ESpectralClass::T) \
	op(ESpectralClass::Y) \
	op(ESpectralClass::NS) \
	op(ESpectralClass::PS) \
	op(ESpectralClass::BH) \
	op(ESpectralClass::Unknown) 

enum class ESpectralClass : uint8;
template<> struct TIsUEnumClass<ESpectralClass> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<ESpectralClass>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
