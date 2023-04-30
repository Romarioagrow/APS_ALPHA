// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Moon.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMoon() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AMoon();
	APS_ALPHA_API UClass* Z_Construct_UClass_AMoon_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AOrbitalBody();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AMoon::StaticRegisterNativesAMoon()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AMoon);
	UClass* Z_Construct_UClass_AMoon_NoRegister()
	{
		return AMoon::StaticClass();
	}
	struct Z_Construct_UClass_AMoon_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AMoon_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AOrbitalBody,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AMoon_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Moon.h" },
		{ "ModuleRelativePath", "Moon.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AMoon_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AMoon>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AMoon_Statics::ClassParams = {
		&AMoon::StaticClass,
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
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AMoon_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AMoon_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AMoon()
	{
		if (!Z_Registration_Info_UClass_AMoon.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AMoon.OuterSingleton, Z_Construct_UClass_AMoon_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AMoon.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AMoon>()
	{
		return AMoon::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AMoon);
	AMoon::~AMoon() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AMoon, AMoon::StaticClass, TEXT("AMoon"), &Z_Registration_Info_UClass_AMoon, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AMoon), 2651434503U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_893679733(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
