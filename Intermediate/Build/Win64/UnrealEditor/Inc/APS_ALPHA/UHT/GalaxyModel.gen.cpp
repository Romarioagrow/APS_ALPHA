// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GalaxyModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxyModel() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FCelestialBodyModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGalaxyModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FGalaxyModel>() == std::is_polymorphic<FCelestialBodyModel>(), "USTRUCT FGalaxyModel cannot be polymorphic unless super FCelestialBodyModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_GalaxyModel;
class UScriptStruct* FGalaxyModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_GalaxyModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_GalaxyModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FGalaxyModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("GalaxyModel"));
	}
	return Z_Registration_Info_UScriptStruct_GalaxyModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FGalaxyModel>()
{
	return FGalaxyModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FGalaxyModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FBytePropertyParams NewProp_GalaxyType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_GalaxyType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_GalaxyClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_GalaxyClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarsCount_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_StarsCount;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxySize_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_GalaxySize;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarsDensity_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_StarsDensity;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGalaxyModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "GalaxyModel.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FGalaxyModel>();
	}
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyType_MetaData[] = {
		{ "Category", "GalaxyModel" },
		{ "ModuleRelativePath", "GalaxyModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyType = { "GalaxyType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGalaxyModel, GalaxyType), Z_Construct_UEnum_APS_ALPHA_EGalaxyType, METADATA_PARAMS(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyType_MetaData)) }; // 2575129112
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyClass_MetaData[] = {
		{ "Category", "GalaxyModel" },
		{ "ModuleRelativePath", "GalaxyModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyClass = { "GalaxyClass", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGalaxyModel, GalaxyClass), Z_Construct_UEnum_APS_ALPHA_EGalaxyClass, METADATA_PARAMS(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyClass_MetaData)) }; // 1326469318
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsCount_MetaData[] = {
		{ "Category", "GalaxyModel" },
		{ "ModuleRelativePath", "GalaxyModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsCount = { "StarsCount", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGalaxyModel, StarsCount), METADATA_PARAMS(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsCount_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsCount_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxySize_MetaData[] = {
		{ "Category", "GalaxyModel" },
		{ "ModuleRelativePath", "GalaxyModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxySize = { "GalaxySize", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGalaxyModel, GalaxySize), METADATA_PARAMS(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxySize_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxySize_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsDensity_MetaData[] = {
		{ "Category", "GalaxyModel" },
		{ "ModuleRelativePath", "GalaxyModel.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsDensity = { "StarsDensity", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FGalaxyModel, StarsDensity), METADATA_PARAMS(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsDensity_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsDensity_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FGalaxyModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxyClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsCount,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_GalaxySize,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewProp_StarsDensity,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FGalaxyModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FCelestialBodyModel,
		&NewStructOps,
		"GalaxyModel",
		sizeof(FGalaxyModel),
		alignof(FGalaxyModel),
		Z_Construct_UScriptStruct_FGalaxyModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGalaxyModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FGalaxyModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGalaxyModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FGalaxyModel()
	{
		if (!Z_Registration_Info_UScriptStruct_GalaxyModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_GalaxyModel.InnerSingleton, Z_Construct_UScriptStruct_FGalaxyModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_GalaxyModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyModel_h_Statics::ScriptStructInfo[] = {
		{ FGalaxyModel::StaticStruct, Z_Construct_UScriptStruct_FGalaxyModel_Statics::NewStructOps, TEXT("GalaxyModel"), &Z_Registration_Info_UScriptStruct_GalaxyModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FGalaxyModel), 2165413138U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyModel_h_2221967849(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
