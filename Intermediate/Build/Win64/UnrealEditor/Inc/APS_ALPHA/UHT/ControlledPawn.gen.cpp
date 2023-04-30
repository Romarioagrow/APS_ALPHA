// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/ControlledPawn.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeControlledPawn() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AControlledPawn();
	APS_ALPHA_API UClass* Z_Construct_UClass_AControlledPawn_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_APawn();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AControlledPawn::StaticRegisterNativesAControlledPawn()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AControlledPawn);
	UClass* Z_Construct_UClass_AControlledPawn_NoRegister()
	{
		return AControlledPawn::StaticClass();
	}
	struct Z_Construct_UClass_AControlledPawn_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AControlledPawn_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APawn,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AControlledPawn_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "ControlledPawn.h" },
		{ "ModuleRelativePath", "ControlledPawn.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AControlledPawn_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AControlledPawn>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AControlledPawn_Statics::ClassParams = {
		&AControlledPawn::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AControlledPawn_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AControlledPawn_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AControlledPawn()
	{
		if (!Z_Registration_Info_UClass_AControlledPawn.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AControlledPawn.OuterSingleton, Z_Construct_UClass_AControlledPawn_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AControlledPawn.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AControlledPawn>()
	{
		return AControlledPawn::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AControlledPawn);
	AControlledPawn::~AControlledPawn() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_ControlledPawn_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_ControlledPawn_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AControlledPawn, AControlledPawn::StaticClass, TEXT("AControlledPawn"), &Z_Registration_Info_UClass_AControlledPawn, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AControlledPawn), 3217009974U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_ControlledPawn_h_4096567539(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_ControlledPawn_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_ControlledPawn_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
