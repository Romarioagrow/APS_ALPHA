// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/OrbitalBodyGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeOrbitalBodyGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FCelestialBodyModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FOrbitalBodyModel>() == std::is_polymorphic<FCelestialBodyModel>(), "USTRUCT FOrbitalBodyModel cannot be polymorphic unless super FCelestialBodyModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_OrbitalBodyModel;
class UScriptStruct* FOrbitalBodyModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_OrbitalBodyModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_OrbitalBodyModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FOrbitalBodyModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("OrbitalBodyModel"));
	}
	return Z_Registration_Info_UScriptStruct_OrbitalBodyModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FOrbitalBodyModel>()
{
	return FOrbitalBodyModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "OrbitalBodyGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FOrbitalBodyModel>();
	}
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FCelestialBodyModel,
		&NewStructOps,
		"OrbitalBodyModel",
		sizeof(FOrbitalBodyModel),
		alignof(FOrbitalBodyModel),
		nullptr,
		0,
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyModel()
	{
		if (!Z_Registration_Info_UScriptStruct_OrbitalBodyModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_OrbitalBodyModel.InnerSingleton, Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_OrbitalBodyModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FOrbitalBodyModel::StaticStruct, Z_Construct_UScriptStruct_FOrbitalBodyModel_Statics::NewStructOps, TEXT("OrbitalBodyModel"), &Z_Registration_Info_UScriptStruct_OrbitalBodyModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FOrbitalBodyModel), 929212765U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_1278727343(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
