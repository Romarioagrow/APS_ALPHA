// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FMoonData();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetModel();
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
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FMoonData_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_MoonOrder,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonData_Statics::NewProp_OrbitRadius,
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

static_assert(std::is_polymorphic<FPlanetModel>() == std::is_polymorphic<FOrbitalBodyModel>(), "USTRUCT FPlanetModel cannot be polymorphic unless super FOrbitalBodyModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlanetModel;
class UScriptStruct* FPlanetModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlanetModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlanetModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlanetModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("PlanetModel"));
	}
	return Z_Registration_Info_UScriptStruct_PlanetModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FPlanetModel>()
{
	return FPlanetModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlanetModel_Statics
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
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPlanetModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlanetModel>();
	}
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetType_MetaData[] = {
		{ "Category", "Planet" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \n" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetType = { "PlanetType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetModel, PlanetType), Z_Construct_UEnum_APS_ALPHA_EPlanetType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetType_MetaData)) }; // 2129899352
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_AmountOfMoons_MetaData[] = {
		{ "Category", "Planet" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_AmountOfMoons = { "AmountOfMoons", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetModel, AmountOfMoons), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_AmountOfMoons_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_AmountOfMoons_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_Temperature_MetaData[] = {
		{ "Category", "Planet" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_Temperature = { "Temperature", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetModel, Temperature), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_Temperature_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_Temperature_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetZone_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetZone_MetaData[] = {
		{ "Category", "Planet" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetZone = { "PlanetZone", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetModel, PlanetZone), Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetZone_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetZone_MetaData)) }; // 266526262
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetDensity_MetaData[] = {
		{ "Category", "Planet" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetDensity = { "PlanetDensity", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetModel, PlanetDensity), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetDensity_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetDensity_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetGravityStrength_MetaData[] = {
		{ "Category", "Planet" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetGravityStrength = { "PlanetGravityStrength", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetModel, PlanetGravityStrength), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetGravityStrength_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetGravityStrength_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_AmountOfMoons,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_Temperature,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetZone_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetZone,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetDensity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetModel_Statics::NewProp_PlanetGravityStrength,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlanetModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FOrbitalBodyModel,
		&NewStructOps,
		"PlanetModel",
		sizeof(FPlanetModel),
		alignof(FPlanetModel),
		Z_Construct_UScriptStruct_FPlanetModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FPlanetModel()
	{
		if (!Z_Registration_Info_UScriptStruct_PlanetModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlanetModel.InnerSingleton, Z_Construct_UScriptStruct_FPlanetModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlanetModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FMoonData::StaticStruct, Z_Construct_UScriptStruct_FMoonData_Statics::NewStructOps, TEXT("MoonData"), &Z_Registration_Info_UScriptStruct_MoonData, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FMoonData), 1115629065U) },
		{ FPlanetModel::StaticStruct, Z_Construct_UScriptStruct_FPlanetModel_Statics::NewStructOps, TEXT("PlanetModel"), &Z_Registration_Info_UScriptStruct_PlanetModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetModel), 2106184861U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_3844956348(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
