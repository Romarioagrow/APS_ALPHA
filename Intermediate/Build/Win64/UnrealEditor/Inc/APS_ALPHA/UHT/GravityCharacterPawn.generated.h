// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GravityCharacterPawn.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class AActor;
class UPrimitiveComponent;
struct FHitResult;
#ifdef APS_ALPHA_GravityCharacterPawn_generated_h
#error "GravityCharacterPawn.generated.h already included, missing '#pragma once' in GravityCharacterPawn.h"
#endif
#define APS_ALPHA_GravityCharacterPawn_generated_h

#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_SPARSE_DATA
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execOnEndOverlap); \
	DECLARE_FUNCTION(execOnBeginOverlap); \
	DECLARE_FUNCTION(execAlignCharacterToCamera); \
	DECLARE_FUNCTION(execRotateYaw); \
	DECLARE_FUNCTION(execRotateRoll); \
	DECLARE_FUNCTION(execRotatePitch); \
	DECLARE_FUNCTION(execMoveUp); \
	DECLARE_FUNCTION(execMoveRight); \
	DECLARE_FUNCTION(execMoveForward); \
	DECLARE_FUNCTION(execLookUp); \
	DECLARE_FUNCTION(execTurn);


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execOnEndOverlap); \
	DECLARE_FUNCTION(execOnBeginOverlap); \
	DECLARE_FUNCTION(execAlignCharacterToCamera); \
	DECLARE_FUNCTION(execRotateYaw); \
	DECLARE_FUNCTION(execRotateRoll); \
	DECLARE_FUNCTION(execRotatePitch); \
	DECLARE_FUNCTION(execMoveUp); \
	DECLARE_FUNCTION(execMoveRight); \
	DECLARE_FUNCTION(execMoveForward); \
	DECLARE_FUNCTION(execLookUp); \
	DECLARE_FUNCTION(execTurn);


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_ACCESSORS
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAGravityCharacterPawn(); \
	friend struct Z_Construct_UClass_AGravityCharacterPawn_Statics; \
public: \
	DECLARE_CLASS(AGravityCharacterPawn, AControlledPawn, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/APS_ALPHA"), NO_API) \
	DECLARE_SERIALIZER(AGravityCharacterPawn) \
	virtual UObject* _getUObject() const override { return const_cast<AGravityCharacterPawn*>(this); }


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_INCLASS \
private: \
	static void StaticRegisterNativesAGravityCharacterPawn(); \
	friend struct Z_Construct_UClass_AGravityCharacterPawn_Statics; \
public: \
	DECLARE_CLASS(AGravityCharacterPawn, AControlledPawn, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/APS_ALPHA"), NO_API) \
	DECLARE_SERIALIZER(AGravityCharacterPawn) \
	virtual UObject* _getUObject() const override { return const_cast<AGravityCharacterPawn*>(this); }


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API AGravityCharacterPawn(const FObjectInitializer& ObjectInitializer); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(AGravityCharacterPawn) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AGravityCharacterPawn); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AGravityCharacterPawn); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API AGravityCharacterPawn(AGravityCharacterPawn&&); \
	NO_API AGravityCharacterPawn(const AGravityCharacterPawn&); \
public: \
	NO_API virtual ~AGravityCharacterPawn();


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API AGravityCharacterPawn(AGravityCharacterPawn&&); \
	NO_API AGravityCharacterPawn(const AGravityCharacterPawn&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AGravityCharacterPawn); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AGravityCharacterPawn); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(AGravityCharacterPawn) \
	NO_API virtual ~AGravityCharacterPawn();


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_20_PROLOG
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_SPARSE_DATA \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_RPC_WRAPPERS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_ACCESSORS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_INCLASS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_SPARSE_DATA \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_ACCESSORS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_INCLASS_NO_PURE_DECLS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_23_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> APS_ALPHA_API UClass* StaticClass<class AGravityCharacterPawn>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
