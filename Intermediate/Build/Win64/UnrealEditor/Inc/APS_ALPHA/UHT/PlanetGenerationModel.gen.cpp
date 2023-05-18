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
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

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
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AmountOfMoons_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_AmountOfMoons;
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
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons_MetaData[] = {
		{ "Category", "Planet" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons = { "AmountOfMoons", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetGenerationModel, AmountOfMoons), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewProp_AmountOfMoons,
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
		{ FPlanetGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FPlanetGenerationModel_Statics::NewStructOps, TEXT("PlanetGenerationModel"), &Z_Registration_Info_UScriptStruct_PlanetGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetGenerationModel), 2220043833U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_2892645585(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
