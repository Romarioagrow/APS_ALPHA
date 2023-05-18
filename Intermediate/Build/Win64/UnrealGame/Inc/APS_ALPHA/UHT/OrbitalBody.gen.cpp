// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/OrbitalBody.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeOrbitalBody() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialBody();
	APS_ALPHA_API UClass* Z_Construct_UClass_AOrbitalBody();
	APS_ALPHA_API UClass* Z_Construct_UClass_AOrbitalBody_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravitySource_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AOrbitalBody::StaticRegisterNativesAOrbitalBody()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AOrbitalBody);
	UClass* Z_Construct_UClass_AOrbitalBody_NoRegister()
	{
		return AOrbitalBody::StaticClass();
	}
	struct Z_Construct_UClass_AOrbitalBody_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AOrbitalBody_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialBody,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AOrbitalBody_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "OrbitalBody.h" },
		{ "ModuleRelativePath", "OrbitalBody.h" },
	};
#endif
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_AOrbitalBody_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UGravitySource_NoRegister, (int32)VTABLE_OFFSET(AOrbitalBody, IGravitySource), false },  // 1846880873
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AOrbitalBody_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AOrbitalBody>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AOrbitalBody_Statics::ClassParams = {
		&AOrbitalBody::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		UE_ARRAY_COUNT(InterfaceParams),
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AOrbitalBody_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AOrbitalBody_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AOrbitalBody()
	{
		if (!Z_Registration_Info_UClass_AOrbitalBody.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AOrbitalBody.OuterSingleton, Z_Construct_UClass_AOrbitalBody_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AOrbitalBody.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AOrbitalBody>()
	{
		return AOrbitalBody::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AOrbitalBody);
	AOrbitalBody::~AOrbitalBody() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AOrbitalBody, AOrbitalBody::StaticClass, TEXT("AOrbitalBody"), &Z_Registration_Info_UClass_AOrbitalBody, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AOrbitalBody), 2697028474U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_3687336844(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitalBody_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
