// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Galaxy.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxy() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxy();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxy_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGalaxy::StaticRegisterNativesAGalaxy()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGalaxy);
	UClass* Z_Construct_UClass_AGalaxy_NoRegister()
	{
		return AGalaxy::StaticClass();
	}
	struct Z_Construct_UClass_AGalaxy_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGalaxy_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialSystem,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGalaxy_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Galaxy.h" },
		{ "ModuleRelativePath", "Galaxy.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGalaxy_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGalaxy>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGalaxy_Statics::ClassParams = {
		&AGalaxy::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AGalaxy_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxy_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGalaxy()
	{
		if (!Z_Registration_Info_UClass_AGalaxy.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGalaxy.OuterSingleton, Z_Construct_UClass_AGalaxy_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGalaxy.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGalaxy>()
	{
		return AGalaxy::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGalaxy);
	AGalaxy::~AGalaxy() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGalaxy, AGalaxy::StaticClass, TEXT("AGalaxy"), &Z_Registration_Info_UClass_AGalaxy, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGalaxy), 1022289381U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_2030595899(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
