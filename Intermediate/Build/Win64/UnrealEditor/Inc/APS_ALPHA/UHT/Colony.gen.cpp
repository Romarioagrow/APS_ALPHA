// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Colony.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeColony() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AColony();
	APS_ALPHA_API UClass* Z_Construct_UClass_AColony_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AColony::StaticRegisterNativesAColony()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AColony);
	UClass* Z_Construct_UClass_AColony_NoRegister()
	{
		return AColony::StaticClass();
	}
	struct Z_Construct_UClass_AColony_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AColony_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ATechActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AColony_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Colony.h" },
		{ "ModuleRelativePath", "Colony.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AColony_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AColony>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AColony_Statics::ClassParams = {
		&AColony::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AColony_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AColony_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AColony()
	{
		if (!Z_Registration_Info_UClass_AColony.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AColony.OuterSingleton, Z_Construct_UClass_AColony_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AColony.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AColony>()
	{
		return AColony::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AColony);
	AColony::~AColony() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Colony_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Colony_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AColony, AColony::StaticClass, TEXT("AColony"), &Z_Registration_Info_UClass_AColony, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AColony), 3139935558U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Colony_h_750446117(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Colony_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Colony_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
