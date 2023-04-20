// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "SpaceCharacter.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_SpaceCharacter_generated_h
#error "SpaceCharacter.generated.h already included, missing '#pragma once' in SpaceCharacter.h"
#endif
#define APS_ALPHA_SpaceCharacter_generated_h

#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_SPARSE_DATA
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execRotateRoll); \
	DECLARE_FUNCTION(execRotateYaw); \
	DECLARE_FUNCTION(execRotatePitch);


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execRotateRoll); \
	DECLARE_FUNCTION(execRotateYaw); \
	DECLARE_FUNCTION(execRotatePitch);


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_ACCESSORS
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesASpaceCharacter(); \
	friend struct Z_Construct_UClass_ASpaceCharacter_Statics; \
public: \
	DECLARE_CLASS(ASpaceCharacter, APawn, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/APS_ALPHA"), NO_API) \
	DECLARE_SERIALIZER(ASpaceCharacter)


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_INCLASS \
private: \
	static void StaticRegisterNativesASpaceCharacter(); \
	friend struct Z_Construct_UClass_ASpaceCharacter_Statics; \
public: \
	DECLARE_CLASS(ASpaceCharacter, APawn, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/APS_ALPHA"), NO_API) \
	DECLARE_SERIALIZER(ASpaceCharacter)


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API ASpaceCharacter(const FObjectInitializer& ObjectInitializer); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(ASpaceCharacter) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ASpaceCharacter); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ASpaceCharacter); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API ASpaceCharacter(ASpaceCharacter&&); \
	NO_API ASpaceCharacter(const ASpaceCharacter&); \
public: \
	NO_API virtual ~ASpaceCharacter();


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API ASpaceCharacter(ASpaceCharacter&&); \
	NO_API ASpaceCharacter(const ASpaceCharacter&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ASpaceCharacter); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ASpaceCharacter); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ASpaceCharacter) \
	NO_API virtual ~ASpaceCharacter();


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_9_PROLOG
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_SPARSE_DATA \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_RPC_WRAPPERS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_ACCESSORS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_INCLASS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_SPARSE_DATA \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_ACCESSORS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_INCLASS_NO_PURE_DECLS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h_12_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> APS_ALPHA_API UClass* StaticClass<class ASpaceCharacter>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceCharacter_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
