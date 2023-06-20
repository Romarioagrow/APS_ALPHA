// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Headquarters.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeHeadquarters() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AHeadquarters();
	APS_ALPHA_API UClass* Z_Construct_UClass_AHeadquarters_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AHeadquarters::StaticRegisterNativesAHeadquarters()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AHeadquarters);
	UClass* Z_Construct_UClass_AHeadquarters_NoRegister()
	{
		return AHeadquarters::StaticClass();
	}
	struct Z_Construct_UClass_AHeadquarters_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AHeadquarters_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ATechActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AHeadquarters_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Headquarters.h" },
		{ "ModuleRelativePath", "Headquarters.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AHeadquarters_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AHeadquarters>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AHeadquarters_Statics::ClassParams = {
		&AHeadquarters::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009000A5u,
		METADATA_PARAMS(Z_Construct_UClass_AHeadquarters_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AHeadquarters_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AHeadquarters()
	{
		if (!Z_Registration_Info_UClass_AHeadquarters.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AHeadquarters.OuterSingleton, Z_Construct_UClass_AHeadquarters_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AHeadquarters.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AHeadquarters>()
	{
		return AHeadquarters::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AHeadquarters);
	AHeadquarters::~AHeadquarters() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AHeadquarters, AHeadquarters::StaticClass, TEXT("AHeadquarters"), &Z_Registration_Info_UClass_AHeadquarters, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AHeadquarters), 4130004412U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_2426860378(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
