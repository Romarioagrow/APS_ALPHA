// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/MoonGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMoonGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FMoonGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FMoonGenerationModel>() == std::is_polymorphic<FOrbitalBodyGenerationModel>(), "USTRUCT FMoonGenerationModel cannot be polymorphic unless super FOrbitalBodyGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_MoonGenerationModel;
class UScriptStruct* FMoonGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_MoonGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_MoonGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FMoonGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("MoonGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_MoonGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FMoonGenerationModel>()
{
	return FMoonGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FMoonGenerationModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "MoonGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FMoonGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FMoonGenerationModel>();
	}
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FMoonGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel,
		&NewStructOps,
		"MoonGenerationModel",
		sizeof(FMoonGenerationModel),
		alignof(FMoonGenerationModel),
		nullptr,
		0,
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FMoonGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_MoonGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_MoonGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FMoonGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_MoonGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FMoonGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FMoonGenerationModel_Statics::NewStructOps, TEXT("MoonGenerationModel"), &Z_Registration_Info_UScriptStruct_MoonGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FMoonGenerationModel), 1974069068U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_1575775656(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
