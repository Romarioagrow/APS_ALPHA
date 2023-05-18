// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarSpectralClass.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarSpectralClass() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EStarSpectralClass;
	static UEnum* EStarSpectralClass_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EStarSpectralClass.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EStarSpectralClass.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EStarSpectralClass"));
		}
		return Z_Registration_Info_UEnum_EStarSpectralClass.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EStarSpectralClass>()
	{
		return EStarSpectralClass_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::Enumerators[] = {
		{ "EStarSpectralClass::O", (int64)EStarSpectralClass::O },
		{ "EStarSpectralClass::B", (int64)EStarSpectralClass::B },
		{ "EStarSpectralClass::A", (int64)EStarSpectralClass::A },
		{ "EStarSpectralClass::F", (int64)EStarSpectralClass::F },
		{ "EStarSpectralClass::G", (int64)EStarSpectralClass::G },
		{ "EStarSpectralClass::K", (int64)EStarSpectralClass::K },
		{ "EStarSpectralClass::M", (int64)EStarSpectralClass::M },
		{ "EStarSpectralClass::L", (int64)EStarSpectralClass::L },
		{ "EStarSpectralClass::T", (int64)EStarSpectralClass::T },
		{ "EStarSpectralClass::Y", (int64)EStarSpectralClass::Y },
		{ "EStarSpectralClass::WD", (int64)EStarSpectralClass::WD },
		{ "EStarSpectralClass::NS", (int64)EStarSpectralClass::NS },
		{ "EStarSpectralClass::BH", (int64)EStarSpectralClass::BH },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::Enum_MetaDataParams[] = {
		{ "A.DisplayName", "A - White" },
		{ "A.Name", "EStarSpectralClass::A" },
		{ "B.DisplayName", "B - Blue White" },
		{ "B.Name", "EStarSpectralClass::B" },
		{ "BH.DisplayName", "Black Hole" },
		{ "BH.Name", "EStarSpectralClass::BH" },
		{ "BlueprintType", "true" },
		{ "F.DisplayName", "F - Yellow White" },
		{ "F.Name", "EStarSpectralClass::F" },
		{ "G.DisplayName", "G - Yellow" },
		{ "G.Name", "EStarSpectralClass::G" },
		{ "K.DisplayName", "K - Orange" },
		{ "K.Name", "EStarSpectralClass::K" },
		{ "L.DisplayName", "L - Brown Dwarf" },
		{ "L.Name", "EStarSpectralClass::L" },
		{ "M.DisplayName", "M - Red" },
		{ "M.Name", "EStarSpectralClass::M" },
		{ "ModuleRelativePath", "StarSpectralClass.h" },
		{ "NS.DisplayName", "Neutron Star" },
		{ "NS.Name", "EStarSpectralClass::NS" },
		{ "O.DisplayName", "O - Blue" },
		{ "O.Name", "EStarSpectralClass::O" },
		{ "T.DisplayName", "T - Methane Dwarf" },
		{ "T.Name", "EStarSpectralClass::T" },
		{ "WD.DisplayName", "White Dwarf" },
		{ "WD.Name", "EStarSpectralClass::WD" },
		{ "Y.DisplayName", "Y - Cool Brown Dwarf" },
		{ "Y.Name", "EStarSpectralClass::Y" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EStarSpectralClass",
		"EStarSpectralClass",
		Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass()
	{
		if (!Z_Registration_Info_UEnum_EStarSpectralClass.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EStarSpectralClass.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EStarSpectralClass_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EStarSpectralClass.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics::EnumInfo[] = {
		{ EStarSpectralClass_StaticEnum, TEXT("EStarSpectralClass"), &Z_Registration_Info_UEnum_EStarSpectralClass, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 3859857935U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_3987062484(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
