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
	APS_ALPHA_API UClass* Z_Construct_UClass_AControlledPawn_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxy_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AMoon_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetOrbit_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceHeadquarters_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceship_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceShipyard_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceStation_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarCluster_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarSystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGalaxyGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UMoonGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarClusterGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarGenerator_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UStarSystemGenerator_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EHomeSystemPosition();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitHeight();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterSize();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarSystemType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStellarType();
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
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedGalaxy_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GeneratedGalaxy;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedStarCluster_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GeneratedStarCluster;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedHomeStarSystem_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GeneratedHomeStarSystem;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomePlanet_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_HomePlanet;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSpaceHeadquarters_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_HomeSpaceHeadquarters;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSpaceStation_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_HomeSpaceStation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSpaceship_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_HomeSpaceship;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSpaceShipyard_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_HomeSpaceShipyard;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateFullScaledWorld_MetaData[];
#endif
		static void NewProp_bGenerateFullScaledWorld_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateFullScaledWorld;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateHomeSystem_MetaData[];
#endif
		static void NewProp_bGenerateHomeSystem_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateHomeSystem;
		static const UECodeGen_Private::FBytePropertyParams NewProp_AstroGenerationLevel_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AstroGenerationLevel_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_AstroGenerationLevel;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarSystemDeadZone_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_StarSystemDeadZone;
		static const UECodeGen_Private::FBytePropertyParams NewProp_HomeSystemPosition_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSystemPosition_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_HomeSystemPosition;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bFullScaledHomeSystem_MetaData[];
#endif
		static void NewProp_bFullScaledHomeSystem_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bFullScaledHomeSystem;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bNeedOrbitRotation_MetaData[];
#endif
		static void NewProp_bNeedOrbitRotation_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bNeedOrbitRotation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSystemRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_HomeSystemRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bRandomHomeSystem_MetaData[];
#endif
		static void NewProp_bRandomHomeSystem_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bRandomHomeSystem;
		static const UECodeGen_Private::FBytePropertyParams NewProp_HomeSystemStarType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSystemStarType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_HomeSystemStarType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bRandomHomeStar_MetaData[];
#endif
		static void NewProp_bRandomHomeStar_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bRandomHomeStar;
		static const UECodeGen_Private::FBytePropertyParams NewProp_HomeStarStellarType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeStarStellarType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_HomeStarStellarType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_HomeStarSpectralClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeStarSpectralClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_HomeStarSpectralClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bRandomHomeSystemType_MetaData[];
#endif
		static void NewProp_bRandomHomeSystemType_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bRandomHomeSystemType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_HomeSystemPlanetaryType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSystemPlanetaryType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_HomeSystemPlanetaryType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_HomeSystemOrbitDistributionType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSystemOrbitDistributionType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_HomeSystemOrbitDistributionType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateRandomGalaxyCluster_MetaData[];
#endif
		static void NewProp_bGenerateRandomGalaxyCluster_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateRandomGalaxyCluster;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bGenerateRandomGalaxy_MetaData[];
#endif
		static void NewProp_bGenerateRandomGalaxy_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bGenerateRandomGalaxy;
		static const UECodeGen_Private::FBytePropertyParams NewProp_GalaxyType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_GalaxyType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_GalaxyGlass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyGlass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_GalaxyGlass;
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
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxySize_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_GalaxySize;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyStarCount_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_GalaxyStarCount;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyStarDensity_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_GalaxyStarDensity;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bCharacterSpawn_MetaData[];
#endif
		static void NewProp_bCharacterSpawn_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCharacterSpawn;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bCharacterSpawnAtRandomPlanet_MetaData[];
