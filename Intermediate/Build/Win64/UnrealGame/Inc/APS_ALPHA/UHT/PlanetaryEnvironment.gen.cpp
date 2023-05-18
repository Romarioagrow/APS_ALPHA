// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetaryEnvironment.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetaryEnvironment() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseInterface();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetaryEnvironment();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetaryEnvironment_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UPlanetaryEnvironment::StaticRegisterNativesUPlanetaryEnvironment()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPlanetaryEnvironment);
	UClass* Z_Construct_UClass_UPlanetaryEnvironment_NoRegister()
	{
		return UPlanetaryEnvironment::StaticClass();
	}
	struct Z_Construct_UClass_UPlanetaryEnvironment_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPlanetaryEnvironment_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPlanetaryEnvironment_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "PlanetaryEnvironment.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPlanetaryEnvironment_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IPlanetaryEnvironment>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPlanetaryEnvironment_Statics::ClassParams = {
		&UPlanetaryEnvironment::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UPlanetaryEnvironment_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UPlanetaryEnvironment_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UPlanetaryEnvironment()
	{
		if (!Z_Registration_Info_UClass_UPlanetaryEnvironment.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPlanetaryEnvironment.OuterSingleton, Z_Construct_UClass_UPlanetaryEnvironment_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPlanetaryEnvironment.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UPlanetaryEnvironment>()
	{
		return UPlanetaryEnvironment::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPlanetaryEnvironment);
	UPlanetaryEnvironment::~UPlanetaryEnvironment() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryEnvironment_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryEnvironment_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPlanetaryEnvironment, UPlanetaryEnvironment::StaticClass, TEXT("UPlanetaryEnvironment"), &Z_Registration_Info_UClass_UPlanetaryEnvironment, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPlanetaryEnvironment), 3707047530U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryEnvironment_h_133513206(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryEnvironment_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryEnvironment_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
