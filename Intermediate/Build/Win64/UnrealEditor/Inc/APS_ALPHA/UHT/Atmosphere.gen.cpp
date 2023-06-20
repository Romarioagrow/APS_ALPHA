// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Atmosphere.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAtmosphere() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAtmosphere();
	APS_ALPHA_API UClass* Z_Construct_UClass_AAtmosphere_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AEnvironment();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AAtmosphere::StaticRegisterNativesAAtmosphere()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAtmosphere);
	UClass* Z_Construct_UClass_AAtmosphere_NoRegister()
	{
		return AAtmosphere::StaticClass();
	}
	struct Z_Construct_UClass_AAtmosphere_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAtmosphere_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AEnvironment,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAtmosphere_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Atmosphere.h" },
		{ "ModuleRelativePath", "Atmosphere.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAtmosphere_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAtmosphere>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAtmosphere_Statics::ClassParams = {
		&AAtmosphere::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AAtmosphere_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAtmosphere_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAtmosphere()
	{
		if (!Z_Registration_Info_UClass_AAtmosphere.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAtmosphere.OuterSingleton, Z_Construct_UClass_AAtmosphere_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAtmosphere.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AAtmosphere>()
	{
		return AAtmosphere::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAtmosphere);
	AAtmosphere::~AAtmosphere() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Atmosphere_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Atmosphere_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAtmosphere, AAtmosphere::StaticClass, TEXT("AAtmosphere"), &Z_Registration_Info_UClass_AAtmosphere, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAtmosphere), 72902233U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Atmosphere_h_3925323018(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Atmosphere_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Atmosphere_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
