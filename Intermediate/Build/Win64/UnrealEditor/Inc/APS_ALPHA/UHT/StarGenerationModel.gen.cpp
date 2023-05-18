// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FCelestialGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FStarGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FStarGenerationModel>() == std::is_polymorphic<FCelestialGenerationModel>(), "USTRUCT FStarGenerationModel cannot be polymorphic unless super FCelestialGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_StarGenerationModel;
class UScriptStruct* FStarGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_StarGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_StarGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FStarGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("StarGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_StarGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FStarGenerationModel>()
{
	return FStarGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FStarGenerationModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FStarGenerationModel>();
	}
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FCelestialGenerationModel,
		&NewStructOps,
		"StarGenerationModel",
		sizeof(FStarGenerationModel),
		alignof(FStarGenerationModel),
		nullptr,
		0,
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FStarGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_StarGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_StarGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FStarGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_StarGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FStarGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewStructOps, TEXT("StarGenerationModel"), &Z_Registration_Info_UScriptStruct_StarGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FStarGenerationModel), 4236531549U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_2922902196(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
