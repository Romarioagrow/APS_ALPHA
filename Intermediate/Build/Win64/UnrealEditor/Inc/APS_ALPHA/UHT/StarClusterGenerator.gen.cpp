// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarClusterGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarClusterGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarCluster_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarClusterGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarClusterGenerator_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UStarClusterGenerator::StaticRegisterNativesUStarClusterGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UStarClusterGenerator);
	UClass* Z_Construct_UClass_UStarClusterGenerator_NoRegister()
	{
		return UStarClusterGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UStarClusterGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_StarClusterClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_StarClusterClass;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UStarClusterGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseProceduralGenerator,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UStarClusterGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "StarClusterGenerator.h" },
		{ "ModuleRelativePath", "StarClusterGenerator.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UStarClusterGenerator_Statics::NewProp_BP_StarClusterClass_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "StarClusterGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_UStarClusterGenerator_Statics::NewProp_BP_StarClusterClass = { "BP_StarClusterClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(UStarClusterGenerator, BP_StarClusterClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AStarCluster_NoRegister, METADATA_PARAMS(Z_Construct_UClass_UStarClusterGenerator_Statics::NewProp_BP_StarClusterClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UStarClusterGenerator_Statics::NewProp_BP_StarClusterClass_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UStarClusterGenerator_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UStarClusterGenerator_Statics::NewProp_BP_StarClusterClass,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UStarClusterGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UStarClusterGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UStarClusterGenerator_Statics::ClassParams = {
		&UStarClusterGenerator::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UStarClusterGenerator_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UStarClusterGenerator_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UStarClusterGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UStarClusterGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UStarClusterGenerator()
	{
		if (!Z_Registration_Info_UClass_UStarClusterGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UStarClusterGenerator.OuterSingleton, Z_Construct_UClass_UStarClusterGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UStarClusterGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UStarClusterGenerator>()
	{
		return UStarClusterGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UStarClusterGenerator);
	UStarClusterGenerator::~UStarClusterGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UStarClusterGenerator, UStarClusterGenerator::StaticClass, TEXT("UStarClusterGenerator"), &Z_Registration_Info_UClass_UStarClusterGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UStarClusterGenerator), 2089277880U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterGenerator_h_3015527458(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
