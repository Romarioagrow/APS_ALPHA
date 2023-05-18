// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetarySystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetarySystem() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void APlanetarySystem::StaticRegisterNativesAPlanetarySystem()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(APlanetarySystem);
	UClass* Z_Construct_UClass_APlanetarySystem_NoRegister()
	{
		return APlanetarySystem::StaticClass();
	}
	struct Z_Construct_UClass_APlanetarySystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Star_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Star;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_APlanetarySystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialSystem,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "PlanetarySystem.h" },
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star_MetaData[] = {
		{ "ModuleRelativePath", "PlanetarySystem.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star = { "Star", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanetarySystem, Star), Z_Construct_UClass_AStar_NoRegister, METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_APlanetarySystem_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanetarySystem_Statics::NewProp_Star,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_APlanetarySystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<APlanetarySystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_APlanetarySystem_Statics::ClassParams = {
		&APlanetarySystem::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_APlanetarySystem_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_APlanetarySystem_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_APlanetarySystem_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_APlanetarySystem()
	{
		if (!Z_Registration_Info_UClass_APlanetarySystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_APlanetarySystem.OuterSingleton, Z_Construct_UClass_APlanetarySystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_APlanetarySystem.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<APlanetarySystem>()
	{
		return APlanetarySystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(APlanetarySystem);
	APlanetarySystem::~APlanetarySystem() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_APlanetarySystem, APlanetarySystem::StaticClass, TEXT("APlanetarySystem"), &Z_Registration_Info_UClass_APlanetarySystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(APlanetarySystem), 872953847U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_2877789545(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystem_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
