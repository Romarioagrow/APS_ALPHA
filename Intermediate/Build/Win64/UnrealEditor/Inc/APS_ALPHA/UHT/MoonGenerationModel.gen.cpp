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
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EMoonType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FMoonModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FOrbitalBodyModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FMoonModel>() == std::is_polymorphic<FOrbitalBodyModel>(), "USTRUCT FMoonModel cannot be polymorphic unless super FOrbitalBodyModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_MoonModel;
class UScriptStruct* FMoonModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_MoonModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_MoonModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FMoonModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("MoonModel"));
	}
	return Z_Registration_Info_UScriptStruct_MoonModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FMoonModel>()
{
	return FMoonModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FMoonModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FBytePropertyParams NewProp_Type_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Type_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_Type;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoonDensity_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_MoonDensity;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoonGravity_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_MoonGravity;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "MoonGenerationModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FMoonModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FMoonModel>();
	}
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_Type_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_Type_MetaData[] = {
		{ "Category", "Moon Generation Model" },
		{ "ModuleRelativePath", "MoonGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_Type = { "Type", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FMoonModel, Type), Z_Construct_UEnum_APS_ALPHA_EMoonType, METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_Type_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_Type_MetaData)) }; // 647168076
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonDensity_MetaData[] = {
		{ "Category", "Moon Generation Model" },
		{ "ModuleRelativePath", "MoonGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonDensity = { "MoonDensity", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FMoonModel, MoonDensity), METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonDensity_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonDensity_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonGravity_MetaData[] = {
		{ "Category", "Moon Generation Model" },
		{ "ModuleRelativePath", "MoonGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonGravity = { "MoonGravity", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FMoonModel, MoonGravity), METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonGravity_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonGravity_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FMoonModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_Type_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_Type,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonDensity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMoonModel_Statics::NewProp_MoonGravity,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FMoonModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FOrbitalBodyModel,
		&NewStructOps,
		"MoonModel",
		sizeof(FMoonModel),
		alignof(FMoonModel),
		Z_Construct_UScriptStruct_FMoonModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FMoonModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMoonModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FMoonModel()
	{
		if (!Z_Registration_Info_UScriptStruct_MoonModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_MoonModel.InnerSingleton, Z_Construct_UScriptStruct_FMoonModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_MoonModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FMoonModel::StaticStruct, Z_Construct_UScriptStruct_FMoonModel_Statics::NewStructOps, TEXT("MoonModel"), &Z_Registration_Info_UScriptStruct_MoonModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FMoonModel), 81838072U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_2061686210(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MoonGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
