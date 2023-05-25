// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AsteroidBelt.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAsteroidBelt() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAsteroidBelt();
	APS_ALPHA_API UClass* Z_Construct_UClass_AAsteroidBelt_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AOrbitalBody();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AAsteroidBelt::StaticRegisterNativesAAsteroidBelt()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAsteroidBelt);
	UClass* Z_Construct_UClass_AAsteroidBelt_NoRegister()
	{
		return AAsteroidBelt::StaticClass();
	}
	struct Z_Construct_UClass_AAsteroidBelt_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAsteroidBelt_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AOrbitalBody,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAsteroidBelt_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "AsteroidBelt.h" },
		{ "ModuleRelativePath", "AsteroidBelt.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAsteroidBelt_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAsteroidBelt>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAsteroidBelt_Statics::ClassParams = {
		&AAsteroidBelt::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AAsteroidBelt_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAsteroidBelt_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAsteroidBelt()
	{
		if (!Z_Registration_Info_UClass_AAsteroidBelt.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAsteroidBelt.OuterSingleton, Z_Construct_UClass_AAsteroidBelt_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAsteroidBelt.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AAsteroidBelt>()
	{
		return AAsteroidBelt::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAsteroidBelt);
	AAsteroidBelt::~AAsteroidBelt() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AsteroidBelt_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AsteroidBelt_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAsteroidBelt, AAsteroidBelt::StaticClass, TEXT("AAsteroidBelt"), &Z_Registration_Info_UClass_AAsteroidBelt, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAsteroidBelt), 320120349U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AsteroidBelt_h_1236555611(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AsteroidBelt_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AsteroidBelt_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
