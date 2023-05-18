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
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FCelestialGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FOrbitalBodyGenerationModel>() == std::is_polymorphic<FCelestialGenerationModel>(), "USTRUCT FOrbitalBodyGenerationModel cannot be polymorphic unless super FCelestialGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel;
class UScriptStruct* FOrbitalBodyGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("OrbitalBodyGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FOrbitalBodyGenerationModel>()
{
	return FOrbitalBodyGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RotationSpeed_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_RotationSpeed;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OrbitDistance_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_OrbitDistance;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "OrbitalBodyGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FOrbitalBodyGenerationModel>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_RotationSpeed_MetaData[] = {
		{ "Category", "OrbitalBodyGenerationModel" },
		{ "ModuleRelativePath", "OrbitalBodyGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_RotationSpeed = { "RotationSpeed", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FOrbitalBodyGenerationModel, RotationSpeed), METADATA_PARAMS(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_RotationSpeed_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_RotationSpeed_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_OrbitDistance_MetaData[] = {
		{ "Category", "OrbitalBodyGenerationModel" },
		{ "ModuleRelativePath", "OrbitalBodyGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_OrbitDistance = { "OrbitDistance", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FOrbitalBodyGenerationModel, OrbitDistance), METADATA_PARAMS(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_OrbitDistance_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_OrbitDistance_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_RotationSpeed,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewProp_OrbitDistance,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FCelestialGenerationModel,
		&NewStructOps,
		"OrbitalBodyGenerationModel",
		sizeof(FOrbitalBodyGenerationModel),
		alignof(FOrbitalBodyGenerationModel),
		Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FOrbitalBodyGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FOrbitalBodyGenerationModel_Statics::NewStructOps, TEXT("OrbitalBodyGenerationModel"), &Z_Registration_Info_UScriptStruct_OrbitalBodyGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FOrbitalBodyGenerationModel), 2170198895U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_1585755083(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBodyGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
