// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/CatalogStarGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCatalogStarGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACatalogStarGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_ACatalogStarGenerator_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	ENGINE_API UClass* Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ACatalogStarGenerator::StaticRegisterNativesACatalogStarGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ACatalogStarGenerator);
	UClass* Z_Construct_UClass_ACatalogStarGenerator_NoRegister()
	{
		return ACatalogStarGenerator::StaticClass();
	}
	struct Z_Construct_UClass_ACatalogStarGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HISMComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_HISMComponent;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ACatalogStarGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACatalogStarGenerator_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "CatalogStarGenerator.h" },
		{ "ModuleRelativePath", "CatalogStarGenerator.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACatalogStarGenerator_Statics::NewProp_HISMComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "Comment", "/*TMap<FString, UHierarchicalInstancedStaticMeshComponent*> HISMMap;*/" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "CatalogStarGenerator.h" },
		{ "ToolTip", "TMap<FString, UHierarchicalInstancedStaticMeshComponent*> HISMMap;" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ACatalogStarGenerator_Statics::NewProp_HISMComponent = { "HISMComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(ACatalogStarGenerator, HISMComponent), Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_ACatalogStarGenerator_Statics::NewProp_HISMComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_ACatalogStarGenerator_Statics::NewProp_HISMComponent_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ACatalogStarGenerator_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ACatalogStarGenerator_Statics::NewProp_HISMComponent,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_ACatalogStarGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ACatalogStarGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ACatalogStarGenerator_Statics::ClassParams = {
		&ACatalogStarGenerator::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_ACatalogStarGenerator_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_ACatalogStarGenerator_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_ACatalogStarGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ACatalogStarGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ACatalogStarGenerator()
	{
		if (!Z_Registration_Info_UClass_ACatalogStarGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ACatalogStarGenerator.OuterSingleton, Z_Construct_UClass_ACatalogStarGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ACatalogStarGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ACatalogStarGenerator>()
	{
		return ACatalogStarGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ACatalogStarGenerator);
	ACatalogStarGenerator::~ACatalogStarGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CatalogStarGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CatalogStarGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ACatalogStarGenerator, ACatalogStarGenerator::StaticClass, TEXT("ACatalogStarGenerator"), &Z_Registration_Info_UClass_ACatalogStarGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ACatalogStarGenerator), 3161213073U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CatalogStarGenerator_h_737185428(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CatalogStarGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CatalogStarGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
