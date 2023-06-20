// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GalaxySector.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxySector() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxySector();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxySector_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGalaxySector::StaticRegisterNativesAGalaxySector()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGalaxySector);
	UClass* Z_Construct_UClass_AGalaxySector_NoRegister()
	{
		return AGalaxySector::StaticClass();
	}
	struct Z_Construct_UClass_AGalaxySector_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGalaxySector_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialSystem,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGalaxySector_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "GalaxySector.h" },
		{ "ModuleRelativePath", "GalaxySector.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGalaxySector_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGalaxySector>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGalaxySector_Statics::ClassParams = {
		&AGalaxySector::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AGalaxySector_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxySector_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGalaxySector()
	{
		if (!Z_Registration_Info_UClass_AGalaxySector.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGalaxySector.OuterSingleton, Z_Construct_UClass_AGalaxySector_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGalaxySector.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGalaxySector>()
	{
		return AGalaxySector::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGalaxySector);
	AGalaxySector::~AGalaxySector() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxySector_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxySector_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGalaxySector, AGalaxySector::StaticClass, TEXT("AGalaxySector"), &Z_Registration_Info_UClass_AGalaxySector, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGalaxySector), 427852772U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxySector_h_3060647528(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxySector_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxySector_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
