// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "SpectralType.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_SpectralType_generated_h
#error "SpectralType.generated.h already included, missing '#pragma once' in SpectralType.h"
#endif
#define APS_ALPHA_SpectralType_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpectralType_h


#define FOREACH_ENUM_ESPECTRALTYPE(op) \
	op(ESpectralType::O) \
	op(ESpectralType::Ia) \
	op(ESpectralType::Iab) \
	op(ESpectralType::Ib) \
	op(ESpectralType::II) \
	op(ESpectralType::III) \
	op(ESpectralType::IV) \
	op(ESpectralType::V) \
	op(ESpectralType::VI) \
	op(ESpectralType::VII) \
	op(ESpectralType::VIII) 

enum class ESpectralType : uint8;
template<> struct TIsUEnumClass<ESpectralType> { enum { Value = true }; };
template<> APS_ALPHA_API UEnum* StaticEnum<ESpectralType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
