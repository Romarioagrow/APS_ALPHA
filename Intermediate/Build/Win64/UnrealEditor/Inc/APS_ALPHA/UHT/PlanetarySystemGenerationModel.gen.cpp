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
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FPlanetarySystemGenerationModel>() == std::is_polymorphic<FGenerationModel>(), "USTRUCT FPlanetarySystemGenerationModel cannot be polymorphic unless super FGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel;
class UScriptStruct* FPlanetarySystemGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("PlanetarySystemGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FPlanetarySystemGenerationModel>()
{
	return FPlanetarySystemGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics
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
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlanetarySystemGenerationModel>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets_MetaData[] = {
		{ "Category", "Planetary System" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets = { "AmountOfPlanets", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemGenerationModel, AmountOfPlanets), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets_MetaData[] = {
		{ "Category", "Planetary System" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets = { "DistanceBetweenPlanets", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemGenerationModel, DistanceBetweenPlanets), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FGenerationModel,
		&NewStructOps,
		"PlanetarySystemGenerationModel",
		sizeof(FPlanetarySystemGenerationModel),
		alignof(FPlanetarySystemGenerationModel),
		Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FPlanetarySystemGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewStructOps, TEXT("PlanetarySystemGenerationModel"), &Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetarySystemGenerationModel), 2213929434U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_2760705638(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
