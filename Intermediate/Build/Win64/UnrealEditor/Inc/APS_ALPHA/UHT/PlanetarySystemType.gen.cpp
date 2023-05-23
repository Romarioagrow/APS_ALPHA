// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetarySystemType.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetarySystemType() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EPlanetarySystemType;
	static UEnum* EPlanetarySystemType_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EPlanetarySystemType.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EPlanetarySystemType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EPlanetarySystemType"));
		}
		return Z_Registration_Info_UEnum_EPlanetarySystemType.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EPlanetarySystemType>()
	{
		return EPlanetarySystemType_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::Enumerators[] = {
		{ "EPlanetarySystemType::NoPlanetSystem", (int64)EPlanetarySystemType::NoPlanetSystem },
		{ "EPlanetarySystemType::SinglePlanetSystem", (int64)EPlanetarySystemType::SinglePlanetSystem },
		{ "EPlanetarySystemType::MultiPlanetSystem", (int64)EPlanetarySystemType::MultiPlanetSystem },
		{ "EPlanetarySystemType::HabitableZoneSystem", (int64)EPlanetarySystemType::HabitableZoneSystem },
		{ "EPlanetarySystemType::GasGiantDominatedSystem", (int64)EPlanetarySystemType::GasGiantDominatedSystem },
		{ "EPlanetarySystemType::Unknown", (int64)EPlanetarySystemType::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "GasGiantDominatedSystem.DisplayName", "Gas Giant Dominated System" },
		{ "GasGiantDominatedSystem.Name", "EPlanetarySystemType::GasGiantDominatedSystem" },
		{ "HabitableZoneSystem.DisplayName", "Habitable Zone System" },
		{ "HabitableZoneSystem.Name", "EPlanetarySystemType::HabitableZoneSystem" },
		{ "ModuleRelativePath", "PlanetarySystemType.h" },
		{ "MultiPlanetSystem.DisplayName", "Multi Planet System" },
		{ "MultiPlanetSystem.Name", "EPlanetarySystemType::MultiPlanetSystem" },
		{ "NoPlanetSystem.DisplayName", "No Planet System" },
		{ "NoPlanetSystem.Name", "EPlanetarySystemType::NoPlanetSystem" },
		{ "SinglePlanetSystem.DisplayName", "Single Planet System" },
		{ "SinglePlanetSystem.Name", "EPlanetarySystemType::SinglePlanetSystem" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "EPlanetarySystemType::Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EPlanetarySystemType",
		"EPlanetarySystemType",
		Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType()
	{
		if (!Z_Registration_Info_UEnum_EPlanetarySystemType.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EPlanetarySystemType.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EPlanetarySystemType.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemType_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemType_h_Statics::EnumInfo[] = {
		{ EPlanetarySystemType_StaticEnum, TEXT("EPlanetarySystemType"), &Z_Registration_Info_UEnum_EPlanetarySystemType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 3370777339U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemType_h_3724600972(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemType_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemType_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
