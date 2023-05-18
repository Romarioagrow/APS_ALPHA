// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GeneratedWorld.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGeneratedWorld() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGeneratedWorld();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGeneratedWorld_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UMoonGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarSystemGenerator_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGeneratedWorld::StaticRegisterNativesAGeneratedWorld()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGeneratedWorld);
	UClass* Z_Construct_UClass_AGeneratedWorld_NoRegister()
	{
		return AGeneratedWorld::StaticClass();
	}
	struct Z_Construct_UClass_AGeneratedWorld_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarSystemGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarSystemGenerator;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarGenerator;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlanetGenerator;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoonGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_MoonGenerator;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGeneratedWorld_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGeneratedWorld_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "GeneratedWorld.h" },
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarSystemGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarSystemGenerator = { "StarSystemGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGeneratedWorld, StarSystemGenerator), Z_Construct_UClass_UStarSystemGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarSystemGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarSystemGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarGenerator = { "StarGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGeneratedWorld, StarGenerator), Z_Construct_UClass_UStarGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_PlanetGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_PlanetGenerator = { "PlanetGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGeneratedWorld, PlanetGenerator), Z_Construct_UClass_UPlanetGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_PlanetGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_PlanetGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_MoonGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_MoonGenerator = { "MoonGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGeneratedWorld, MoonGenerator), Z_Construct_UClass_UMoonGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_MoonGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_MoonGenerator_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AGeneratedWorld_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarSystemGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_StarGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_PlanetGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGeneratedWorld_Statics::NewProp_MoonGenerator,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGeneratedWorld_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGeneratedWorld>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGeneratedWorld_Statics::ClassParams = {
		&AGeneratedWorld::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AGeneratedWorld_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AGeneratedWorld_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AGeneratedWorld_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGeneratedWorld_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGeneratedWorld()
	{
		if (!Z_Registration_Info_UClass_AGeneratedWorld.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGeneratedWorld.OuterSingleton, Z_Construct_UClass_AGeneratedWorld_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGeneratedWorld.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGeneratedWorld>()
	{
		return AGeneratedWorld::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGeneratedWorld);
	AGeneratedWorld::~AGeneratedWorld() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGeneratedWorld, AGeneratedWorld::StaticClass, TEXT("AGeneratedWorld"), &Z_Registration_Info_UClass_AGeneratedWorld, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGeneratedWorld), 1569988385U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_3946015089(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
