// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarClusterComposition.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarClusterComposition() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EStarClusterComposition;
	static UEnum* EStarClusterComposition_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterComposition.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EStarClusterComposition.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EStarClusterComposition"));
		}
		return Z_Registration_Info_UEnum_EStarClusterComposition.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterComposition>()
	{
		return EStarClusterComposition_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::Enumerators[] = {
		{ "EStarClusterComposition::AllSpectral", (int64)EStarClusterComposition::AllSpectral },
		{ "EStarClusterComposition::OnlyBlue", (int64)EStarClusterComposition::OnlyBlue },
		{ "EStarClusterComposition::MostlyBlue", (int64)EStarClusterComposition::MostlyBlue },
		{ "EStarClusterComposition::BlueWhite", (int64)EStarClusterComposition::BlueWhite },
		{ "EStarClusterComposition::OnlyWhite", (int64)EStarClusterComposition::OnlyWhite },
		{ "EStarClusterComposition::MostlyWhite", (int64)EStarClusterComposition::MostlyWhite },
		{ "EStarClusterComposition::WhiteYellow", (int64)EStarClusterComposition::WhiteYellow },
		{ "EStarClusterComposition::OnlyYellow", (int64)EStarClusterComposition::OnlyYellow },
		{ "EStarClusterComposition::MostlyYellow", (int64)EStarClusterComposition::MostlyYellow },
		{ "EStarClusterComposition::YellowOrange", (int64)EStarClusterComposition::YellowOrange },
		{ "EStarClusterComposition::OnlyOrange", (int64)EStarClusterComposition::OnlyOrange },
		{ "EStarClusterComposition::MostlyOrange", (int64)EStarClusterComposition::MostlyOrange },
		{ "EStarClusterComposition::OrangeRed", (int64)EStarClusterComposition::OrangeRed },
		{ "EStarClusterComposition::OnlyRed", (int64)EStarClusterComposition::OnlyRed },
		{ "EStarClusterComposition::MostlyRed", (int64)EStarClusterComposition::MostlyRed },
		{ "EStarClusterComposition::Unknown", (int64)EStarClusterComposition::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::Enum_MetaDataParams[] = {
		{ "AllSpectral.DisplayName", "All Spectral" },
		{ "AllSpectral.Name", "EStarClusterComposition::AllSpectral" },
		{ "BlueprintType", "true" },
		{ "BlueWhite.DisplayName", "Blue White" },
		{ "BlueWhite.Name", "EStarClusterComposition::BlueWhite" },
		{ "ModuleRelativePath", "StarClusterComposition.h" },
		{ "MostlyBlue.DisplayName", "Mostly Blue" },
		{ "MostlyBlue.Name", "EStarClusterComposition::MostlyBlue" },
		{ "MostlyOrange.DisplayName", "Mostly Orange" },
		{ "MostlyOrange.Name", "EStarClusterComposition::MostlyOrange" },
		{ "MostlyRed.DisplayName", "Mostly Red" },
		{ "MostlyRed.Name", "EStarClusterComposition::MostlyRed" },
		{ "MostlyWhite.DisplayName", "Mostly White" },
		{ "MostlyWhite.Name", "EStarClusterComposition::MostlyWhite" },
		{ "MostlyYellow.DisplayName", "Mostly Yellow" },
		{ "MostlyYellow.Name", "EStarClusterComposition::MostlyYellow" },
		{ "OnlyBlue.DisplayName", "Only Blue" },
		{ "OnlyBlue.Name", "EStarClusterComposition::OnlyBlue" },
		{ "OnlyOrange.DisplayName", "Only Orange" },
		{ "OnlyOrange.Name", "EStarClusterComposition::OnlyOrange" },
		{ "OnlyRed.DisplayName", "Only Red" },
		{ "OnlyRed.Name", "EStarClusterComposition::OnlyRed" },
		{ "OnlyWhite.DisplayName", "Only White" },
		{ "OnlyWhite.Name", "EStarClusterComposition::OnlyWhite" },
		{ "OnlyYellow.DisplayName", "Only Yellow" },
		{ "OnlyYellow.Name", "EStarClusterComposition::OnlyYellow" },
		{ "OrangeRed.DisplayName", "Orange Red" },
		{ "OrangeRed.Name", "EStarClusterComposition::OrangeRed" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "EStarClusterComposition::Unknown" },
		{ "WhiteYellow.DisplayName", "White Yellow" },
		{ "WhiteYellow.Name", "EStarClusterComposition::WhiteYellow" },
		{ "YellowOrange.DisplayName", "Yellow Orange" },
		{ "YellowOrange.Name", "EStarClusterComposition::YellowOrange" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EStarClusterComposition",
		"EStarClusterComposition",
		Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterComposition.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EStarClusterComposition.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EStarClusterComposition_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EStarClusterComposition.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterComposition_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterComposition_h_Statics::EnumInfo[] = {
		{ EStarClusterComposition_StaticEnum, TEXT("EStarClusterComposition"), &Z_Registration_Info_UEnum_EStarClusterComposition, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 399386891U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterComposition_h_2160719517(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterComposition_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterComposition_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
