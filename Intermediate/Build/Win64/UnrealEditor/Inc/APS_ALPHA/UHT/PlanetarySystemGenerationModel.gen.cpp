// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/PlanetarySystemGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanetarySystemGenerationModel() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FGenerationModel();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References

static_assert(std::is_polymorphic<FPlanetarySystemGenerationModel>() == std::is_polymorphic<FGenerationModel>(), "USTRUCT FPlanetarySystemGenerationModel cannot be polymorphic unless super FGenerationModel is polymorphic");

	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel;
class UScriptStruct* FPlanetarySystemGenerationModel::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("PlanetarySystemGenerationModel"));
	}
	return Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FPlanetarySystemGenerationModel>()
{
	return FPlanetarySystemGenerationModel::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AmountOfPlanets_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_AmountOfPlanets;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DistanceBetweenPlanets_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_DistanceBetweenPlanets;
		static const UECodeGen_Private::FBytePropertyParams NewProp_PlanetarySystemType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetarySystemType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_PlanetarySystemType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_OrbitDistributionType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OrbitDistributionType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_OrbitDistributionType;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Comment", "//USTRUCT(BlueprintType)\n//struct FZoneRadius\n//{\n//\x09GENERATED_USTRUCT_BODY()\n//\n//\x09\x09UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"Zone\")\n//\x09\x09""double InnerRadius;\n//\n//\x09UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"Zone\")\n//\x09\x09""double OuterRadius;\n//\n//\x09""FZoneRadius()\n//\x09{\n//\x09\x09InnerRadius = 0.0;\n//\x09\x09OuterRadius = 0.0;\n//\x09}\n//};\n" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
		{ "ToolTip", "USTRUCT(BlueprintType)\nstruct FZoneRadius\n{\n       GENERATED_USTRUCT_BODY()\n\n               UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"Zone\")\n               double InnerRadius;\n\n       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"Zone\")\n               double OuterRadius;\n\n       FZoneRadius()\n       {\n               InnerRadius = 0.0;\n               OuterRadius = 0.0;\n       }\n};" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlanetarySystemGenerationModel>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets_MetaData[] = {
		{ "Category", "Planetary System" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets = { "AmountOfPlanets", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemGenerationModel, AmountOfPlanets), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets_MetaData[] = {
		{ "Category", "Planetary System" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets = { "DistanceBetweenPlanets", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemGenerationModel, DistanceBetweenPlanets), METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_PlanetarySystemType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_PlanetarySystemType_MetaData[] = {
		{ "Category", "PlanetarySystemGenerationModel" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_PlanetarySystemType = { "PlanetarySystemType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemGenerationModel, PlanetarySystemType), Z_Construct_UEnum_APS_ALPHA_EPlanetarySystemType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_PlanetarySystemType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_PlanetarySystemType_MetaData)) }; // 1543107703
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_OrbitDistributionType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_OrbitDistributionType_MetaData[] = {
		{ "Category", "PlanetarySystemGenerationModel" },
		{ "ModuleRelativePath", "PlanetarySystemGenerationModel.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_OrbitDistributionType = { "OrbitDistributionType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FPlanetarySystemGenerationModel, OrbitDistributionType), Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType, METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_OrbitDistributionType_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_OrbitDistributionType_MetaData)) }; // 2085035236
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_AmountOfPlanets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_DistanceBetweenPlanets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_PlanetarySystemType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_PlanetarySystemType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_OrbitDistributionType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewProp_OrbitDistributionType,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		Z_Construct_UScriptStruct_FGenerationModel,
		&NewStructOps,
		"PlanetarySystemGenerationModel",
		sizeof(FPlanetarySystemGenerationModel),
		alignof(FPlanetarySystemGenerationModel),
		Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel()
	{
		if (!Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.InnerSingleton, Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo[] = {
		{ FPlanetarySystemGenerationModel::StaticStruct, Z_Construct_UScriptStruct_FPlanetarySystemGenerationModel_Statics::NewStructOps, TEXT("PlanetarySystemGenerationModel"), &Z_Registration_Info_UScriptStruct_PlanetarySystemGenerationModel, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlanetarySystemGenerationModel), 943300335U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_3742591571(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_PlanetarySystemGenerationModel_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
