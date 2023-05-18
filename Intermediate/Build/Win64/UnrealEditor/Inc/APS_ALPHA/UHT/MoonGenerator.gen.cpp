// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/MoonGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMoonGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UMoonGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UMoonGenerator_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UMoonGenerator::StaticRegisterNativesUMoonGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UMoonGenerator);
	UClass* Z_Construct_UClass_UMoonGenerator_NoRegister()
	{
		return UMoonGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UMoonGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UMoonGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseProceduralGenerator,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UMoonGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "MoonGenerator.h" },
		{ "ModuleRelativePath", "MoonGenerator.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UMoonGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UMoonGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UMoonGenerator_Statics::ClassParams = {
		&UMoonGenerator::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UMoonGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UMoonGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UMoonGenerator()
	{
		if (!Z_Registration_Info_UClass_UMoonGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UMoonGenerator.OuterSingleton, Z_Construct_UClass_UMoonGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UMoonGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UMoonGenerator>()
	{
		return UMoonGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UMoonGenerator);
	UMoonGenerator::~UMoonGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UMoonGenerator, UMoonGenerator::StaticClass, TEXT("UMoonGenerator"), &Z_Registration_Info_UClass_UMoonGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UMoonGenerator), 2621780759U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerator_h_2147455930(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
