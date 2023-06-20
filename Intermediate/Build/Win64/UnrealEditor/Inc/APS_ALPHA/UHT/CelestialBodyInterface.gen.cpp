// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/CelestialBodyInterface.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCelestialBodyInterface() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UCelestialBodyInterface();
	APS_ALPHA_API UClass* Z_Construct_UClass_UCelestialBodyInterface_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UCelestialBodyInterface::StaticRegisterNativesUCelestialBodyInterface()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCelestialBodyInterface);
	UClass* Z_Construct_UClass_UCelestialBodyInterface_NoRegister()
	{
		return UCelestialBodyInterface::StaticClass();
	}
	struct Z_Construct_UClass_UCelestialBodyInterface_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCelestialBodyInterface_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCelestialBodyInterface_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "CelestialBodyInterface.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCelestialBodyInterface_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ICelestialBodyInterface>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCelestialBodyInterface_Statics::ClassParams = {
		&UCelestialBodyInterface::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UCelestialBodyInterface_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UCelestialBodyInterface_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UCelestialBodyInterface()
	{
		if (!Z_Registration_Info_UClass_UCelestialBodyInterface.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCelestialBodyInterface.OuterSingleton, Z_Construct_UClass_UCelestialBodyInterface_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCelestialBodyInterface.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UCelestialBodyInterface>()
	{
		return UCelestialBodyInterface::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCelestialBodyInterface);
	UCelestialBodyInterface::~UCelestialBodyInterface() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBodyInterface_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBodyInterface_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCelestialBodyInterface, UCelestialBodyInterface::StaticClass, TEXT("UCelestialBodyInterface"), &Z_Registration_Info_UClass_UCelestialBodyInterface, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCelestialBodyInterface), 3860820788U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBodyInterface_h_1852779389(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBodyInterface_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBodyInterface_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
