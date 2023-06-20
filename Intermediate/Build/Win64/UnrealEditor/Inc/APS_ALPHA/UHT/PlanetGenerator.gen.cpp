// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetGenerator_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UPlanetGenerator::StaticRegisterNativesUPlanetGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPlanetGenerator);
	UClass* Z_Construct_UClass_UPlanetGenerator_NoRegister()
	{
		return UPlanetGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UPlanetGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPlanetGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseProceduralGenerator,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPlanetGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "PlanetGenerator.h" },
		{ "ModuleRelativePath", "PlanetGenerator.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPlanetGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPlanetGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPlanetGenerator_Statics::ClassParams = {
		&UPlanetGenerator::StaticClass,
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
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UPlanetGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UPlanetGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UPlanetGenerator()
	{
		if (!Z_Registration_Info_UClass_UPlanetGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPlanetGenerator.OuterSingleton, Z_Construct_UClass_UPlanetGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPlanetGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UPlanetGenerator>()
	{
		return UPlanetGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPlanetGenerator);
	UPlanetGenerator::~UPlanetGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPlanetGenerator, UPlanetGenerator::StaticClass, TEXT("UPlanetGenerator"), &Z_Registration_Info_UClass_UPlanetGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPlanetGenerator), 1096021581U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerator_h_2370316272(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
