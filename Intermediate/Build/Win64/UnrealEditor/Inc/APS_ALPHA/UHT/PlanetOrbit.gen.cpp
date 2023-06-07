// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetOrbit.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetOrbit() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetOrbit();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetOrbit_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void APlanetOrbit::StaticRegisterNativesAPlanetOrbit()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(APlanetOrbit);
	UClass* Z_Construct_UClass_APlanetOrbit_NoRegister()
	{
		return APlanetOrbit::StaticClass();
	}
	struct Z_Construct_UClass_APlanetOrbit_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_APlanetOrbit_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AAstroActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetOrbit_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "PlanetOrbit.h" },
		{ "ModuleRelativePath", "PlanetOrbit.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_APlanetOrbit_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<APlanetOrbit>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_APlanetOrbit_Statics::ClassParams = {
		&APlanetOrbit::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_APlanetOrbit_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetOrbit_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_APlanetOrbit()
	{
		if (!Z_Registration_Info_UClass_APlanetOrbit.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_APlanetOrbit.OuterSingleton, Z_Construct_UClass_APlanetOrbit_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_APlanetOrbit.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<APlanetOrbit>()
	{
		return APlanetOrbit::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(APlanetOrbit);
	APlanetOrbit::~APlanetOrbit() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetOrbit_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetOrbit_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_APlanetOrbit, APlanetOrbit::StaticClass, TEXT("APlanetOrbit"), &Z_Registration_Info_UClass_APlanetOrbit, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(APlanetOrbit), 323359548U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetOrbit_h_3976723566(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetOrbit_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetOrbit_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
