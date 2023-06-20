// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarGenerator_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UStarGenerator::StaticRegisterNativesUStarGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UStarGenerator);
	UClass* Z_Construct_UClass_UStarGenerator_NoRegister()
	{
		return UStarGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UStarGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UStarGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseProceduralGenerator,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UStarGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "StarGenerator.h" },
		{ "ModuleRelativePath", "StarGenerator.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UStarGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UStarGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UStarGenerator_Statics::ClassParams = {
		&UStarGenerator::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UStarGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UStarGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UStarGenerator()
	{
		if (!Z_Registration_Info_UClass_UStarGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UStarGenerator.OuterSingleton, Z_Construct_UClass_UStarGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UStarGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UStarGenerator>()
	{
		return UStarGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UStarGenerator);
	UStarGenerator::~UStarGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UStarGenerator, UStarGenerator::StaticClass, TEXT("UStarGenerator"), &Z_Registration_Info_UClass_UStarGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UStarGenerator), 1458523287U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerator_h_3597877352(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
