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
	APS_ALPHA_API UClass* Z_Construct_UClass_AMoon_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarClusterGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarClusterGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarSystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UMoonGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarSystemGenerator_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AStarClusterGenerator::StaticRegisterNativesAStarClusterGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AStarClusterGenerator);
	UClass* Z_Construct_UClass_AStarClusterGenerator_NoRegister()
	{
		return AStarClusterGenerator::StaticClass();
	}
	struct Z_Construct_UClass_AStarClusterGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GeneratedStarSystems_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedStarSystems_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_GeneratedStarSystems;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarSystemGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarSystemGenerator;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetarySystemGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlanetarySystemGenerator;
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
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_PlanetarySystemClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_PlanetarySystemClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_StarSystemClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_StarSystemClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_StarClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_StarClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_PlanetClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_PlanetClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_MoonClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_MoonClass;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AStarClusterGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "GeneratedWorld.h" },
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_GeneratedStarSystems_Inner = { "GeneratedStarSystems", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UClass_AStarSystem_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_GeneratedStarSystems_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_GeneratedStarSystems = { "GeneratedStarSystems", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, GeneratedStarSystems), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_GeneratedStarSystems_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_GeneratedStarSystems_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarSystemGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarSystemGenerator = { "StarSystemGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, StarSystemGenerator), Z_Construct_UClass_UStarSystemGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarSystemGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarSystemGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetarySystemGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetarySystemGenerator = { "PlanetarySystemGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, PlanetarySystemGenerator), Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetarySystemGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetarySystemGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarGenerator = { "StarGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, StarGenerator), Z_Construct_UClass_UStarGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetGenerator = { "PlanetGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, PlanetGenerator), Z_Construct_UClass_UPlanetGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_MoonGenerator_MetaData[] = {
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_MoonGenerator = { "MoonGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, MoonGenerator), Z_Construct_UClass_UMoonGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_MoonGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_MoonGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetarySystemClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetarySystemClass = { "BP_PlanetarySystemClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, BP_PlanetarySystemClass), Z_Construct_UClass_UClass, Z_Construct_UClass_APlanetarySystem_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetarySystemClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetarySystemClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarSystemClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarSystemClass = { "BP_StarSystemClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, BP_StarSystemClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AStarSystem_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarSystemClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarSystemClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarClass = { "BP_StarClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, BP_StarClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AStar_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetClass = { "BP_PlanetClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, BP_PlanetClass), Z_Construct_UClass_UClass, Z_Construct_UClass_APlanet_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_MoonClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "GeneratedWorld.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_MoonClass = { "BP_MoonClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarClusterGenerator, BP_MoonClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AMoon_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_MoonClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_MoonClass_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AStarClusterGenerator_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_GeneratedStarSystems_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_GeneratedStarSystems,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarSystemGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetarySystemGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_StarGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_PlanetGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_MoonGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetarySystemClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarSystemClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_StarClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_PlanetClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarClusterGenerator_Statics::NewProp_BP_MoonClass,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AStarClusterGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AStarClusterGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AStarClusterGenerator_Statics::ClassParams = {
		&AStarClusterGenerator::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AStarClusterGenerator_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AStarClusterGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AStarClusterGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AStarClusterGenerator()
	{
		if (!Z_Registration_Info_UClass_AStarClusterGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AStarClusterGenerator.OuterSingleton, Z_Construct_UClass_AStarClusterGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AStarClusterGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AStarClusterGenerator>()
	{
		return AStarClusterGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AStarClusterGenerator);
	AStarClusterGenerator::~AStarClusterGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AStarClusterGenerator, AStarClusterGenerator::StaticClass, TEXT("AStarClusterGenerator"), &Z_Registration_Info_UClass_AStarClusterGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AStarClusterGenerator), 2267168927U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_4067836552(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GeneratedWorld_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
