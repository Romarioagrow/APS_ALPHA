// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GalaxyGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxyGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGalaxyGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGalaxyGenerator_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UGalaxyGenerator::StaticRegisterNativesUGalaxyGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGalaxyGenerator);
	UClass* Z_Construct_UClass_UGalaxyGenerator_NoRegister()
	{
		return UGalaxyGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UGalaxyGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGalaxyGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseProceduralGenerator,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGalaxyGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "GalaxyGenerator.h" },
		{ "ModuleRelativePath", "GalaxyGenerator.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGalaxyGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGalaxyGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGalaxyGenerator_Statics::ClassParams = {
		&UGalaxyGenerator::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UGalaxyGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UGalaxyGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UGalaxyGenerator()
	{
		if (!Z_Registration_Info_UClass_UGalaxyGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGalaxyGenerator.OuterSingleton, Z_Construct_UClass_UGalaxyGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGalaxyGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UGalaxyGenerator>()
	{
		return UGalaxyGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGalaxyGenerator);
	UGalaxyGenerator::~UGalaxyGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGalaxyGenerator, UGalaxyGenerator::StaticClass, TEXT("UGalaxyGenerator"), &Z_Registration_Info_UClass_UGalaxyGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGalaxyGenerator), 954813762U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyGenerator_h_3875354567(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS