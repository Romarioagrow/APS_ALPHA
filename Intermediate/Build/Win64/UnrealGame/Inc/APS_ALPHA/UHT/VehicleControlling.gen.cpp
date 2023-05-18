// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/VehicleControlling.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeVehicleControlling() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseInterface();
	APS_ALPHA_API UClass* Z_Construct_UClass_UVehicleControlling();
	APS_ALPHA_API UClass* Z_Construct_UClass_UVehicleControlling_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UVehicleControlling::StaticRegisterNativesUVehicleControlling()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UVehicleControlling);
	UClass* Z_Construct_UClass_UVehicleControlling_NoRegister()
	{
		return UVehicleControlling::StaticClass();
	}
	struct Z_Construct_UClass_UVehicleControlling_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UVehicleControlling_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UVehicleControlling_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "VehicleControlling.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UVehicleControlling_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IVehicleControlling>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UVehicleControlling_Statics::ClassParams = {
		&UVehicleControlling::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x000840A1u,
		METADATA_PARAMS(Z_Construct_UClass_UVehicleControlling_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UVehicleControlling_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UVehicleControlling()
	{
		if (!Z_Registration_Info_UClass_UVehicleControlling.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UVehicleControlling.OuterSingleton, Z_Construct_UClass_UVehicleControlling_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UVehicleControlling.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UVehicleControlling>()
	{
		return UVehicleControlling::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UVehicleControlling);
	UVehicleControlling::~UVehicleControlling() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_VehicleControlling_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_VehicleControlling_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UVehicleControlling, UVehicleControlling::StaticClass, TEXT("UVehicleControlling"), &Z_Registration_Info_UClass_UVehicleControlling, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UVehicleControlling), 1138385574U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_VehicleControlling_h_4010493674(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_VehicleControlling_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_VehicleControlling_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
