// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AutonomousOutpost.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAutonomousOutpost() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAutonomousOutpost();
	APS_ALPHA_API UClass* Z_Construct_UClass_AAutonomousOutpost_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AAutonomousOutpost::StaticRegisterNativesAAutonomousOutpost()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAutonomousOutpost);
	UClass* Z_Construct_UClass_AAutonomousOutpost_NoRegister()
	{
		return AAutonomousOutpost::StaticClass();
	}
	struct Z_Construct_UClass_AAutonomousOutpost_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAutonomousOutpost_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ATechActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAutonomousOutpost_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "AutonomousOutpost.h" },
		{ "ModuleRelativePath", "AutonomousOutpost.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAutonomousOutpost_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAutonomousOutpost>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAutonomousOutpost_Statics::ClassParams = {
		&AAutonomousOutpost::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AAutonomousOutpost_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAutonomousOutpost_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAutonomousOutpost()
	{
		if (!Z_Registration_Info_UClass_AAutonomousOutpost.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAutonomousOutpost.OuterSingleton, Z_Construct_UClass_AAutonomousOutpost_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAutonomousOutpost.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AAutonomousOutpost>()
	{
		return AAutonomousOutpost::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAutonomousOutpost);
	AAutonomousOutpost::~AAutonomousOutpost() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AutonomousOutpost_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AutonomousOutpost_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAutonomousOutpost, AAutonomousOutpost::StaticClass, TEXT("AAutonomousOutpost"), &Z_Registration_Info_UClass_AAutonomousOutpost, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAutonomousOutpost), 2356528696U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AutonomousOutpost_h_933004370(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AutonomousOutpost_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AutonomousOutpost_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