#endif
		static void NewProp_bCharacterSpawnAtRandomPlanet_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCharacterSpawnAtRandomPlanet;
		static const UECodeGen_Private::FBytePropertyParams NewProp_CharSpawnPlace_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CharSpawnPlace_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_CharSpawnPlace;
		static const UECodeGen_Private::FBytePropertyParams NewProp_HomeSpaceStationOrbitHeight_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HomeSpaceStationOrbitHeight_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_HomeSpaceStationOrbitHeight;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_CharacterClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_CharacterClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_HomeSpaceStation_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_HomeSpaceStation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_HomeSpaceship_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_HomeSpaceship;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_HomeSpaceShipyard_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_HomeSpaceShipyard;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_HomeSpaceHeadquarters_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_HomeSpaceHeadquarters;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GeneratedStarSystems_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedStarSystems_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_GeneratedStarSystems;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyGenerator_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GalaxyGenerator;
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
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BP_GalaxyClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_BP_GalaxyClass;
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
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedGalaxy_MetaData[] = {
		{ "Category", "Generated Astro Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedGalaxy = { "GeneratedGalaxy", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GeneratedGalaxy), Z_Construct_UClass_AGalaxy_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedGalaxy_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedGalaxy_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarCluster_MetaData[] = {
		{ "Category", "Generated Astro Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarCluster = { "GeneratedStarCluster", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GeneratedStarCluster), Z_Construct_UClass_AStarCluster_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarCluster_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarCluster_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedHomeStarSystem_MetaData[] = {
		{ "Category", "Generated Astro Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedHomeStarSystem = { "GeneratedHomeStarSystem", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GeneratedHomeStarSystem), Z_Construct_UClass_AStarSystem_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedHomeStarSystem_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedHomeStarSystem_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomePlanet_MetaData[] = {
		{ "Category", "Generated Astro Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomePlanet = { "HomePlanet", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomePlanet), Z_Construct_UClass_APlanet_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomePlanet_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomePlanet_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceHeadquarters_MetaData[] = {
		{ "Category", "Generated Tech Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceHeadquarters = { "HomeSpaceHeadquarters", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSpaceHeadquarters), Z_Construct_UClass_ASpaceHeadquarters_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceHeadquarters_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceHeadquarters_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStation_MetaData[] = {
		{ "Category", "Generated Tech Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStation = { "HomeSpaceStation", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSpaceStation), Z_Construct_UClass_ASpaceStation_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStation_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceship_MetaData[] = {
		{ "Category", "Generated Tech Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceship = { "HomeSpaceship", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSpaceship), Z_Construct_UClass_ASpaceship_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceship_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceship_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceShipyard_MetaData[] = {
		{ "Category", "Generated Tech Actros" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceShipyard = { "HomeSpaceShipyard", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSpaceShipyard), Z_Construct_UClass_ASpaceShipyard_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceShipyard_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceShipyard_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledWorld_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "Comment", "/// BASE ASTRO GENERATOR\n" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
		{ "ToolTip", "BASE ASTRO GENERATOR" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledWorld_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bGenerateFullScaledWorld = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledWorld = { "bGenerateFullScaledWorld", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledWorld_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledWorld_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledWorld_MetaData)) };
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
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_AstroGenerationLevel_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_AstroGenerationLevel_MetaData[] = {
		{ "Category", "Generation Params" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_AstroGenerationLevel = { "AstroGenerationLevel", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, AstroGenerationLevel), Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_AstroGenerationLevel_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_AstroGenerationLevel_MetaData)) }; // 2796898956
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemDeadZone_MetaData[] = {
		{ "Category", "Home System" },
		{ "Comment", "/// STAR SYSTEM GENERATOR\n" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
		{ "ToolTip", "STAR SYSTEM GENERATOR" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemDeadZone = { "StarSystemDeadZone", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, StarSystemDeadZone), METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemDeadZone_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemDeadZone_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPosition_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPosition_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPosition = { "HomeSystemPosition", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSystemPosition), Z_Construct_UEnum_APS_ALPHA_EHomeSystemPosition, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPosition_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPosition_MetaData)) }; // 3061429149
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bFullScaledHomeSystem_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bFullScaledHomeSystem_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bFullScaledHomeSystem = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bFullScaledHomeSystem = { "bFullScaledHomeSystem", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bFullScaledHomeSystem_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bFullScaledHomeSystem_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bFullScaledHomeSystem_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bNeedOrbitRotation = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation = { "bNeedOrbitRotation", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius = { "HomeSystemRadius", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSystemRadius), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystem_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystem_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bRandomHomeSystem = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystem = { "bRandomHomeSystem", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystem_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystem_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystem_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemStarType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemStarType_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemStarType = { "HomeSystemStarType", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSystemStarType), Z_Construct_UEnum_APS_ALPHA_EStarSystemType, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemStarType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemStarType_MetaData)) }; // 1630425490
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeStar_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeStar_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bRandomHomeStar = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeStar = { "bRandomHomeStar", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeStar_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeStar_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeStar_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarStellarType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarStellarType_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarStellarType = { "HomeStarStellarType", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeStarStellarType), Z_Construct_UEnum_APS_ALPHA_EStellarType, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarStellarType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarStellarType_MetaData)) }; // 3048584829
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarSpectralClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarSpectralClass_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarSpectralClass = { "HomeStarSpectralClass", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeStarSpectralClass), Z_Construct_UEnum_APS_ALPHA_ESpectralClass, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarSpectralClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarSpectralClass_MetaData)) }; // 1101356590
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystemType_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystemType_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bRandomHomeSystemType = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystemType = { "bRandomHomeSystemType", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystemType_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystemType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystemType_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPlanetaryType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPlanetaryType_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPlanetaryType = { "HomeSystemPlanetaryType", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSystemPlanetaryType), Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPlanetaryType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPlanetaryType_MetaData)) }; // 3370777339
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemOrbitDistributionType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemOrbitDistributionType_MetaData[] = {
		{ "Category", "Home System" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemOrbitDistributionType = { "HomeSystemOrbitDistributionType", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSystemOrbitDistributionType), Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemOrbitDistributionType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemOrbitDistributionType_MetaData)) }; // 2085035236
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxyCluster_MetaData[] = {
		{ "Category", "Galaxy Cluster" },
		{ "Comment", "/// STAR CLUSTER GENERATOR\n" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
		{ "ToolTip", "STAR CLUSTER GENERATOR" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxyCluster_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bGenerateRandomGalaxyCluster = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxyCluster = { "bGenerateRandomGalaxyCluster", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxyCluster_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxyCluster_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxyCluster_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxy_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxy_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bGenerateRandomGalaxy = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxy = { "bGenerateRandomGalaxy", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxy_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxy_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxy_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyType_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyType = { "GalaxyType", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GalaxyType), Z_Construct_UEnum_APS_ALPHA_EGalaxyType, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyType_MetaData)) }; // 2575129112
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGlass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGlass_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGlass = { "GalaxyGlass", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GalaxyGlass), Z_Construct_UEnum_APS_ALPHA_EGalaxyClass, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGlass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGlass_MetaData)) }; // 1326469318
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster_MetaData[] = {
		{ "Category", "Star Cluster" },
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
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxySize_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxySize = { "GalaxySize", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GalaxySize), METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxySize_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxySize_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarCount_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarCount = { "GalaxyStarCount", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GalaxyStarCount), METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarCount_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarCount_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarDensity_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarDensity = { "GalaxyStarDensity", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GalaxyStarDensity), METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarDensity_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarDensity_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawn_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawn_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bCharacterSpawn = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawn = { "bCharacterSpawn", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawn_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawn_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawn_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawnAtRandomPlanet_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	void Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawnAtRandomPlanet_SetBit(void* Obj)
	{
		((AAstroGenerator*)Obj)->bCharacterSpawnAtRandomPlanet = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawnAtRandomPlanet = { "bCharacterSpawnAtRandomPlanet", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(AAstroGenerator), &Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawnAtRandomPlanet_SetBit, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawnAtRandomPlanet_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawnAtRandomPlanet_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_CharSpawnPlace_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_CharSpawnPlace_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_CharSpawnPlace = { "CharSpawnPlace", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, CharSpawnPlace), Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_CharSpawnPlace_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_CharSpawnPlace_MetaData)) }; // 4286103121
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStationOrbitHeight_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStationOrbitHeight_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStationOrbitHeight = { "HomeSpaceStationOrbitHeight", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, HomeSpaceStationOrbitHeight), Z_Construct_UEnum_APS_ALPHA_EOrbitHeight, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStationOrbitHeight_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStationOrbitHeight_MetaData)) }; // 2567945888
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_CharacterClass_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_CharacterClass = { "BP_CharacterClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_CharacterClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AControlledPawn_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_CharacterClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_CharacterClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceStation_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceStation = { "BP_HomeSpaceStation", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_HomeSpaceStation), Z_Construct_UClass_UClass, Z_Construct_UClass_ASpaceStation_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceStation_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceStation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceship_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceship = { "BP_HomeSpaceship", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_HomeSpaceship), Z_Construct_UClass_UClass, Z_Construct_UClass_ASpaceship_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceship_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceship_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceShipyard_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceShipyard = { "BP_HomeSpaceShipyard", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_HomeSpaceShipyard), Z_Construct_UClass_UClass, Z_Construct_UClass_ASpaceShipyard_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceShipyard_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceShipyard_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceHeadquarters_MetaData[] = {
		{ "Category", "Player Spawn" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceHeadquarters = { "BP_HomeSpaceHeadquarters", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_HomeSpaceHeadquarters), Z_Construct_UClass_UClass, Z_Construct_UClass_ASpaceHeadquarters_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceHeadquarters_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceHeadquarters_MetaData)) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_Inner = { "GeneratedStarSystems", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UClass_AStarSystem_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems = { "GeneratedStarSystems", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GeneratedStarSystems), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGenerator_MetaData[] = {
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGenerator = { "GalaxyGenerator", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, GalaxyGenerator), Z_Construct_UClass_UGalaxyGenerator_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGenerator_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGenerator_MetaData)) };
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
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_GalaxyClass_MetaData[] = {
		{ "Category", "AstroObject BP" },
		{ "ModuleRelativePath", "AstroGenerator.h" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_GalaxyClass = { "BP_GalaxyClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AAstroGenerator, BP_GalaxyClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AGalaxy_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_GalaxyClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_GalaxyClass_MetaData)) };
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
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedGalaxy,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarCluster,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedHomeStarSystem,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomePlanet,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceHeadquarters,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceship,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceShipyard,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateFullScaledWorld,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateHomeSystem,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_AstroGenerationLevel_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_AstroGenerationLevel,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemDeadZone,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPosition_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPosition,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bFullScaledHomeSystem,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bNeedOrbitRotation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystem,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemStarType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemStarType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeStar,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarStellarType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarStellarType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarSpectralClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeStarSpectralClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bRandomHomeSystemType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPlanetaryType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemPlanetaryType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemOrbitDistributionType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSystemOrbitDistributionType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxyCluster,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomGalaxy,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGlass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGlass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bGenerateRandomCluster,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterSize,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterPopulation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterComposition,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxySize,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarCount,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyStarDensity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawn,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_bCharacterSpawnAtRandomPlanet,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_CharSpawnPlace_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_CharSpawnPlace,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStationOrbitHeight_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_HomeSpaceStationOrbitHeight,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_CharacterClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceStation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceship,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceShipyard,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_HomeSpaceHeadquarters,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GeneratedStarSystems,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_GalaxyGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarClusterGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarSystemGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetarySystemGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_StarGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_PlanetGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_MoonGenerator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AAstroGenerator_Statics::NewProp_BP_GalaxyClass,
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
		{ Z_Construct_UClass_AAstroGenerator, AAstroGenerator::StaticClass, TEXT("AAstroGenerator"), &Z_Registration_Info_UClass_AAstroGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAstroGenerator), 1550507783U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_4078658436(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
