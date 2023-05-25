// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetarySystem.h"
#include "APS_ALPHA/PlanetarySystemGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetarySystem() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetData();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FZoneRadius();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void APlanetarySystem::StaticRegisterNativesAPlanetarySystem()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(APlanetarySystem);
	UClass* Z_Construct_UClass_APlanetarySystem_NoRegister()
	{
		return APlanetarySystem::StaticClass();
	}
	struct Z_Construct_UClass_APlanetarySystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Star_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Star;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AmountOfPlanets_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_AmountOfPlanets;
		static const UECodeGen_Private::FBytePropertyParams NewProp_PlanetarySystemType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetarySystemType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_PlanetarySystemType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_OrbitDistributionType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OrbitDistributionType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_OrbitDistributionType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_FullSpectralName_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_FullSpectralName;
		static const UECodeGen_Private::FStructPropertyParams NewProp_PlanetsList_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetsList_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_PlanetsList;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HabitableZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_HabitableZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ColdZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_ColdZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_IceZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_IceZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_WarmZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_WarmZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HotZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_HotZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_InnerPlanetZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_InnerPlanetZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OuterPlanetZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_OuterPlanetZoneRadius;
		static const UECodeGen_Private::FStructPropertyParams NewProp_AsteroidBeltZoneRadius_ValueProp;
		static const UECodeGen_Private::FIntPropertyParams NewProp_AsteroidBeltZoneRadius_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AsteroidBeltZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_AsteroidBeltZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_KuiperBeltZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_KuiperBeltZoneRadius;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_APlanetarySystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialSystem,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "PlanetarySystem.h" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star_MetaData[] = {
		{ "Category", "PlanetarySystem" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star = { "Star", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, Star), Z_Construct_UClass_AStar_NoRegister, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AmountOfPlanets_MetaData[] = {
		{ "Category", "PlanetarySystem" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AmountOfPlanets = { "AmountOfPlanets", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, AmountOfPlanets), METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AmountOfPlanets_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AmountOfPlanets_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetarySystemType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetarySystemType_MetaData[] = {
		{ "Category", "PlanetarySystem" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetarySystemType = { "PlanetarySystemType", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, PlanetarySystemType), Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetarySystemType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetarySystemType_MetaData)) }; // 3370777339
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OrbitDistributionType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OrbitDistributionType_MetaData[] = {
		{ "Category", "PlanetarySystem" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OrbitDistributionType = { "OrbitDistributionType", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, OrbitDistributionType), Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OrbitDistributionType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OrbitDistributionType_MetaData)) }; // 2085035236
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_FullSpectralName_MetaData[] = {
		{ "Category", "PlanetarySystem" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_FullSpectralName = { "FullSpectralName", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, FullSpectralName), METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_FullSpectralName_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_FullSpectralName_MetaData)) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetsList_Inner = { "PlanetsList", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UScriptStruct_FPlanetData, METADATA_PARAMS(nullptr, 0) }; // 2294851697
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetsList_MetaData[] = {
		{ "Category", "PlanetarySystem" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetsList = { "PlanetsList", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, PlanetsList), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetsList_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetsList_MetaData)) }; // 2294851697
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HabitableZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "Comment", "/// ZONES \n/// TO SINGLE STRUCT\n" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
		{ "ToolTip", "ZONES\nTO SINGLE STRUCT" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HabitableZoneRadius = { "HabitableZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, HabitableZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HabitableZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HabitableZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_ColdZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_ColdZoneRadius = { "ColdZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, ColdZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_ColdZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_ColdZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_IceZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_IceZoneRadius = { "IceZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, IceZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_IceZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_IceZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_WarmZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_WarmZoneRadius = { "WarmZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, WarmZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_WarmZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_WarmZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HotZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HotZoneRadius = { "HotZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, HotZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HotZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HotZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_InnerPlanetZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_InnerPlanetZoneRadius = { "InnerPlanetZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, InnerPlanetZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_InnerPlanetZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_InnerPlanetZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OuterPlanetZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OuterPlanetZoneRadius = { "OuterPlanetZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, OuterPlanetZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OuterPlanetZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OuterPlanetZoneRadius_MetaData)) }; // 3993418724
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius_ValueProp = { "AsteroidBeltZoneRadius", nullptr, (EPropertyFlags)0x0000000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 1, Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(nullptr, 0) }; // 3993418724
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius_Key_KeyProp = { "AsteroidBeltZoneRadius_Key", nullptr, (EPropertyFlags)0x0000000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius = { "AsteroidBeltZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, AsteroidBeltZoneRadius), EMapPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_KuiperBeltZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_KuiperBeltZoneRadius = { "KuiperBeltZoneRadius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, KuiperBeltZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_KuiperBeltZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_KuiperBeltZoneRadius_MetaData)) }; // 3993418724
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_APlanetarySystem_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AmountOfPlanets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetarySystemType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetarySystemType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OrbitDistributionType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OrbitDistributionType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_FullSpectralName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetsList_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_PlanetsList,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HabitableZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_ColdZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_IceZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_WarmZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_HotZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_InnerPlanetZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_OuterPlanetZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_AsteroidBeltZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_KuiperBeltZoneRadius,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_APlanetarySystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<APlanetarySystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_APlanetarySystem_Statics::ClassParams = {
		&APlanetarySystem::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_APlanetarySystem_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_APlanetarySystem()
	{
		if (!Z_Registration_Info_UClass_APlanetarySystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_APlanetarySystem.OuterSingleton, Z_Construct_UClass_APlanetarySystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_APlanetarySystem.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<APlanetarySystem>()
	{
		return APlanetarySystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(APlanetarySystem);
	APlanetarySystem::~APlanetarySystem() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_APlanetarySystem, APlanetarySystem::StaticClass, TEXT("APlanetarySystem"), &Z_Registration_Info_UClass_APlanetarySystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(APlanetarySystem), 1301392695U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_3525957960(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
