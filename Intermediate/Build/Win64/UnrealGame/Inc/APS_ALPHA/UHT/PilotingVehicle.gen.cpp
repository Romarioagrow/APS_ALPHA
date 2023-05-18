// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PilotingVehicle.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePilotingVehicle() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AControlledPawn();
	APS_ALPHA_API UClass* Z_Construct_UClass_APilotingVehicle();
	APS_ALPHA_API UClass* Z_Construct_UClass_APilotingVehicle_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UVehicleControlling_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void APilotingVehicle::StaticRegisterNativesAPilotingVehicle()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(APilotingVehicle);
	UClass* Z_Construct_UClass_APilotingVehicle_NoRegister()
	{
		return APilotingVehicle::StaticClass();
	}
	struct Z_Construct_UClass_APilotingVehicle_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_APilotingVehicle_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AControlledPawn,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APilotingVehicle_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "PilotingVehicle.h" },
		{ "ModuleRelativePath", "PilotingVehicle.h" },
	};
#endif
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_APilotingVehicle_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UVehicleControlling_NoRegister, (int32)VTABLE_OFFSET(APilotingVehicle, IVehicleControlling), false },  // 1138385574
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_APilotingVehicle_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<APilotingVehicle>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_APilotingVehicle_Statics::ClassParams = {
		&APilotingVehicle::StaticClass,
		"Game",
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
		METADATA_PARAMS(Z_Construct_UClass_APilotingVehicle_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_APilotingVehicle_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_APilotingVehicle()
	{
		if (!Z_Registration_Info_UClass_APilotingVehicle.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_APilotingVehicle.OuterSingleton, Z_Construct_UClass_APilotingVehicle_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_APilotingVehicle.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<APilotingVehicle>()
	{
		return APilotingVehicle::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(APilotingVehicle);
	APilotingVehicle::~APilotingVehicle() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PilotingVehicle_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PilotingVehicle_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_APilotingVehicle, APilotingVehicle::StaticClass, TEXT("APilotingVehicle"), &Z_Registration_Info_UClass_APilotingVehicle, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(APilotingVehicle), 1307076047U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PilotingVehicle_h_325852419(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PilotingVehicle_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PilotingVehicle_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
