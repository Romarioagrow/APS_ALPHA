// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AstroGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAstroGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AMoon_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetOrbit_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarCluster_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarSystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UMoonGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarClusterGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarSystemGenerator_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterSize();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterType();
	COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AAstroGenerator::StaticRegisterNativesAAstroGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAstroGenerator);
	UClass* Z_Construct_UClass_AAstroGenerator_NoRegister()
	{
		return AAstroGenerator::StaticClass();
	}
	struct Z_Construct_UClass_AAstroGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bNeedOrbitRotation_MetaData[];
#endif
		static void NewProp_bNeedOrbitRotation_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bNeedOrbitRotation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateHomeSystem_MetaData[];
#endif
		static void NewProp_bGenerateHomeSystem_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateHomeSystem;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateStarCluster_MetaData[];
#endif
		static void NewProp_bGenerateStarCluster_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateStarCluster;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateFullScaledStarCluster_MetaData[];
#endif
		static void NewProp_bGenerateFullScaledStarCluster_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateFullScaledStarCluster;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateRandomCluster_MetaData[];
#endif
		static void NewProp_bGenerateRandomCluster_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateRandomCluster;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarClusterSize_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarClusterSize_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarClusterSize;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarClusterType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarClusterType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarClusterType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarClusterPopulation_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarClusterPopulation_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarClusterPopulation;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarClusterComposition_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarClusterComposition_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarClusterComposition;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSystemRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_HomeSystemRadius;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GeneratedStarSystems_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedStarSystems_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_GeneratedStarSystems;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarClusterGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarClusterGenerator;
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
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_StarClusterClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_StarClusterClass;
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
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_PlanetOrbit_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_PlanetOrbit;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_MoonClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_MoonClass;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAstroGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "AstroGenerator.h" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bNeedOrbitRotation = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation = { "bNeedOrbitRotation", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bGenerateHomeSystem = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem = { "bGenerateHomeSystem", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateStarCluster_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateStarCluster_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bGenerateStarCluster = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateStarCluster = { "bGenerateStarCluster", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateStarCluster_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateStarCluster_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateStarCluster_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledStarCluster_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledStarCluster_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bGenerateFullScaledStarCluster = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledStarCluster = { "bGenerateFullScaledStarCluster", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledStarCluster_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledStarCluster_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledStarCluster_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bGenerateRandomCluster = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster = { "bGenerateRandomCluster", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "EditCondition", "!bGenerateRandomCluster" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize = { "StarClusterSize", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarClusterSize), Z_Construct_UEnum_APS_ALPHA_EStarClusterSize, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize_MetaData)) }; // 1922975825
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "EditCondition", "!bGenerateRandomCluster" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType = { "StarClusterType", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarClusterType), Z_Construct_UEnum_APS_ALPHA_EStarClusterType, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType_MetaData)) }; // 651664193
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "EditCondition", "!bGenerateRandomCluster" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation = { "StarClusterPopulation", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarClusterPopulation), Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation_MetaData)) }; // 2512550455
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "EditCondition", "!bGenerateRandomCluster" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition = { "StarClusterComposition", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarClusterComposition), Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition_MetaData)) }; // 399386891
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius = { "HomeSystemRadius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSystemRadius), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius_MetaData)) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_Inner = { "GeneratedStarSystems", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UClass_AStarSystem_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems = { "GeneratedStarSystems", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GeneratedStarSystems), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterGenerator_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterGenerator = { "StarClusterGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarClusterGenerator), Z_Construct_UClass_UStarClusterGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemGenerator_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemGenerator = { "StarSystemGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarSystemGenerator), Z_Construct_UClass_UStarSystemGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetarySystemGenerator_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetarySystemGenerator = { "PlanetarySystemGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, PlanetarySystemGenerator), Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetarySystemGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetarySystemGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarGenerator_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarGenerator = { "StarGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarGenerator), Z_Construct_UClass_UStarGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetGenerator_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetGenerator = { "PlanetGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, PlanetGenerator), Z_Construct_UClass_UPlanetGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_MoonGenerator_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_MoonGenerator = { "MoonGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, MoonGenerator), Z_Construct_UClass_UMoonGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_MoonGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_MoonGenerator_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClusterClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClusterClass = { "BP_StarClusterClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_StarClusterClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AStarCluster_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClusterClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClusterClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetarySystemClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetarySystemClass = { "BP_PlanetarySystemClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_PlanetarySystemClass), Z_Construct_UClass_UClass, Z_Construct_UClass_APlanetarySystem_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetarySystemClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetarySystemClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarSystemClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarSystemClass = { "BP_StarSystemClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_StarSystemClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AStarSystem_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarSystemClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarSystemClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClass = { "BP_StarClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_StarClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AStar_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetClass = { "BP_PlanetClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_PlanetClass), Z_Construct_UClass_UClass, Z_Construct_UClass_APlanet_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetOrbit_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetOrbit = { "BP_PlanetOrbit", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_PlanetOrbit), Z_Construct_UClass_UClass, Z_Construct_UClass_APlanetOrbit_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetOrbit_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetOrbit_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_MoonClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_MoonClass = { "BP_MoonClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_MoonClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AMoon_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_MoonClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_MoonClass_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AAstroGenerator_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateStarCluster,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledStarCluster,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetarySystemGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_MoonGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClusterClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetarySystemClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarSystemClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_StarClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_PlanetOrbit,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_MoonClass,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAstroGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAstroGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAstroGenerator_Statics::ClassParams = {
		&AAstroGenerator::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AAstroGenerator_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAstroGenerator()
	{
		if (!Z_Registration_Info_UClass_AAstroGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAstroGenerator.OuterSingleton, Z_Construct_UClass_AAstroGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAstroGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AAstroGenerator>()
	{
		return AAstroGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAstroGenerator);
	AAstroGenerator::~AAstroGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAstroGenerator, AAstroGenerator::StaticClass, TEXT("AAstroGenerator"), &Z_Registration_Info_UClass_AAstroGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAstroGenerator), 327815485U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_1771020928(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
