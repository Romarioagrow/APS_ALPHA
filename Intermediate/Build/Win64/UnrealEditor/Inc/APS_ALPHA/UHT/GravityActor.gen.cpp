// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityActor() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityActor_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	ENGINE_API UClass* Z_Construct_UClass_USphereComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGravityActor::StaticRegisterNativesAGravityActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGravityActor);
	UClass* Z_Construct_UClass_AGravityActor_NoRegister()
	{
		return AGravityActor::StaticClass();
	}
	struct Z_Construct_UClass_AGravityActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SphereCollisionComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_SphereCollisionComponent;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGravityActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityActor_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "GravityActor.h" },
		{ "ModuleRelativePath", "GravityActor.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityActor_Statics::NewProp_SphereCollisionComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityActor.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityActor_Statics::NewProp_SphereCollisionComponent = { "SphereCollisionComponent", nullptr, (EPropertyFlags)0x00200800000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityActor, SphereCollisionComponent), Z_Construct_UClass_USphereComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityActor_Statics::NewProp_SphereCollisionComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityActor_Statics::NewProp_SphereCollisionComponent_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AGravityActor_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityActor_Statics::NewProp_SphereCollisionComponent,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGravityActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGravityActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGravityActor_Statics::ClassParams = {
		&AGravityActor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AGravityActor_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AGravityActor_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AGravityActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGravityActor()
	{
		if (!Z_Registration_Info_UClass_AGravityActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGravityActor.OuterSingleton, Z_Construct_UClass_AGravityActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGravityActor.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGravityActor>()
	{
		return AGravityActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGravityActor);
	AGravityActor::~AGravityActor() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGravityActor, AGravityActor::StaticClass, TEXT("AGravityActor"), &Z_Registration_Info_UClass_AGravityActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGravityActor), 3564739396U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActor_h_1650902733(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
