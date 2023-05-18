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
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet_NoRegister();
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
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Planets_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Planets_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_Planets;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
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
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_Planets_Inner = { "Planets", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UClass_APlanet_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_Planets_MetaData[] = {
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_Planets = { "Planets", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, Planets), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_Planets_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_Planets_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AStar_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_Planets_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_Planets,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AStar_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AStar>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AStar_Statics::ClassParams = {
		&AStar::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AStar_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::PropPointers),
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
		{ Z_Construct_UClass_AStar, AStar::StaticClass, TEXT("AStar"), &Z_Registration_Info_UClass_AStar, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AStar), 2389174334U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_2766739925(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
