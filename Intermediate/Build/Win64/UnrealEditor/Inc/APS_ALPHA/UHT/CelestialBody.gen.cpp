// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/CelestialBody.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCelestialBody() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialBody();
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialBody_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstonomicalBody_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ACelestialBody::StaticRegisterNativesACelestialBody()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ACelestialBody);
	UClass* Z_Construct_UClass_ACelestialBody_NoRegister()
	{
		return ACelestialBody::StaticClass();
	}
	struct Z_Construct_UClass_ACelestialBody_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Radius_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Radius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Mass_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Mass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Age_MetaData[];
#endif
		static const UECodeGen_Private::FStrPropertyParams NewProp_Age;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AstroName_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_AstroName;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ACelestialBody_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AAstroActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACelestialBody_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "CelestialBody.h" },
		{ "ModuleRelativePath", "CelestialBody.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACelestialBody_Statics::NewProp_Radius_MetaData[] = {
		{ "Category", "Celestial Body" },
		{ "ModuleRelativePath", "CelestialBody.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_ACelestialBody_Statics::NewProp_Radius = { "Radius", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(ACelestialBody, Radius), METADATA_PARAMS(Z_Construct_UClass_ACelestialBody_Statics::NewProp_Radius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_ACelestialBody_Statics::NewProp_Radius_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACelestialBody_Statics::NewProp_Mass_MetaData[] = {
		{ "Category", "Celestial Body" },
		{ "ModuleRelativePath", "CelestialBody.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_ACelestialBody_Statics::NewProp_Mass = { "Mass", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(ACelestialBody, Mass), METADATA_PARAMS(Z_Construct_UClass_ACelestialBody_Statics::NewProp_Mass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_ACelestialBody_Statics::NewProp_Mass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACelestialBody_Statics::NewProp_Age_MetaData[] = {
		{ "Category", "Celestial Body" },
		{ "ModuleRelativePath", "CelestialBody.h" },
	};
#endif
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_ACelestialBody_Statics::NewProp_Age = { "Age", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(ACelestialBody, Age), METADATA_PARAMS(Z_Construct_UClass_ACelestialBody_Statics::NewProp_Age_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_ACelestialBody_Statics::NewProp_Age_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ACelestialBody_Statics::NewProp_AstroName_MetaData[] = {
		{ "Category", "Celestial Body" },
		{ "ModuleRelativePath", "CelestialBody.h" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_ACelestialBody_Statics::NewProp_AstroName = { "AstroName", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(ACelestialBody, AstroName), METADATA_PARAMS(Z_Construct_UClass_ACelestialBody_Statics::NewProp_AstroName_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_ACelestialBody_Statics::NewProp_AstroName_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ACelestialBody_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ACelestialBody_Statics::NewProp_Radius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ACelestialBody_Statics::NewProp_Mass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ACelestialBody_Statics::NewProp_Age,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ACelestialBody_Statics::NewProp_AstroName,
	};
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_ACelestialBody_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UAstonomicalBody_NoRegister, (int32)VTABLE_OFFSET(ACelestialBody, IAstonomicalBody), false },  // 4128211536
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_ACelestialBody_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ACelestialBody>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ACelestialBody_Statics::ClassParams = {
		&ACelestialBody::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_ACelestialBody_Statics::PropPointers,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_ACelestialBody_Statics::PropPointers),
		UE_ARRAY_COUNT(InterfaceParams),
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_ACelestialBody_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ACelestialBody_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ACelestialBody()
	{
		if (!Z_Registration_Info_UClass_ACelestialBody.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ACelestialBody.OuterSingleton, Z_Construct_UClass_ACelestialBody_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ACelestialBody.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ACelestialBody>()
	{
		return ACelestialBody::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ACelestialBody);
	ACelestialBody::~ACelestialBody() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBody_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBody_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ACelestialBody, ACelestialBody::StaticClass, TEXT("ACelestialBody"), &Z_Registration_Info_UClass_ACelestialBody, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ACelestialBody), 2550715138U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBody_h_2764905258(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBody_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CelestialBody_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
