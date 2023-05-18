// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Star.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStar() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialBody();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AStar::StaticRegisterNativesAStar()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AStar);
	UClass* Z_Construct_UClass_AStar_NoRegister()
	{
		return AStar::StaticClass();
	}
	struct Z_Construct_UClass_AStar_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AStar_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialBody,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Star.h" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AStar_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AStar>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AStar_Statics::ClassParams = {
		&AStar::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AStar()
	{
		if (!Z_Registration_Info_UClass_AStar.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AStar.OuterSingleton, Z_Construct_UClass_AStar_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AStar.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AStar>()
	{
		return AStar::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AStar);
	AStar::~AStar() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AStar, AStar::StaticClass, TEXT("AStar"), &Z_Registration_Info_UClass_AStar, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AStar), 515391313U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_1458754565(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
