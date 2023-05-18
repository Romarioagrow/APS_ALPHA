// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravitySource.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravitySource() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityObject();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravitySource();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravitySource_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UGravitySource::StaticRegisterNativesUGravitySource()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGravitySource);
	UClass* Z_Construct_UClass_UGravitySource_NoRegister()
	{
		return UGravitySource::StaticClass();
	}
	struct Z_Construct_UClass_UGravitySource_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGravitySource_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGravityObject,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGravitySource_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravitySource.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGravitySource_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IGravitySource>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGravitySource_Statics::ClassParams = {
		&UGravitySource::StaticClass,
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
		0x000840A1u,
		METADATA_PARAMS(Z_Construct_UClass_UGravitySource_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UGravitySource_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UGravitySource()
	{
		if (!Z_Registration_Info_UClass_UGravitySource.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGravitySource.OuterSingleton, Z_Construct_UClass_UGravitySource_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGravitySource.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UGravitySource>()
	{
		return UGravitySource::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGravitySource);
	UGravitySource::~UGravitySource() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravitySource_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravitySource_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGravitySource, UGravitySource::StaticClass, TEXT("UGravitySource"), &Z_Registration_Info_UClass_UGravitySource, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGravitySource), 1846880873U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravitySource_h_1880328132(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravitySource_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravitySource_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
