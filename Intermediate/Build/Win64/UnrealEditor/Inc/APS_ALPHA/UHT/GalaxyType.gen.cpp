// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GalaxyType.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxyType() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EGalaxyType;
	static UEnum* EGalaxyType_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EGalaxyType.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EGalaxyType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EGalaxyType, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EGalaxyType"));
		}
		return Z_Registration_Info_UEnum_EGalaxyType.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EGalaxyType>()
	{
		return EGalaxyType_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::Enumerators[] = {
		{ "EGalaxyType::Elliptical", (int64)EGalaxyType::Elliptical },
		{ "EGalaxyType::Lenticular", (int64)EGalaxyType::Lenticular },
		{ "EGalaxyType::Spiral", (int64)EGalaxyType::Spiral },
		{ "EGalaxyType::BarredSpiral", (int64)EGalaxyType::BarredSpiral },
		{ "EGalaxyType::Irregular", (int64)EGalaxyType::Irregular },
		{ "EGalaxyType::Peculiar", (int64)EGalaxyType::Peculiar },
		{ "EGalaxyType::Unknown", (int64)EGalaxyType::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::Enum_MetaDataParams[] = {
		{ "BarredSpiral.DisplayName", "Barred Spiral Galaxy" },
		{ "BarredSpiral.Name", "EGalaxyType::BarredSpiral" },
		{ "BlueprintType", "true" },
		{ "Elliptical.DisplayName", "Elliptical Galaxy" },
		{ "Elliptical.Name", "EGalaxyType::Elliptical" },
		{ "Irregular.DisplayName", "Irregular Galaxy" },
		{ "Irregular.Name", "EGalaxyType::Irregular" },
		{ "Lenticular.DisplayName", "Lenticular Galaxy" },
		{ "Lenticular.Name", "EGalaxyType::Lenticular" },
		{ "ModuleRelativePath", "GalaxyType.h" },
		{ "Peculiar.DisplayName", "Peculiar Galaxy" },
		{ "Peculiar.Name", "EGalaxyType::Peculiar" },
		{ "Spiral.DisplayName", "Spiral Galaxy" },
		{ "Spiral.Name", "EGalaxyType::Spiral" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "EGalaxyType::Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EGalaxyType",
		"EGalaxyType",
		Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyType()
	{
		if (!Z_Registration_Info_UEnum_EGalaxyType.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EGalaxyType.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EGalaxyType_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EGalaxyType.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyType_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyType_h_Statics::EnumInfo[] = {
		{ EGalaxyType_StaticEnum, TEXT("EGalaxyType"), &Z_Registration_Info_UEnum_EGalaxyType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2575129112U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyType_h_682422206(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyType_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyType_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
