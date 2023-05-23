// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/KuiperBelt.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeKuiperBelt() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAsteroidBelt();
	APS_ALPHA_API UClass* Z_Construct_UClass_AKuiperBelt();
	APS_ALPHA_API UClass* Z_Construct_UClass_AKuiperBelt_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AKuiperBelt::StaticRegisterNativesAKuiperBelt()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AKuiperBelt);
	UClass* Z_Construct_UClass_AKuiperBelt_NoRegister()
	{
		return AKuiperBelt::StaticClass();
	}
	struct Z_Construct_UClass_AKuiperBelt_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AKuiperBelt_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AAsteroidBelt,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AKuiperBelt_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "KuiperBelt.h" },
		{ "ModuleRelativePath", "KuiperBelt.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AKuiperBelt_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AKuiperBelt>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AKuiperBelt_Statics::ClassParams = {
		&AKuiperBelt::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AKuiperBelt_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AKuiperBelt_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AKuiperBelt()
	{
		if (!Z_Registration_Info_UClass_AKuiperBelt.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AKuiperBelt.OuterSingleton, Z_Construct_UClass_AKuiperBelt_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AKuiperBelt.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AKuiperBelt>()
	{
		return AKuiperBelt::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AKuiperBelt);
	AKuiperBelt::~AKuiperBelt() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_KuiperBelt_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_KuiperBelt_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AKuiperBelt, AKuiperBelt::StaticClass, TEXT("AKuiperBelt"), &Z_Registration_Info_UClass_AKuiperBelt, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AKuiperBelt), 3622827405U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_KuiperBelt_h_597065385(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_KuiperBelt_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_KuiperBelt_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
