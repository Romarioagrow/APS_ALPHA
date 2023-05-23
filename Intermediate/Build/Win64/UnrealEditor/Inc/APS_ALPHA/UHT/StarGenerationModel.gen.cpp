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
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStellarClass();
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
		static const UECodeGen_Private::FBytePropertyParams NewProp_StellarClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StellarClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StellarClass;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SpectralClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SpectralClass;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SpectralType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SpectralType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Luminosity_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Luminosity;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SurfaceTemperature_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_SurfaceTemperature;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Age_MetaData[];
#endif
		static const UECodeGen_Private::FStrPropertyParams NewProp_Age;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_FullSpectralClass_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_FullSpectralClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_FullSpectralName_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_FullSpectralName;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralSubclass_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_SpectralSubclass;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarStellarClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarStellarClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarStellarClass;
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
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StellarClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StellarClass_MetaData[] = {
		{ "Category", "Star" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd (\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd.\xef\xbf\xbd.)\n" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd (\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd, \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd \xef\xbf\xbd.\xef\xbf\xbd.)" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StellarClass = { "StellarClass", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, StellarClass), Z_Construct_UEnum_APS_ALPHA_EStellarClass, METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StellarClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StellarClass_MetaData)) }; // 3378852079
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralClass_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralClass = { "SpectralClass", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, SpectralClass), Z_Construct_UEnum_APS_ALPHA_ESpectralClass, METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralClass_MetaData)) }; // 1101356590
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralType_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralType = { "SpectralType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, SpectralType), Z_Construct_UEnum_APS_ALPHA_ESpectralType, METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralType_MetaData)) }; // 3694631182
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
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Age_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Age = { "Age", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, Age), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Age_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Age_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralClass_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralClass = { "FullSpectralClass", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, FullSpectralClass), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralName_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralName = { "FullSpectralName", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, FullSpectralName), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralName_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralName_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralSubclass_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralSubclass = { "SpectralSubclass", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, SpectralSubclass), METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralSubclass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralSubclass_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarStellarClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarStellarClass_MetaData[] = {
		{ "Category", "StarGenerationModel" },
		{ "ModuleRelativePath", "StarGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarStellarClass = { "StarStellarClass", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FStarGenerationModel, StarStellarClass), Z_Construct_UEnum_APS_ALPHA_EStellarClass, METADATA_PARAMS(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarStellarClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarStellarClass_MetaData)) }; // 3378852079
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FStarGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StellarClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StellarClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Luminosity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SurfaceTemperature,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_Age,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_FullSpectralName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_SpectralSubclass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarStellarClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewProp_StarStellarClass,
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
		{ FStarGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FStarGenerationModel_Statics::NewStructOps, TEXT("StarGenerationModel"), &Z_Registration_Info_UScriptStruct_StarGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FStarGenerationModel), 871323455U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_2191269459(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
