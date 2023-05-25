// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetaryProceduralGenerator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetaryProceduralGenerator() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseProceduralGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetarySystemGenerator();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetTypeProbability();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlanetTypeProbability;
class UScriptStruct* FPlanetTypeProbability::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlanetTypeProbability.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlanetTypeProbability.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlanetTypeProbability, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("PlanetTypeProbability"));
	}
	return Z_Registration_Info_UScriptStruct_PlanetTypeProbability.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FPlanetTypeProbability>()
{
	return FPlanetTypeProbability::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FBytePropertyParams NewProp_PlanetType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_PlanetType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Probability_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Probability;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "PlanetaryProceduralGenerator.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlanetTypeProbability>();
	}
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_PlanetType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_PlanetType_MetaData[] = {
		{ "Category", "Zone" },
		{ "ModuleRelativePath", "PlanetaryProceduralGenerator.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_PlanetType = { "PlanetType", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetTypeProbability, PlanetType), Z_Construct_UEnum_APS_ALPHA_EPlanetType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_PlanetType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_PlanetType_MetaData)) }; // 2129899352
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_Probability_MetaData[] = {
		{ "Category", "Zone" },
		{ "ModuleRelativePath", "PlanetaryProceduralGenerator.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_Probability = { "Probability", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetTypeProbability, Probability), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_Probability_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_Probability_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_PlanetType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_PlanetType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewProp_Probability,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"PlanetTypeProbability",
		sizeof(FPlanetTypeProbability),
		alignof(FPlanetTypeProbability),
		Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FPlanetTypeProbability()
	{
		if (!Z_Registration_Info_UScriptStruct_PlanetTypeProbability.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlanetTypeProbability.InnerSingleton, Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlanetTypeProbability.InnerSingleton;
	}
	void UPlanetarySystemGenerator::StaticRegisterNativesUPlanetarySystemGenerator()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPlanetarySystemGenerator);
	UClass* Z_Construct_UClass_UPlanetarySystemGenerator_NoRegister()
	{
		return UPlanetarySystemGenerator::StaticClass();
	}
	struct Z_Construct_UClass_UPlanetarySystemGenerator_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPlanetarySystemGenerator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseProceduralGenerator,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPlanetarySystemGenerator_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "PlanetaryProceduralGenerator.h" },
		{ "ModuleRelativePath", "PlanetaryProceduralGenerator.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPlanetarySystemGenerator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPlanetarySystemGenerator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPlanetarySystemGenerator_Statics::ClassParams = {
		&UPlanetarySystemGenerator::StaticClass,
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
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UPlanetarySystemGenerator_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UPlanetarySystemGenerator_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UPlanetarySystemGenerator()
	{
		if (!Z_Registration_Info_UClass_UPlanetarySystemGenerator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPlanetarySystemGenerator.OuterSingleton, Z_Construct_UClass_UPlanetarySystemGenerator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPlanetarySystemGenerator.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UPlanetarySystemGenerator>()
	{
		return UPlanetarySystemGenerator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPlanetarySystemGenerator);
	UPlanetarySystemGenerator::~UPlanetarySystemGenerator() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ScriptStructInfo[] = {
		{ FPlanetTypeProbability::StaticStruct, Z_Construct_UScriptStruct_FPlanetTypeProbability_Statics::NewStructOps, TEXT("PlanetTypeProbability"), &Z_Registration_Info_UScriptStruct_PlanetTypeProbability, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetTypeProbability), 593961349U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPlanetarySystemGenerator, UPlanetarySystemGenerator::StaticClass, TEXT("UPlanetarySystemGenerator"), &Z_Registration_Info_UClass_UPlanetarySystemGenerator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPlanetarySystemGenerator), 1692124698U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_1689476917(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetaryProceduralGenerator_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
