// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Geosphere.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGeosphere() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AEnvironment();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGeosphere();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGeosphere_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGeosphere::StaticRegisterNativesAGeosphere()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGeosphere);
	UClass* Z_Construct_UClass_AGeosphere_NoRegister()
	{
		return AGeosphere::StaticClass();
	}
	struct Z_Construct_UClass_AGeosphere_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGeosphere_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AEnvironment,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGeosphere_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Geosphere.h" },
		{ "ModuleRelativePath", "Geosphere.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGeosphere_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGeosphere>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGeosphere_Statics::ClassParams = {
		&AGeosphere::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AGeosphere_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGeosphere_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGeosphere()
	{
		if (!Z_Registration_Info_UClass_AGeosphere.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGeosphere.OuterSingleton, Z_Construct_UClass_AGeosphere_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGeosphere.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGeosphere>()
	{
		return AGeosphere::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGeosphere);
	AGeosphere::~AGeosphere() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Geosphere_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Geosphere_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGeosphere, AGeosphere::StaticClass, TEXT("AGeosphere"), &Z_Registration_Info_UClass_AGeosphere, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGeosphere), 591051135U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Geosphere_h_341640306(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Geosphere_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Geosphere_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
