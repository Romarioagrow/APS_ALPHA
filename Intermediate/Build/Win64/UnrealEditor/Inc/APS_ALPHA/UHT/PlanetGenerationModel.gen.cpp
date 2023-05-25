// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetGenerationModel.h"
#include "APS_ALPHA/MoonGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FMoonData();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FMoonGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_MoonData;
class UScriptStruct* FMoonData::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_MoonData.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_MoonData.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FMoonData, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("MoonData"));
	}
	return Z_Registration_Info_UScriptStruct_MoonData.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FMoonData>()
{
	return FMoonData::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FMoonData_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoonOrder_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_MoonOrder;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OrbitRadius_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_OrbitRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoonModel_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_MoonModel;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonData_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FMoonData_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FMoonData>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonOrder_MetaData[] = {
		{ "Category", "Planet Mode" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonOrder = { "MoonOrder", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FMoonData, MoonOrder), METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonOrder_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonOrder_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_OrbitRadius_MetaData[] = {
		{ "Category", "Planet Mode" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_OrbitRadius = { "OrbitRadius", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FMoonData, OrbitRadius), METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_OrbitRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_OrbitRadius_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonModel_MetaData[] = {
		{ "Category", "Planet Model" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonModel = { "MoonModel", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FMoonData, MoonModel), Z_Construct_UScriptStruct_FMoonGenerationModel, METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonModel_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonModel_MetaData)) }; // 1141667045
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FMoonData_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonOrder,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_OrbitRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonModel,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FMoonData_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"MoonData",
		sizeof(FMoonData),
		alignof(FMoonData),
		Z_Construct_UScriptStruct_FMoonData_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonData_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonData_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonData_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FMoonData()
	{
		if (!Z_Registration_Info_UScriptStruct_MoonData.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_MoonData.InnerSingleton, Z_Construct_UScriptStruct_FMoonData_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_MoonData.InnerSingleton;
	}

static_assert(std::is_polymorphic<FPlanetGenerationModel>() == std::is_polymorphic<FOrbitalBodyGenerationModel>(), "USTRUCT FPlanetGenerationModel cannot be polymorphic unless super FOrbitalBodyGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlanetGenerationModel;
class UScriptStruct* FPlanetGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlanetGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlanetGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlanetGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("PlanetGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_PlanetGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FPlanetGenerationModel>()
{
	return FPlanetGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FBytePropertyParams NewProp_PlanetType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_PlanetType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AmountOfMoons_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_AmountOfMoons;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Temperature_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_Temperature;
		static const UECodeGen_Private::FBytePropertyParams NewProp_PlanetZone_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetZone_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_PlanetZone;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetDensity_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_PlanetDensity;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetGravityStrength_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_PlanetGravityStrength;
		static const UECodeGen_Private::FStructPropertyParams NewProp_MoonsList_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoonsList_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_MoonsList;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlanetGenerationModel>();
	}
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetType_MetaData[] = {
		{ "Category", "Planet" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd (\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd.\xef\xbf\xbd.)\n" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd (\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd.\xef\xbf\xbd.)" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetType = { "PlanetType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, PlanetType), Z_Construct_UEnum_APS_ALPHA_EPlanetType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetType_MetaData)) }; // 2129899352
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons_MetaData[] = {
		{ "Category", "Planet" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons = { "AmountOfMoons", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, AmountOfMoons), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_Temperature_MetaData[] = {
		{ "Category", "Planet" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_Temperature = { "Temperature", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, Temperature), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_Temperature_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_Temperature_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetZone_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetZone_MetaData[] = {
		{ "Category", "Planet" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetZone = { "PlanetZone", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, PlanetZone), Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetZone_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetZone_MetaData)) }; // 266526262
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetDensity_MetaData[] = {
		{ "Category", "Planet" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetDensity = { "PlanetDensity", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, PlanetDensity), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetDensity_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetDensity_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetGravityStrength_MetaData[] = {
		{ "Category", "Planet" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetGravityStrength = { "PlanetGravityStrength", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, PlanetGravityStrength), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetGravityStrength_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetGravityStrength_MetaData)) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_MoonsList_Inner = { "MoonsList", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UScriptStruct_FMoonData, METADATA_PARAMS(nullptr, 0) }; // 2878841582
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_MoonsList_MetaData[] = {
		{ "Category", "PlanetGenerationModel" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_MoonsList = { "MoonsList", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, MoonsList), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_MoonsList_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_MoonsList_MetaData)) }; // 2878841582
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_Temperature,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetZone_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetZone,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetDensity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_PlanetGravityStrength,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_MoonsList_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_MoonsList,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel,
		&NewStructOps,
		"PlanetGenerationModel",
		sizeof(FPlanetGenerationModel),
		alignof(FPlanetGenerationModel),
		Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FPlanetGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_PlanetGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlanetGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlanetGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FMoonData::StaticStruct, Z_Construct_UScriptStruct_FMoonData_Statics::NewStructOps, TEXT("MoonData"), &Z_Registration_Info_UScriptStruct_MoonData, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FMoonData), 2878841582U) },
		{ FPlanetGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewStructOps, TEXT("PlanetGenerationModel"), &Z_Registration_Info_UScriptStruct_PlanetGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetGenerationModel), 2668456687U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_1242569252(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
