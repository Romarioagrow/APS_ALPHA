// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/TechInfrastructure.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTechInfrastructure() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechInfrastructure();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechInfrastructure_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ATechInfrastructure::StaticRegisterNativesATechInfrastructure()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ATechInfrastructure);
	UClass* Z_Construct_UClass_ATechInfrastructure_NoRegister()
	{
		return ATechInfrastructure::StaticClass();
	}
	struct Z_Construct_UClass_ATechInfrastructure_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ATechInfrastructure_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ATechActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ATechInfrastructure_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "TechInfrastructure.h" },
		{ "ModuleRelativePath", "TechInfrastructure.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_ATechInfrastructure_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ATechInfrastructure>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ATechInfrastructure_Statics::ClassParams = {
		&ATechInfrastructure::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_ATechInfrastructure_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ATechInfrastructure_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ATechInfrastructure()
	{
		if (!Z_Registration_Info_UClass_ATechInfrastructure.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ATechInfrastructure.OuterSingleton, Z_Construct_UClass_ATechInfrastructure_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ATechInfrastructure.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ATechInfrastructure>()
	{
		return ATechInfrastructure::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ATechInfrastructure);
	ATechInfrastructure::~ATechInfrastructure() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechInfrastructure_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechInfrastructure_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ATechInfrastructure, ATechInfrastructure::StaticClass, TEXT("ATechInfrastructure"), &Z_Registration_Info_UClass_ATechInfrastructure, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ATechInfrastructure), 3577551421U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechInfrastructure_h_3098148306(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechInfrastructure_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechInfrastructure_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
