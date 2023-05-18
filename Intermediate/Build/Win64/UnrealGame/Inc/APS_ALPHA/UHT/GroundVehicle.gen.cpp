// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GroundVehicle.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGroundVehicle() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGroundVehicle();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGroundVehicle_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APilotingVehicle();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGroundVehicle::StaticRegisterNativesAGroundVehicle()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGroundVehicle);
	UClass* Z_Construct_UClass_AGroundVehicle_NoRegister()
	{
		return AGroundVehicle::StaticClass();
	}
	struct Z_Construct_UClass_AGroundVehicle_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGroundVehicle_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APilotingVehicle,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGroundVehicle_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "GroundVehicle.h" },
		{ "ModuleRelativePath", "GroundVehicle.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGroundVehicle_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGroundVehicle>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGroundVehicle_Statics::ClassParams = {
		&AGroundVehicle::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AGroundVehicle_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGroundVehicle_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGroundVehicle()
	{
		if (!Z_Registration_Info_UClass_AGroundVehicle.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGroundVehicle.OuterSingleton, Z_Construct_UClass_AGroundVehicle_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGroundVehicle.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGroundVehicle>()
	{
		return AGroundVehicle::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGroundVehicle);
	AGroundVehicle::~AGroundVehicle() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GroundVehicle_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GroundVehicle_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGroundVehicle, AGroundVehicle::StaticClass, TEXT("AGroundVehicle"), &Z_Registration_Info_UClass_AGroundVehicle, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGroundVehicle), 778393448U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GroundVehicle_h_2587240279(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GroundVehicle_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GroundVehicle_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
