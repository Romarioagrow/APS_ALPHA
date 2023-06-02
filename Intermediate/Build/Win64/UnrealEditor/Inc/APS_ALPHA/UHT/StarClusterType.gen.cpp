// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarClusterType.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarClusterType() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EStarClusterType;
	static UEnum* EStarClusterType_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterType.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EStarClusterType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EStarClusterType, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EStarClusterType"));
		}
		return Z_Registration_Info_UEnum_EStarClusterType.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterType>()
	{
		return EStarClusterType_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::Enumerators[] = {
		{ "EStarClusterType::OpenCluster", (int64)EStarClusterType::OpenCluster },
		{ "EStarClusterType::GlobularCluster", (int64)EStarClusterType::GlobularCluster },
		{ "EStarClusterType::Supercluster", (int64)EStarClusterType::Supercluster },
		{ "EStarClusterType::Nebula", (int64)EStarClusterType::Nebula },
		{ "EStarClusterType::Unknown", (int64)EStarClusterType::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "GlobularCluster.DisplayName", "Globular Cluster" },
		{ "GlobularCluster.Name", "EStarClusterType::GlobularCluster" },
		{ "ModuleRelativePath", "StarClusterType.h" },
		{ "Nebula.DisplayName", "Nebula" },
		{ "Nebula.Name", "EStarClusterType::Nebula" },
		{ "OpenCluster.DisplayName", "Open Cluster" },
		{ "OpenCluster.Name", "EStarClusterType::OpenCluster" },
		{ "Supercluster.DisplayName", "Supercluster" },
		{ "Supercluster.Name", "EStarClusterType::Supercluster" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "EStarClusterType::Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EStarClusterType",
		"EStarClusterType",
		Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterType()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterType.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EStarClusterType.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EStarClusterType_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EStarClusterType.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterType_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterType_h_Statics::EnumInfo[] = {
		{ EStarClusterType_StaticEnum, TEXT("EStarClusterType"), &Z_Registration_Info_UEnum_EStarClusterType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 651664193U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterType_h_1923270878(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterType_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterType_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
