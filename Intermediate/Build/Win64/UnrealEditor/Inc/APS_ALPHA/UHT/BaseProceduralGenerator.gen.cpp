// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/BaseProceduralGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeBaseProceduralGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UBaseProceduralGenerator::StaticRegisterNativesUBaseProceduralGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UBaseProceduralGenerator);
	UClass* Z_Construct_UClass_UBaseProceduralGenerator_NoRegister()
	{
		return UBaseProceduralGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UBaseProceduralGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UBaseProceduralGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UBaseProceduralGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "BaseProceduralGenerator.h" },
		{ "ModuleRelativePath", "BaseProceduralGenerator.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UBaseProceduralGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UBaseProceduralGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UBaseProceduralGenerator_Statics::ClassParams = {
		&UBaseProceduralGenerator::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UBaseProceduralGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UBaseProceduralGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UBaseProceduralGenerator()
	{
		if (!Z_Registration_Info_UClass_UBaseProceduralGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UBaseProceduralGenerator.OuterSingleton, Z_Construct_UClass_UBaseProceduralGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UBaseProceduralGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UBaseProceduralGenerator>()
	{
		return UBaseProceduralGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UBaseProceduralGenerator);
	UBaseProceduralGenerator::~UBaseProceduralGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_BaseProceduralGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_BaseProceduralGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UBaseProceduralGenerator, UBaseProceduralGenerator::StaticClass, TEXT("UBaseProceduralGenerator"), &Z_Registration_Info_UClass_UBaseProceduralGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UBaseProceduralGenerator), 2750329783U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_BaseProceduralGenerator_h_3117258852(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_BaseProceduralGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_BaseProceduralGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
