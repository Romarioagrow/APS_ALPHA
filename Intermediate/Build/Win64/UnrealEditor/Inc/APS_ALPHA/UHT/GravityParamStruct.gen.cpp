// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityParamStruct.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityParamStruct() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGravityParamStruct();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_GravityParamStruct;
class UScriptStruct* FGravityParamStruct::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_GravityParamStruct.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_GravityParamStruct.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FGravityParamStruct, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("GravityParamStruct"));
	}
	return Z_Registration_Info_UScriptStruct_GravityParamStruct.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FGravityParamStruct>()
{
	return FGravityParamStruct::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FGravityParamStruct_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LinearDampingZeroG_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_LinearDampingZeroG;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AngularDampingZeroG_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_AngularDampingZeroG;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LinearDampingStation_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_LinearDampingStation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AngularDampingStation_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_AngularDampingStation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LinearDampingPlanet_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_LinearDampingPlanet;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AngularDampingPlanet_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_AngularDampingPlanet;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LinearDampingShip_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_LinearDampingShip;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AngularDampingShip_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_AngularDampingShip;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LinearDampingLowG_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_LinearDampingLowG;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AngularDampingLowG_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_AngularDampingLowG;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FGravityParamStruct>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingZeroG_MetaData[] = {
		{ "Category", "Linear Damping - ZeroG" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingZeroG = { "LinearDampingZeroG", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, LinearDampingZeroG), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingZeroG_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingZeroG_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingZeroG_MetaData[] = {
		{ "Category", "Angular Damping - ZeroG" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingZeroG = { "AngularDampingZeroG", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, AngularDampingZeroG), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingZeroG_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingZeroG_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingStation_MetaData[] = {
		{ "Category", "Linear Damping - Station" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingStation = { "LinearDampingStation", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, LinearDampingStation), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingStation_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingStation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingStation_MetaData[] = {
		{ "Category", "Angular Damping - Station" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingStation = { "AngularDampingStation", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, AngularDampingStation), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingStation_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingStation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingPlanet_MetaData[] = {
		{ "Category", "Linear Damping - Planet" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingPlanet = { "LinearDampingPlanet", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, LinearDampingPlanet), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingPlanet_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingPlanet_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingPlanet_MetaData[] = {
		{ "Category", "Angular Damping - Planet" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingPlanet = { "AngularDampingPlanet", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, AngularDampingPlanet), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingPlanet_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingPlanet_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingShip_MetaData[] = {
		{ "Category", "Linear Damping - Ship" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingShip = { "LinearDampingShip", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, LinearDampingShip), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingShip_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingShip_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingShip_MetaData[] = {
		{ "Category", "Angular Damping - Ship" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingShip = { "AngularDampingShip", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, AngularDampingShip), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingShip_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingShip_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingLowG_MetaData[] = {
		{ "Category", "Linear Damping - LowG" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingLowG = { "LinearDampingLowG", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, LinearDampingLowG), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingLowG_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingLowG_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingLowG_MetaData[] = {
		{ "Category", "Angular Damping - LowG" },
		{ "ModuleRelativePath", "GravityParamStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingLowG = { "AngularDampingLowG", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityParamStruct, AngularDampingLowG), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingLowG_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingLowG_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FGravityParamStruct_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingZeroG,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingZeroG,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingStation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingStation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingPlanet,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingPlanet,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingShip,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingShip,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_LinearDampingLowG,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewProp_AngularDampingLowG,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FGravityParamStruct_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"GravityParamStruct",
		sizeof(FGravityParamStruct),
		alignof(FGravityParamStruct),
		Z_Construct_UScriptStruct_FGravityParamStruct_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityParamStruct_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FGravityParamStruct()
	{
		if (!Z_Registration_Info_UScriptStruct_GravityParamStruct.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_GravityParamStruct.InnerSingleton, Z_Construct_UScriptStruct_FGravityParamStruct_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_GravityParamStruct.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityParamStruct_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityParamStruct_h_Statics::ScriptStructInfo[] = {
		{ FGravityParamStruct::StaticStruct, Z_Construct_UScriptStruct_FGravityParamStruct_Statics::NewStructOps, TEXT("GravityParamStruct"), &Z_Registration_Info_UScriptStruct_GravityParamStruct, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FGravityParamStruct), 3112262396U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityParamStruct_h_1027397107(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityParamStruct_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityParamStruct_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
