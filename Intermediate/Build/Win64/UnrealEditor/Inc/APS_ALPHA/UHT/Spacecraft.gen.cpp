// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Spacecraft.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSpacecraft() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_APilotingVehicle();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpacecraft();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpacecraft_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ASpacecraft::StaticRegisterNativesASpacecraft()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ASpacecraft);
	UClass* Z_Construct_UClass_ASpacecraft_NoRegister()
	{
		return ASpacecraft::StaticClass();
	}
	struct Z_Construct_UClass_ASpacecraft_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ASpacecraft_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APilotingVehicle,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASpacecraft_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "Spacecraft.h" },
		{ "ModuleRelativePath", "Spacecraft.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_ASpacecraft_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASpacecraft>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ASpacecraft_Statics::ClassParams = {
		&ASpacecraft::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_ASpacecraft_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ASpacecraft_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ASpacecraft()
	{
		if (!Z_Registration_Info_UClass_ASpacecraft.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASpacecraft.OuterSingleton, Z_Construct_UClass_ASpacecraft_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ASpacecraft.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ASpacecraft>()
	{
		return ASpacecraft::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ASpacecraft);
	ASpacecraft::~ASpacecraft() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spacecraft_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spacecraft_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ASpacecraft, ASpacecraft::StaticClass, TEXT("ASpacecraft"), &Z_Registration_Info_UClass_ASpacecraft, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASpacecraft), 3161084275U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spacecraft_h_2310107150(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spacecraft_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spacecraft_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
