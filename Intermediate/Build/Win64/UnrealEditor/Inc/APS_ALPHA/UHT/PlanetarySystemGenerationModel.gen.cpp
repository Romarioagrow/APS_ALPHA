// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetarySystemGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetarySystemGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetarySystemModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetData();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FZoneRadius();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_ZoneRadius;
class UScriptStruct* FZoneRadius::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_ZoneRadius.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_ZoneRadius.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FZoneRadius, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("ZoneRadius"));
	}
	return Z_Registration_Info_UScriptStruct_ZoneRadius.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FZoneRadius>()
{
	return FZoneRadius::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FZoneRadius_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_InnerRadius_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_InnerRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OuterRadius_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_OuterRadius;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FZoneRadius_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FZoneRadius_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FZoneRadius>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_InnerRadius_MetaData[] = {
		{ "Category", "Zone" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_InnerRadius = { "InnerRadius", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FZoneRadius, InnerRadius), METADATA_PARAMS(Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_InnerRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_InnerRadius_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_OuterRadius_MetaData[] = {
		{ "Category", "Zone" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_OuterRadius = { "OuterRadius", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FZoneRadius, OuterRadius), METADATA_PARAMS(Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_OuterRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_OuterRadius_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FZoneRadius_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_InnerRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FZoneRadius_Statics::NewProp_OuterRadius,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FZoneRadius_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"ZoneRadius",
		sizeof(FZoneRadius),
		alignof(FZoneRadius),
		Z_Construct_UScriptStruct_FZoneRadius_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FZoneRadius_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FZoneRadius_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FZoneRadius_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FZoneRadius()
	{
		if (!Z_Registration_Info_UScriptStruct_ZoneRadius.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_ZoneRadius.InnerSingleton, Z_Construct_UScriptStruct_FZoneRadius_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_ZoneRadius.InnerSingleton;
	}
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlanetData;
class UScriptStruct* FPlanetData::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlanetData.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlanetData.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlanetData, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("PlanetData"));
	}
	return Z_Registration_Info_UScriptStruct_PlanetData.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FPlanetData>()
{
	return FPlanetData::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlanetData_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetOrder_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_PlanetOrder;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OrbitRadius_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_OrbitRadius;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetData_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPlanetData_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlanetData>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_PlanetOrder_MetaData[] = {
		{ "Category", "Planet Mode" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_PlanetOrder = { "PlanetOrder", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetData, PlanetOrder), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_PlanetOrder_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_PlanetOrder_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_OrbitRadius_MetaData[] = {
		{ "Category", "Planet Mode" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_OrbitRadius = { "OrbitRadius", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetData, OrbitRadius), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_OrbitRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_OrbitRadius_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetData_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_PlanetOrder,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetData_Statics::NewProp_OrbitRadius,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlanetData_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"PlanetData",
		sizeof(FPlanetData),
		alignof(FPlanetData),
		Z_Construct_UScriptStruct_FPlanetData_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetData_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetData_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetData_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FPlanetData()
	{
		if (!Z_Registration_Info_UScriptStruct_PlanetData.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlanetData.InnerSingleton, Z_Construct_UScriptStruct_FPlanetData_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlanetData.InnerSingleton;
	}

static_assert(std::is_polymorphic<FPlanetarySystemModel>() == std::is_polymorphic<FGenerationModel>(), "USTRUCT FPlanetarySystemModel cannot be polymorphic unless super FGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlanetarySystemModel;
class UScriptStruct* FPlanetarySystemModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlanetarySystemModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlanetarySystemModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlanetarySystemModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("PlanetarySystemModel"));
	}
	return Z_Registration_Info_UScriptStruct_PlanetarySystemModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FPlanetarySystemModel>()
{
	return FPlanetarySystemModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AmountOfPlanets_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_AmountOfPlanets;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DistanceBetweenPlanets_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_DistanceBetweenPlanets;
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
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DeadZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_DeadZoneRadius;
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
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GasGiantsZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_GasGiantsZoneRadius;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlanetarySystemModel>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AmountOfPlanets_MetaData[] = {
		{ "Category", "Planetary System" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AmountOfPlanets = { "AmountOfPlanets", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, AmountOfPlanets), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AmountOfPlanets_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AmountOfPlanets_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DistanceBetweenPlanets_MetaData[] = {
		{ "Category", "Planetary System" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DistanceBetweenPlanets = { "DistanceBetweenPlanets", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, DistanceBetweenPlanets), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DistanceBetweenPlanets_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DistanceBetweenPlanets_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetarySystemType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetarySystemType_MetaData[] = {
		{ "Category", "PlanetarySystemModel" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetarySystemType = { "PlanetarySystemType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, PlanetarySystemType), Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetarySystemType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetarySystemType_MetaData)) }; // 3370777339
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OrbitDistributionType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OrbitDistributionType_MetaData[] = {
		{ "Category", "PlanetarySystemModel" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OrbitDistributionType = { "OrbitDistributionType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, OrbitDistributionType), Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OrbitDistributionType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OrbitDistributionType_MetaData)) }; // 2085035236
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_FullSpectralName_MetaData[] = {
		{ "Category", "PlanetarySystemModel" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_FullSpectralName = { "FullSpectralName", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, FullSpectralName), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_FullSpectralName_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_FullSpectralName_MetaData)) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetsList_Inner = { "PlanetsList", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UScriptStruct_FPlanetData, METADATA_PARAMS(nullptr, 0) }; // 2289700741
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetsList_MetaData[] = {
		{ "Category", "PlanetarySystemModel" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetsList = { "PlanetsList", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, PlanetsList), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetsList_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetsList_MetaData)) }; // 2289700741
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DeadZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "Comment", "/// TODO: To one struct\n" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
		{ "ToolTip", "TODO: To one struct" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DeadZoneRadius = { "DeadZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, DeadZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DeadZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DeadZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HabitableZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HabitableZoneRadius = { "HabitableZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, HabitableZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HabitableZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HabitableZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_ColdZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_ColdZoneRadius = { "ColdZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, ColdZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_ColdZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_ColdZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_IceZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_IceZoneRadius = { "IceZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, IceZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_IceZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_IceZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_WarmZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_WarmZoneRadius = { "WarmZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, WarmZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_WarmZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_WarmZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HotZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HotZoneRadius = { "HotZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, HotZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HotZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HotZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_InnerPlanetZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_InnerPlanetZoneRadius = { "InnerPlanetZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, InnerPlanetZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_InnerPlanetZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_InnerPlanetZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OuterPlanetZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OuterPlanetZoneRadius = { "OuterPlanetZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, OuterPlanetZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OuterPlanetZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OuterPlanetZoneRadius_MetaData)) }; // 3993418724
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius_ValueProp = { "AsteroidBeltZoneRadius", nullptr, (EPropertyFlags)0x0000000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 1, Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(nullptr, 0) }; // 3993418724
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius_Key_KeyProp = { "AsteroidBeltZoneRadius_Key", nullptr, (EPropertyFlags)0x0000000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius = { "AsteroidBeltZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, AsteroidBeltZoneRadius), EMapPropertyFlags::None, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_KuiperBeltZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_KuiperBeltZoneRadius = { "KuiperBeltZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, KuiperBeltZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_KuiperBeltZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_KuiperBeltZoneRadius_MetaData)) }; // 3993418724
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_GasGiantsZoneRadius_MetaData[] = {
		{ "Category", "Zones" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_GasGiantsZoneRadius = { "GasGiantsZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemModel, GasGiantsZoneRadius), Z_Construct_UScriptStruct_FZoneRadius, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_GasGiantsZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_GasGiantsZoneRadius_MetaData)) }; // 3993418724
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AmountOfPlanets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DistanceBetweenPlanets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetarySystemType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetarySystemType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OrbitDistributionType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OrbitDistributionType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_FullSpectralName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetsList_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_PlanetsList,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_DeadZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HabitableZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_ColdZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_IceZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_WarmZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_HotZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_InnerPlanetZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_OuterPlanetZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_AsteroidBeltZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_KuiperBeltZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewProp_GasGiantsZoneRadius,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FGenerationModel,
		&NewStructOps,
		"PlanetarySystemModel",
		sizeof(FPlanetarySystemModel),
		alignof(FPlanetarySystemModel),
		Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FPlanetarySystemModel()
	{
		if (!Z_Registration_Info_UScriptStruct_PlanetarySystemModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlanetarySystemModel.InnerSingleton, Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlanetarySystemModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FZoneRadius::StaticStruct, Z_Construct_UScriptStruct_FZoneRadius_Statics::NewStructOps, TEXT("ZoneRadius"), &Z_Registration_Info_UScriptStruct_ZoneRadius, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FZoneRadius), 3993418724U) },
		{ FPlanetData::StaticStruct, Z_Construct_UScriptStruct_FPlanetData_Statics::NewStructOps, TEXT("PlanetData"), &Z_Registration_Info_UScriptStruct_PlanetData, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetData), 2289700741U) },
		{ FPlanetarySystemModel::StaticStruct, Z_Construct_UScriptStruct_FPlanetarySystemModel_Statics::NewStructOps, TEXT("PlanetarySystemModel"), &Z_Registration_Info_UScriptStruct_PlanetarySystemModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetarySystemModel), 4197131761U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_678318334(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
