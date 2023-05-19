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
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarType();
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
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Luminosity_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Luminosity;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SurfaceTemperature_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_SurfaceTemperature;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarSpectralClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarSpectralClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarSpectralClass;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
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
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarType_MetaData[] = {
		{ "Category", "Star" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd (\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd.\xef\xbf\xbd.)\n" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd (\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd.\xef\xbf\xbd.)" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarType = { "StarType", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, StarType), Z_Construct_UEnum_APS_ALPHA_EStarType, METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarType_MetaData)) }; // 1510841097
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Luminosity_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Luminosity = { "Luminosity", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, Luminosity), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Luminosity_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Luminosity_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SurfaceTemperature_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SurfaceTemperature = { "SurfaceTemperature", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, SurfaceTemperature), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SurfaceTemperature_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SurfaceTemperature_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarSpectralClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarSpectralClass_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarSpectralClass = { "StarSpectralClass", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, StarSpectralClass), Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass, METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarSpectralClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarSpectralClass_MetaData)) }; // 3859857935
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FStarGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Luminosity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SurfaceTemperature,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarSpectralClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarSpectralClass,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FCelestialGenerationModel,
		&NewStructOps,
		"StarGenerationModel",
		sizeof(FStarGenerationModel),
		alignof(FStarGenerationModel),
		Z_Construct_UScriptStruct_FStarGenerationModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::PropPointers),
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
		{ FStarGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewStructOps, TEXT("StarGenerationModel"), &Z_Registration_Info_UScriptStruct_StarGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FStarGenerationModel), 858023757U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_4024271729(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
