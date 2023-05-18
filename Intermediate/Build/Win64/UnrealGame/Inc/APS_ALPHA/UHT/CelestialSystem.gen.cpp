// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/CelestialSystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCelestialSystem() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstonomicalSystem_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ACelestialSystem::StaticRegisterNativesACelestialSystem()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ACelestialSystem);
	UClass* Z_Construct_UClass_ACelestialSystem_NoRegister()
	{
		return ACelestialSystem::StaticClass();
	}
	struct Z_Construct_UClass_ACelestialSystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ACelestialSystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AAstroActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACelestialSystem_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "CelestialSystem.h" },
		{ "ModuleRelativePath", "CelestialSystem.h" },
	};
#endif
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_ACelestialSystem_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UAstonomicalSystem_NoRegister, (int32)VTABLE_OFFSET(ACelestialSystem, IAstonomicalSystem), false },  // 1538104280
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_ACelestialSystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ACelestialSystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ACelestialSystem_Statics::ClassParams = {
		&ACelestialSystem::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_ACelestialSystem_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ACelestialSystem_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ACelestialSystem()
	{
		if (!Z_Registration_Info_UClass_ACelestialSystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ACelestialSystem.OuterSingleton, Z_Construct_UClass_ACelestialSystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ACelestialSystem.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ACelestialSystem>()
	{
		return ACelestialSystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ACelestialSystem);
	ACelestialSystem::~ACelestialSystem() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialSystem_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialSystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ACelestialSystem, ACelestialSystem::StaticClass, TEXT("ACelestialSystem"), &Z_Registration_Info_UClass_ACelestialSystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ACelestialSystem), 1566747496U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialSystem_h_2343021763(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialSystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialSystem_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
