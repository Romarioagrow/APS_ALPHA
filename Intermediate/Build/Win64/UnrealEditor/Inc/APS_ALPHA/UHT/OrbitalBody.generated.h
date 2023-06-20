// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "OrbitalBody.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef APS_ALPHA_OrbitalBody_generated_h
#error "OrbitalBody.generated.h already included, missing '#pragma once' in OrbitalBody.h"
#endif
#define APS_ALPHA_OrbitalBody_generated_h

#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_SPARSE_DATA
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_RPC_WRAPPERS
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_ACCESSORS
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAOrbitalBody(); \
	friend struct Z_Construct_UClass_AOrbitalBody_Statics; \
public: \
	DECLARE_CLASS(AOrbitalBody, ACelestialBody, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/APS_ALPHA"), NO_API) \
	DECLARE_SERIALIZER(AOrbitalBody) \
	virtual UObject* _getUObject() const override { return const_cast<AOrbitalBody*>(this); }


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_INCLASS \
private: \
	static void StaticRegisterNativesAOrbitalBody(); \
	friend struct Z_Construct_UClass_AOrbitalBody_Statics; \
public: \
	DECLARE_CLASS(AOrbitalBody, ACelestialBody, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/APS_ALPHA"), NO_API) \
	DECLARE_SERIALIZER(AOrbitalBody) \
	virtual UObject* _getUObject() const override { return const_cast<AOrbitalBody*>(this); }


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API AOrbitalBody(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(AOrbitalBody) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AOrbitalBody); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AOrbitalBody); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API AOrbitalBody(AOrbitalBody&&); \
	NO_API AOrbitalBody(const AOrbitalBody&); \
public: \
	NO_API virtual ~AOrbitalBody();


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API AOrbitalBody() { }; \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API AOrbitalBody(AOrbitalBody&&); \
	NO_API AOrbitalBody(const AOrbitalBody&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AOrbitalBody); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AOrbitalBody); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(AOrbitalBody) \
	NO_API virtual ~AOrbitalBody();


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_13_PROLOG
#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_SPARSE_DATA \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_RPC_WRAPPERS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_ACCESSORS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_INCLASS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_SPARSE_DATA \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_ACCESSORS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_INCLASS_NO_PURE_DECLS \
	FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> APS_ALPHA_API UClass* StaticClass<class AOrbitalBody>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
