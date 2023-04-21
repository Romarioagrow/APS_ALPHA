// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetGravityActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetGravityActor() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetGravityActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetGravityActor_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void APlanetGravityActor::StaticRegisterNativesAPlanetGravityActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(APlanetGravityActor);
	UClass* Z_Construct_UClass_APlanetGravityActor_NoRegister()
	{
		return APlanetGravityActor::StaticClass();
	}
	struct Z_Construct_UClass_APlanetGravityActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_APlanetGravityActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AGravityActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetGravityActor_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "PlanetGravityActor.h" },
		{ "ModuleRelativePath", "PlanetGravityActor.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_APlanetGravityActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<APlanetGravityActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_APlanetGravityActor_Statics::ClassParams = {
		&APlanetGravityActor::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_APlanetGravityActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetGravityActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_APlanetGravityActor()
	{
		if (!Z_Registration_Info_UClass_APlanetGravityActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_APlanetGravityActor.OuterSingleton, Z_Construct_UClass_APlanetGravityActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_APlanetGravityActor.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<APlanetGravityActor>()
	{
		return APlanetGravityActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(APlanetGravityActor);
	APlanetGravityActor::~APlanetGravityActor() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGravityActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGravityActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_APlanetGravityActor, APlanetGravityActor::StaticClass, TEXT("APlanetGravityActor"), &Z_Registration_Info_UClass_APlanetGravityActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(APlanetGravityActor), 3134933121U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGravityActor_h_3111497036(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGravityActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetGravityActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
