// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/HomeStarSystemGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeHomeStarSystemGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FStarSystemGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FHomeStarSystemGenerationModel>() == std::is_polymorphic<FStarSystemGenerationModel>(), "USTRUCT FHomeStarSystemGenerationModel cannot be polymorphic unless super FStarSystemGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel;
class UScriptStruct* FHomeStarSystemGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("HomeStarSystemGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FHomeStarSystemGenerationModel>()
{
	return FHomeStarSystemGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "HomeStarSystemGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FHomeStarSystemGenerationModel>();
	}
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FStarSystemGenerationModel,
		&NewStructOps,
		"HomeStarSystemGenerationModel",
		sizeof(FHomeStarSystemGenerationModel),
		alignof(FHomeStarSystemGenerationModel),
		nullptr,
		0,
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_HomeStarSystemGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_HomeStarSystemGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FHomeStarSystemGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FHomeStarSystemGenerationModel_Statics::NewStructOps, TEXT("HomeStarSystemGenerationModel"), &Z_Registration_Info_UScriptStruct_HomeStarSystemGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FHomeStarSystemGenerationModel), 3020112730U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_HomeStarSystemGenerationModel_h_2389714654(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_HomeStarSystemGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_HomeStarSystemGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
