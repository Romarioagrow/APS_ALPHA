// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarSystemGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarSystemGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FStarSystemGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FStarSystemGenerationModel>() == std::is_polymorphic<FGenerationModel>(), "USTRUCT FStarSystemGenerationModel cannot be polymorphic unless super FGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_StarSystemGenerationModel;
class UScriptStruct* FStarSystemGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_StarSystemGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_StarSystemGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FStarSystemGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("StarSystemGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_StarSystemGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FStarSystemGenerationModel>()
{
	return FStarSystemGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AmountOfStars_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_AmountOfStars;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DistanceBetweenStars_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_DistanceBetweenStars;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "StarSystemGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FStarSystemGenerationModel>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_AmountOfStars_MetaData[] = {
		{ "Category", "Star System" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarSystemGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_AmountOfStars = { "AmountOfStars", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarSystemGenerationModel, AmountOfStars), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_AmountOfStars_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_AmountOfStars_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_DistanceBetweenStars_MetaData[] = {
		{ "Category", "Star System" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarSystemGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_DistanceBetweenStars = { "DistanceBetweenStars", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarSystemGenerationModel, DistanceBetweenStars), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_DistanceBetweenStars_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_DistanceBetweenStars_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_AmountOfStars,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewProp_DistanceBetweenStars,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FGenerationModel,
		&NewStructOps,
		"StarSystemGenerationModel",
		sizeof(FStarSystemGenerationModel),
		alignof(FStarSystemGenerationModel),
		Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FStarSystemGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_StarSystemGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_StarSystemGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_StarSystemGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FStarSystemGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FStarSystemGenerationModel_Statics::NewStructOps, TEXT("StarSystemGenerationModel"), &Z_Registration_Info_UScriptStruct_StarSystemGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FStarSystemGenerationModel), 2218997459U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemGenerationModel_h_1149994536(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
