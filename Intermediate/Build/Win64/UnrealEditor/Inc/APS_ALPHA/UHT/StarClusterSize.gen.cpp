// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarClusterSize.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarClusterSize() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterSize();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EStarClusterSize;
	static UEnum* EStarClusterSize_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterSize.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EStarClusterSize.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EStarClusterSize, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EStarClusterSize"));
		}
		return Z_Registration_Info_UEnum_EStarClusterSize.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterSize>()
	{
		return EStarClusterSize_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::Enumerators[] = {
		{ "EStarClusterSize::Tiny", (int64)EStarClusterSize::Tiny },
		{ "EStarClusterSize::Small", (int64)EStarClusterSize::Small },
		{ "EStarClusterSize::Medium", (int64)EStarClusterSize::Medium },
		{ "EStarClusterSize::Large", (int64)EStarClusterSize::Large },
		{ "EStarClusterSize::Giant", (int64)EStarClusterSize::Giant },
		{ "EStarClusterSize::Unknown", (int64)EStarClusterSize::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Giant.DisplayName", "Giant" },
		{ "Giant.Name", "EStarClusterSize::Giant" },
		{ "Large.DisplayName", "Large" },
		{ "Large.Name", "EStarClusterSize::Large" },
		{ "Medium.DisplayName", "Medium" },
		{ "Medium.Name", "EStarClusterSize::Medium" },
		{ "ModuleRelativePath", "StarClusterSize.h" },
		{ "Small.DisplayName", "Small" },
		{ "Small.Name", "EStarClusterSize::Small" },
		{ "Tiny.DisplayName", "Tiny" },
		{ "Tiny.Name", "EStarClusterSize::Tiny" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "EStarClusterSize::Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EStarClusterSize",
		"EStarClusterSize",
		Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterSize()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterSize.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EStarClusterSize.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EStarClusterSize_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EStarClusterSize.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterSize_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterSize_h_Statics::EnumInfo[] = {
		{ EStarClusterSize_StaticEnum, TEXT("EStarClusterSize"), &Z_Registration_Info_UEnum_EStarClusterSize, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 1922975825U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterSize_h_2337105716(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterSize_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterSize_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
