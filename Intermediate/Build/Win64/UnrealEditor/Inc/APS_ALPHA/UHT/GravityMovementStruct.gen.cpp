// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityMovementStruct.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityMovementStruct() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGravityMovementStruct();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_GravityMovementStruct;
class UScriptStruct* FGravityMovementStruct::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_GravityMovementStruct.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_GravityMovementStruct.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FGravityMovementStruct, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("GravityMovementStruct"));
	}
	return Z_Registration_Info_UScriptStruct_GravityMovementStruct.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FGravityMovementStruct>()
{
	return FGravityMovementStruct::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FGravityMovementStruct_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MovementsForceSpeedZeroG_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_MovementsForceSpeedZeroG;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MovementsForceSpeedStation_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_MovementsForceSpeedStation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MovementsForceSpeedPlanet_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_MovementsForceSpeedPlanet;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MovementsForceSpeedShip_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_MovementsForceSpeedShip;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_JumpForceSpeedZeroG_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_JumpForceSpeedZeroG;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_JumpForceSpeedStation_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_JumpForceSpeedStation;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_JumpForceSpeedPlanet_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_JumpForceSpeedPlanet;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_JumpForceSpeedShip_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_JumpForceSpeedShip;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FGravityMovementStruct>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedZeroG_MetaData[] = {
		{ "Category", "Movements Force Speed - ZeroG" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedZeroG = { "MovementsForceSpeedZeroG", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, MovementsForceSpeedZeroG), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedZeroG_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedZeroG_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedStation_MetaData[] = {
		{ "Category", "Movements Force Speed - Station" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedStation = { "MovementsForceSpeedStation", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, MovementsForceSpeedStation), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedStation_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedStation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedPlanet_MetaData[] = {
		{ "Category", "Movements Force Speed - Planet" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedPlanet = { "MovementsForceSpeedPlanet", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, MovementsForceSpeedPlanet), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedPlanet_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedPlanet_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedShip_MetaData[] = {
		{ "Category", "Movements Force Speed - Ship" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedShip = { "MovementsForceSpeedShip", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, MovementsForceSpeedShip), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedShip_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedShip_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedZeroG_MetaData[] = {
		{ "Category", "Jump Force Speed - ZeroG" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedZeroG = { "JumpForceSpeedZeroG", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, JumpForceSpeedZeroG), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedZeroG_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedZeroG_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedStation_MetaData[] = {
		{ "Category", "Jump Force Speed - Station" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedStation = { "JumpForceSpeedStation", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, JumpForceSpeedStation), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedStation_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedStation_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedPlanet_MetaData[] = {
		{ "Category", "Jump Force Speed - Planet" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedPlanet = { "JumpForceSpeedPlanet", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, JumpForceSpeedPlanet), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedPlanet_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedPlanet_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedShip_MetaData[] = {
		{ "Category", "Jump Force Speed - Ship" },
		{ "ModuleRelativePath", "GravityMovementStruct.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedShip = { "JumpForceSpeedShip", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGravityMovementStruct, JumpForceSpeedShip), METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedShip_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedShip_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedZeroG,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedStation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedPlanet,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_MovementsForceSpeedShip,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedZeroG,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedStation,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedPlanet,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewProp_JumpForceSpeedShip,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"GravityMovementStruct",
		sizeof(FGravityMovementStruct),
		alignof(FGravityMovementStruct),
		Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FGravityMovementStruct()
	{
		if (!Z_Registration_Info_UScriptStruct_GravityMovementStruct.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_GravityMovementStruct.InnerSingleton, Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_GravityMovementStruct.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityMovementStruct_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityMovementStruct_h_Statics::ScriptStructInfo[] = {
		{ FGravityMovementStruct::StaticStruct, Z_Construct_UScriptStruct_FGravityMovementStruct_Statics::NewStructOps, TEXT("GravityMovementStruct"), &Z_Registration_Info_UScriptStruct_GravityMovementStruct, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FGravityMovementStruct), 1162324698U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityMovementStruct_h_1036830151(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityMovementStruct_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityMovementStruct_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
