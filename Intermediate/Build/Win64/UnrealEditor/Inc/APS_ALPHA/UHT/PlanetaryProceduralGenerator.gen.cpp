// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetaryProceduralGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetaryProceduralGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetarySystemGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UPlanetarySystemGenerator::StaticRegisterNativesUPlanetarySystemGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPlanetarySystemGenerator);
	UClass* Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister()
	{
		return UPlanetarySystemGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UPlanetarySystemGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPlanetarySystemGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseProceduralGenerator,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPlanetarySystemGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "PlanetaryProceduralGenerator.h" },
		{ "ModuleRelativePath", "PlanetaryProceduralGenerator.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPlanetarySystemGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPlanetarySystemGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPlanetarySystemGenerator_Statics::ClassParams = {
		&UPlanetarySystemGenerator::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UPlanetarySystemGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UPlanetarySystemGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UPlanetarySystemGenerator()
	{
		if (!Z_Registration_Info_UClass_UPlanetarySystemGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPlanetarySystemGenerator.OuterSingleton, Z_Construct_UClass_UPlanetarySystemGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPlanetarySystemGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UPlanetarySystemGenerator>()
	{
		return UPlanetarySystemGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPlanetarySystemGenerator);
	UPlanetarySystemGenerator::~UPlanetarySystemGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPlanetarySystemGenerator, UPlanetarySystemGenerator::StaticClass, TEXT("UPlanetarySystemGenerator"), &Z_Registration_Info_UClass_UPlanetarySystemGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPlanetarySystemGenerator), 1692124698U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_2631518379(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
