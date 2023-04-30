// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AstonomicalSystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAstonomicalSystem() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstonomicalSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstonomicalSystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstroObject();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UAstonomicalSystem::StaticRegisterNativesUAstonomicalSystem()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAstonomicalSystem);
	UClass* Z_Construct_UClass_UAstonomicalSystem_NoRegister()
	{
		return UAstonomicalSystem::StaticClass();
	}
	struct Z_Construct_UClass_UAstonomicalSystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAstonomicalSystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UAstroObject,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAstonomicalSystem_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "AstonomicalSystem.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAstonomicalSystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IAstonomicalSystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UAstonomicalSystem_Statics::ClassParams = {
		&UAstonomicalSystem::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UAstonomicalSystem_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UAstonomicalSystem_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UAstonomicalSystem()
	{
		if (!Z_Registration_Info_UClass_UAstonomicalSystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAstonomicalSystem.OuterSingleton, Z_Construct_UClass_UAstonomicalSystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UAstonomicalSystem.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UAstonomicalSystem>()
	{
		return UAstonomicalSystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAstonomicalSystem);
	UAstonomicalSystem::~UAstonomicalSystem() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstonomicalSystem_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstonomicalSystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UAstonomicalSystem, UAstonomicalSystem::StaticClass, TEXT("UAstonomicalSystem"), &Z_Registration_Info_UClass_UAstonomicalSystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAstonomicalSystem), 1538104280U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstonomicalSystem_h_586770707(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstonomicalSystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstonomicalSystem_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
