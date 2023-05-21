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
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralClass();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ESpectralClass;
	static UEnum* ESpectralClass_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ESpectralClass.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ESpectralClass.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_ESpectralClass, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("ESpectralClass"));
		}
		return Z_Registration_Info_UEnum_ESpectralClass.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<ESpectralClass>()
	{
		return ESpectralClass_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::Enumerators[] = {
		{ "ESpectralClass::O", (int64)ESpectralClass::O },
		{ "ESpectralClass::B", (int64)ESpectralClass::B },
		{ "ESpectralClass::A", (int64)ESpectralClass::A },
		{ "ESpectralClass::F", (int64)ESpectralClass::F },
		{ "ESpectralClass::G", (int64)ESpectralClass::G },
		{ "ESpectralClass::K", (int64)ESpectralClass::K },
		{ "ESpectralClass::M", (int64)ESpectralClass::M },
		{ "ESpectralClass::L", (int64)ESpectralClass::L },
		{ "ESpectralClass::T", (int64)ESpectralClass::T },
		{ "ESpectralClass::Y", (int64)ESpectralClass::Y },
		{ "ESpectralClass::NS", (int64)ESpectralClass::NS },
		{ "ESpectralClass::PS", (int64)ESpectralClass::PS },
		{ "ESpectralClass::BH", (int64)ESpectralClass::BH },
		{ "ESpectralClass::Unknown", (int64)ESpectralClass::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::Enum_MetaDataParams[] = {
		{ "A.DisplayName", "A - White" },
		{ "A.Name", "ESpectralClass::A" },
		{ "B.DisplayName", "B - Blue-White" },
		{ "B.Name", "ESpectralClass::B" },
		{ "BH.DisplayName", "Black Hole" },
		{ "BH.Name", "ESpectralClass::BH" },
		{ "BlueprintType", "true" },
		{ "F.DisplayName", "F - Yellow-White" },
		{ "F.Name", "ESpectralClass::F" },
		{ "G.DisplayName", "G - Yellow" },
		{ "G.Name", "ESpectralClass::G" },
		{ "K.DisplayName", "K - Orange" },
		{ "K.Name", "ESpectralClass::K" },
		{ "L.DisplayName", "L - Brown Dwarf" },
		{ "L.Name", "ESpectralClass::L" },
		{ "M.DisplayName", "M - Red" },
		{ "M.Name", "ESpectralClass::M" },
		{ "ModuleRelativePath", "StarSpectralClass.h" },
		{ "NS.DisplayName", "Neutron Star" },
		{ "NS.Name", "ESpectralClass::NS" },
		{ "O.DisplayName", "O - Blue" },
		{ "O.Name", "ESpectralClass::O" },
		{ "PS.DisplayName", "Proto Star" },
		{ "PS.Name", "ESpectralClass::PS" },
		{ "T.DisplayName", "T - Tauri Dwarf" },
		{ "T.Name", "ESpectralClass::T" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "ESpectralClass::Unknown" },
		{ "Y.DisplayName", "Y - Cool Brown Dwarf" },
		{ "Y.Name", "ESpectralClass::Y" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"ESpectralClass",
		"ESpectralClass",
		Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralClass()
	{
		if (!Z_Registration_Info_UEnum_ESpectralClass.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ESpectralClass.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_ESpectralClass_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ESpectralClass.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics::EnumInfo[] = {
		{ ESpectralClass_StaticEnum, TEXT("ESpectralClass"), &Z_Registration_Info_UEnum_ESpectralClass, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 1101356590U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_2513888165(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSpectralClass_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
