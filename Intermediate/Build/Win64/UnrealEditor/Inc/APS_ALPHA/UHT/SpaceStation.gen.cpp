// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/SpaceStation.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSpaceStation() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceStation();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceStation_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ASpaceStation::StaticRegisterNativesASpaceStation()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ASpaceStation);
	UClass* Z_Construct_UClass_ASpaceStation_NoRegister()
	{
		return ASpaceStation::StaticClass();
	}
	struct Z_Construct_UClass_ASpaceStation_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ASpaceStation_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ATechActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASpaceStation_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "SpaceStation.h" },
		{ "ModuleRelativePath", "SpaceStation.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_ASpaceStation_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASpaceStation>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ASpaceStation_Statics::ClassParams = {
		&ASpaceStation::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_ASpaceStation_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ASpaceStation_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ASpaceStation()
	{
		if (!Z_Registration_Info_UClass_ASpaceStation.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASpaceStation.OuterSingleton, Z_Construct_UClass_ASpaceStation_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ASpaceStation.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ASpaceStation>()
	{
		return ASpaceStation::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ASpaceStation);
	ASpaceStation::~ASpaceStation() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceStation_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceStation_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ASpaceStation, ASpaceStation::StaticClass, TEXT("ASpaceStation"), &Z_Registration_Info_UClass_ASpaceStation, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASpaceStation), 3418993243U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceStation_h_2129360383(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceStation_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceStation_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
