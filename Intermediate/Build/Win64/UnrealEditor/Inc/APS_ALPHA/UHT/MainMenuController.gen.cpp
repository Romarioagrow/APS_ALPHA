// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/MainMenuController.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMainMenuController() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AMainMenuController();
	APS_ALPHA_API UClass* Z_Construct_UClass_AMainMenuController_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_APlayerController();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AMainMenuController::StaticRegisterNativesAMainMenuController()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AMainMenuController);
	UClass* Z_Construct_UClass_AMainMenuController_NoRegister()
	{
		return AMainMenuController::StaticClass();
	}
	struct Z_Construct_UClass_AMainMenuController_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AMainMenuController_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APlayerController,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AMainMenuController_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Collision Rendering Transformation" },
		{ "IncludePath", "MainMenuController.h" },
		{ "ModuleRelativePath", "MainMenuController.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AMainMenuController_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AMainMenuController>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AMainMenuController_Statics::ClassParams = {
		&AMainMenuController::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AMainMenuController_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AMainMenuController_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AMainMenuController()
	{
		if (!Z_Registration_Info_UClass_AMainMenuController.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AMainMenuController.OuterSingleton, Z_Construct_UClass_AMainMenuController_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AMainMenuController.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AMainMenuController>()
	{
		return AMainMenuController::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AMainMenuController);
	AMainMenuController::~AMainMenuController() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MainMenuController_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MainMenuController_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AMainMenuController, AMainMenuController::StaticClass, TEXT("AMainMenuController"), &Z_Registration_Info_UClass_AMainMenuController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AMainMenuController), 3443190922U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MainMenuController_h_3397733640(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MainMenuController_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_MainMenuController_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
