// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/SuperSpeedSpectatorPawn.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSuperSpeedSpectatorPawn() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ASuperSpeedSpectatorPawn();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASuperSpeedSpectatorPawn_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_ASpectatorPawn();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ASuperSpeedSpectatorPawn::StaticRegisterNativesASuperSpeedSpectatorPawn()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ASuperSpeedSpectatorPawn);
	UClass* Z_Construct_UClass_ASuperSpeedSpectatorPawn_NoRegister()
	{
		return ASuperSpeedSpectatorPawn::StaticClass();
	}
	struct Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ASpectatorPawn,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "SuperSpeedSpectatorPawn.h" },
		{ "ModuleRelativePath", "SuperSpeedSpectatorPawn.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASuperSpeedSpectatorPawn>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics::ClassParams = {
		&ASuperSpeedSpectatorPawn::StaticClass,
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
		0x009002A4u,
		METADATA_PARAMS(Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ASuperSpeedSpectatorPawn()
	{
		if (!Z_Registration_Info_UClass_ASuperSpeedSpectatorPawn.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASuperSpeedSpectatorPawn.OuterSingleton, Z_Construct_UClass_ASuperSpeedSpectatorPawn_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ASuperSpeedSpectatorPawn.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ASuperSpeedSpectatorPawn>()
	{
		return ASuperSpeedSpectatorPawn::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ASuperSpeedSpectatorPawn);
	ASuperSpeedSpectatorPawn::~ASuperSpeedSpectatorPawn() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SuperSpeedSpectatorPawn_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SuperSpeedSpectatorPawn_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ASuperSpeedSpectatorPawn, ASuperSpeedSpectatorPawn::StaticClass, TEXT("ASuperSpeedSpectatorPawn"), &Z_Registration_Info_UClass_ASuperSpeedSpectatorPawn, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASuperSpeedSpectatorPawn), 3909069706U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SuperSpeedSpectatorPawn_h_4174227043(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SuperSpeedSpectatorPawn_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SuperSpeedSpectatorPawn_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
