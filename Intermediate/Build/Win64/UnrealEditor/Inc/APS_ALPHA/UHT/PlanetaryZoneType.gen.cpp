// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetaryZoneType.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetaryZoneType() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EPlanetaryZoneType;
	static UEnum* EPlanetaryZoneType_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EPlanetaryZoneType.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EPlanetaryZoneType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EPlanetaryZoneType"));
		}
		return Z_Registration_Info_UEnum_EPlanetaryZoneType.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EPlanetaryZoneType>()
	{
		return EPlanetaryZoneType_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::Enumerators[] = {
		{ "EPlanetaryZoneType::DeadZone", (int64)EPlanetaryZoneType::DeadZone },
		{ "EPlanetaryZoneType::HabitableZone", (int64)EPlanetaryZoneType::HabitableZone },
		{ "EPlanetaryZoneType::ColdZone", (int64)EPlanetaryZoneType::ColdZone },
		{ "EPlanetaryZoneType::IceZone", (int64)EPlanetaryZoneType::IceZone },
		{ "EPlanetaryZoneType::WarmZone", (int64)EPlanetaryZoneType::WarmZone },
		{ "EPlanetaryZoneType::HotZone", (int64)EPlanetaryZoneType::HotZone },
		{ "EPlanetaryZoneType::InnerPlanetZone", (int64)EPlanetaryZoneType::InnerPlanetZone },
		{ "EPlanetaryZoneType::OuterPlanetZone", (int64)EPlanetaryZoneType::OuterPlanetZone },
		{ "EPlanetaryZoneType::AsteroidBeltZone", (int64)EPlanetaryZoneType::AsteroidBeltZone },
		{ "EPlanetaryZoneType::KuiperBeltZone", (int64)EPlanetaryZoneType::KuiperBeltZone },
		{ "EPlanetaryZoneType::GasGiantsZone", (int64)EPlanetaryZoneType::GasGiantsZone },
		{ "EPlanetaryZoneType::Unknown", (int64)EPlanetaryZoneType::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::Enum_MetaDataParams[] = {
		{ "AsteroidBeltZone.DisplayName", "Asteroid Belt Zone" },
		{ "AsteroidBeltZone.Name", "EPlanetaryZoneType::AsteroidBeltZone" },
		{ "BlueprintType", "true" },
		{ "ColdZone.DisplayName", "Cold Zone" },
		{ "ColdZone.Name", "EPlanetaryZoneType::ColdZone" },
		{ "DeadZone.DisplayName", "Dead Zone" },
		{ "DeadZone.Name", "EPlanetaryZoneType::DeadZone" },
		{ "GasGiantsZone.DisplayName", "Gas Giants Zone" },
		{ "GasGiantsZone.Name", "EPlanetaryZoneType::GasGiantsZone" },
		{ "HabitableZone.DisplayName", "Habitable Zone" },
		{ "HabitableZone.Name", "EPlanetaryZoneType::HabitableZone" },
		{ "HotZone.DisplayName", "Hot Zone" },
		{ "HotZone.Name", "EPlanetaryZoneType::HotZone" },
		{ "IceZone.DisplayName", "Ice Zone" },
		{ "IceZone.Name", "EPlanetaryZoneType::IceZone" },
		{ "InnerPlanetZone.DisplayName", "Inner Planet Zone" },
		{ "InnerPlanetZone.Name", "EPlanetaryZoneType::InnerPlanetZone" },
		{ "KuiperBeltZone.DisplayName", "Kuiper Belt Zone" },
		{ "KuiperBeltZone.Name", "EPlanetaryZoneType::KuiperBeltZone" },
		{ "ModuleRelativePath", "PlanetaryZoneType.h" },
		{ "OuterPlanetZone.DisplayName", "Outer Planet Zone" },
		{ "OuterPlanetZone.Name", "EPlanetaryZoneType::OuterPlanetZone" },
		{ "Unknown.DisplayName", "Unknown Zone" },
		{ "Unknown.Name", "EPlanetaryZoneType::Unknown" },
		{ "WarmZone.DisplayName", "Warm Zone" },
		{ "WarmZone.Name", "EPlanetaryZoneType::WarmZone" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EPlanetaryZoneType",
		"EPlanetaryZoneType",
		Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType()
	{
		if (!Z_Registration_Info_UEnum_EPlanetaryZoneType.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EPlanetaryZoneType.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EPlanetaryZoneType_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EPlanetaryZoneType.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryZoneType_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryZoneType_h_Statics::EnumInfo[] = {
		{ EPlanetaryZoneType_StaticEnum, TEXT("EPlanetaryZoneType"), &Z_Registration_Info_UEnum_EPlanetaryZoneType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 266526262U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryZoneType_h_2467347768(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryZoneType_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryZoneType_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
