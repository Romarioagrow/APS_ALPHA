// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Environment.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeEnvironment() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AEnvironment();
	APS_ALPHA_API UClass* Z_Construct_UClass_AEnvironment_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AWorldActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AEnvironment::StaticRegisterNativesAEnvironment()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AEnvironment);
	UClass* Z_Construct_UClass_AEnvironment_NoRegister()
	{
		return AEnvironment::StaticClass();
	}
	struct Z_Construct_UClass_AEnvironment_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AEnvironment_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AWorldActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AEnvironment_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Environment.h" },
		{ "ModuleRelativePath", "Environment.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AEnvironment_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AEnvironment>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AEnvironment_Statics::ClassParams = {
		&AEnvironment::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AEnvironment_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AEnvironment_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AEnvironment()
	{
		if (!Z_Registration_Info_UClass_AEnvironment.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AEnvironment.OuterSingleton, Z_Construct_UClass_AEnvironment_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AEnvironment.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AEnvironment>()
	{
		return AEnvironment::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AEnvironment);
	AEnvironment::~AEnvironment() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Environment_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Environment_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AEnvironment, AEnvironment::StaticClass, TEXT("AEnvironment"), &Z_Registration_Info_UClass_AEnvironment, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AEnvironment), 2643612417U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Environment_h_3346363651(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Environment_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Environment_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
