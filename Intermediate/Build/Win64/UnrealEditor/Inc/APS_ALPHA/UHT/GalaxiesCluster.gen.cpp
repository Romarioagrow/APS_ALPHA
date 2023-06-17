// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GalaxiesCluster.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxiesCluster() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxiesCluster();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxiesCluster_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGalaxiesCluster::StaticRegisterNativesAGalaxiesCluster()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGalaxiesCluster);
	UClass* Z_Construct_UClass_AGalaxiesCluster_NoRegister()
	{
		return AGalaxiesCluster::StaticClass();
	}
	struct Z_Construct_UClass_AGalaxiesCluster_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGalaxiesCluster_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialSystem,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGalaxiesCluster_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "GalaxiesCluster.h" },
		{ "ModuleRelativePath", "GalaxiesCluster.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGalaxiesCluster_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGalaxiesCluster>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGalaxiesCluster_Statics::ClassParams = {
		&AGalaxiesCluster::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AGalaxiesCluster_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxiesCluster_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGalaxiesCluster()
	{
		if (!Z_Registration_Info_UClass_AGalaxiesCluster.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGalaxiesCluster.OuterSingleton, Z_Construct_UClass_AGalaxiesCluster_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGalaxiesCluster.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGalaxiesCluster>()
	{
		return AGalaxiesCluster::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGalaxiesCluster);
	AGalaxiesCluster::~AGalaxiesCluster() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxiesCluster_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxiesCluster_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGalaxiesCluster, AGalaxiesCluster::StaticClass, TEXT("AGalaxiesCluster"), &Z_Registration_Info_UClass_AGalaxiesCluster, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGalaxiesCluster), 2092548087U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxiesCluster_h_1788606939(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxiesCluster_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxiesCluster_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
