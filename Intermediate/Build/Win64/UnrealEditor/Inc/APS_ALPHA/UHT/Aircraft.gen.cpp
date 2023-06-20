// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Aircraft.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAircraft() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAircraft();
	APS_ALPHA_API UClass* Z_Construct_UClass_AAircraft_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APilotingVehicle();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AAircraft::StaticRegisterNativesAAircraft()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAircraft);
	UClass* Z_Construct_UClass_AAircraft_NoRegister()
	{
		return AAircraft::StaticClass();
	}
	struct Z_Construct_UClass_AAircraft_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAircraft_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APilotingVehicle,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAircraft_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "Aircraft.h" },
		{ "ModuleRelativePath", "Aircraft.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAircraft_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAircraft>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAircraft_Statics::ClassParams = {
		&AAircraft::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AAircraft_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAircraft_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAircraft()
	{
		if (!Z_Registration_Info_UClass_AAircraft.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAircraft.OuterSingleton, Z_Construct_UClass_AAircraft_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAircraft.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AAircraft>()
	{
		return AAircraft::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAircraft);
	AAircraft::~AAircraft() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Aircraft_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Aircraft_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAircraft, AAircraft::StaticClass, TEXT("AAircraft"), &Z_Registration_Info_UClass_AAircraft, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAircraft), 3544092636U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Aircraft_h_3925237808(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Aircraft_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Aircraft_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
