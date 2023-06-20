// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/CelestialGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCelestialGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FCelestialBodyModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FCelestialBodyModel>() == std::is_polymorphic<FGenerationModel>(), "USTRUCT FCelestialBodyModel cannot be polymorphic unless super FGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_CelestialBodyModel;
class UScriptStruct* FCelestialBodyModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_CelestialBodyModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_CelestialBodyModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FCelestialBodyModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("CelestialBodyModel"));
	}
	return Z_Registration_Info_UScriptStruct_CelestialBodyModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FCelestialBodyModel>()
{
	return FCelestialBodyModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FCelestialBodyModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Mass_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Mass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Radius_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Radius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RadiusKM_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_RadiusKM;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "CelestialGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FCelestialBodyModel>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Mass_MetaData[] = {
		{ "Category", "CelestialGeneration" },
		{ "ModuleRelativePath", "CelestialGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Mass = { "Mass", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FCelestialBodyModel, Mass), METADATA_PARAMS(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Mass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Mass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Radius_MetaData[] = {
		{ "Category", "CelestialGeneration" },
		{ "ModuleRelativePath", "CelestialGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Radius = { "Radius", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FCelestialBodyModel, Radius), METADATA_PARAMS(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Radius_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Radius_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_RadiusKM_MetaData[] = {
		{ "Category", "CelestialGeneration" },
		{ "ModuleRelativePath", "CelestialGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_RadiusKM = { "RadiusKM", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FCelestialBodyModel, RadiusKM), METADATA_PARAMS(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_RadiusKM_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_RadiusKM_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Mass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_Radius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewProp_RadiusKM,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FGenerationModel,
		&NewStructOps,
		"CelestialBodyModel",
		sizeof(FCelestialBodyModel),
		alignof(FCelestialBodyModel),
		Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FCelestialBodyModel()
	{
		if (!Z_Registration_Info_UScriptStruct_CelestialBodyModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_CelestialBodyModel.InnerSingleton, Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_CelestialBodyModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FCelestialBodyModel::StaticStruct, Z_Construct_UScriptStruct_FCelestialBodyModel_Statics::NewStructOps, TEXT("CelestialBodyModel"), &Z_Registration_Info_UScriptStruct_CelestialBodyModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FCelestialBodyModel), 982923349U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialGenerationModel_h_786860741(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
