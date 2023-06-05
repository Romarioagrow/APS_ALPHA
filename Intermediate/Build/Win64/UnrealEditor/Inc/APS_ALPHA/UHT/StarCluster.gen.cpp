// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarCluster.h"
#include "APS_ALPHA/StarGenerationModel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarCluster() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarCluster();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarCluster_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStellarClass();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FClusterParameters();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FRange();
	APS_ALPHA_API UScriptStruct* Z_Construct_UScriptStruct_FStarModel();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
	ENGINE_API UClass* Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_Range;
class UScriptStruct* FRange::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_Range.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_Range.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FRange, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("Range"));
	}
	return Z_Registration_Info_UScriptStruct_Range.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FRange>()
{
	return FRange::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FRange_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LowerBound_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_LowerBound;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_UpperBound_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_UpperBound;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FRange_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	void* Z_Construct_UScriptStruct_FRange_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FRange>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FRange_Statics::NewProp_LowerBound_MetaData[] = {
		{ "Category", "Range" },
		{ "ModuleRelativePath", "StarCluster.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FRange_Statics::NewProp_LowerBound = { "LowerBound", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FRange, LowerBound), METADATA_PARAMS(Z_Construct_UScriptStruct_FRange_Statics::NewProp_LowerBound_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRange_Statics::NewProp_LowerBound_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FRange_Statics::NewProp_UpperBound_MetaData[] = {
		{ "Category", "Range" },
		{ "ModuleRelativePath", "StarCluster.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FRange_Statics::NewProp_UpperBound = { "UpperBound", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FRange, UpperBound), METADATA_PARAMS(Z_Construct_UScriptStruct_FRange_Statics::NewProp_UpperBound_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRange_Statics::NewProp_UpperBound_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FRange_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRange_Statics::NewProp_LowerBound,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRange_Statics::NewProp_UpperBound,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FRange_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"Range",
		sizeof(FRange),
		alignof(FRange),
		Z_Construct_UScriptStruct_FRange_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRange_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FRange_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRange_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FRange()
	{
		if (!Z_Registration_Info_UScriptStruct_Range.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_Range.InnerSingleton, Z_Construct_UScriptStruct_FRange_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_Range.InnerSingleton;
	}
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_ClusterParameters;
class UScriptStruct* FClusterParameters::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_ClusterParameters.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_ClusterParameters.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FClusterParameters, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("ClusterParameters"));
	}
	return Z_Registration_Info_UScriptStruct_ClusterParameters.OuterSingleton;
}
template<> APS_ALPHA_API UScriptStruct* StaticStruct<FClusterParameters>()
{
	return FClusterParameters::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FClusterParameters_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarCountRange_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_StarCountRange;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarDensityRange_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_StarDensityRange;
		static const UECodeGen_Private::FStructPropertyParams NewProp_SpectralClassProbabilitiesRange_ValueProp;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SpectralClassProbabilitiesRange_Key_KeyProp_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SpectralClassProbabilitiesRange_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralClassProbabilitiesRange_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_SpectralClassProbabilitiesRange;
		static const UECodeGen_Private::FStructPropertyParams NewProp_StellarClassProbabilitiesRange_ValueProp;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StellarClassProbabilitiesRange_Key_KeyProp_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StellarClassProbabilitiesRange_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StellarClassProbabilitiesRange_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_StellarClassProbabilitiesRange;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FClusterParameters_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	void* Z_Construct_UScriptStruct_FClusterParameters_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FClusterParameters>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarCountRange_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarCountRange = { "StarCountRange", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FClusterParameters, StarCountRange), Z_Construct_UScriptStruct_FRange, METADATA_PARAMS(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarCountRange_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarCountRange_MetaData)) }; // 3882291279
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarDensityRange_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarDensityRange = { "StarDensityRange", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FClusterParameters, StarDensityRange), Z_Construct_UScriptStruct_FRange, METADATA_PARAMS(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarDensityRange_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarDensityRange_MetaData)) }; // 3882291279
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_ValueProp = { "SpectralClassProbabilitiesRange", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 1, Z_Construct_UScriptStruct_FRange, METADATA_PARAMS(nullptr, 0) }; // 3882291279
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_Key_KeyProp_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_Key_KeyProp = { "SpectralClassProbabilitiesRange_Key", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UEnum_APS_ALPHA_ESpectralClass, METADATA_PARAMS(nullptr, 0) }; // 1101356590
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange = { "SpectralClassProbabilitiesRange", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FClusterParameters, SpectralClassProbabilitiesRange), EMapPropertyFlags::None, METADATA_PARAMS(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_MetaData)) }; // 1101356590 3882291279
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_ValueProp = { "StellarClassProbabilitiesRange", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 1, Z_Construct_UScriptStruct_FRange, METADATA_PARAMS(nullptr, 0) }; // 3882291279
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_Key_KeyProp_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_Key_KeyProp = { "StellarClassProbabilitiesRange_Key", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UEnum_APS_ALPHA_EStellarClass, METADATA_PARAMS(nullptr, 0) }; // 3378852079
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange = { "StellarClassProbabilitiesRange", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(FClusterParameters, StellarClassProbabilitiesRange), EMapPropertyFlags::None, METADATA_PARAMS(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_MetaData)) }; // 3378852079 3882291279
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FClusterParameters_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarCountRange,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StarDensityRange,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_Key_KeyProp_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_SpectralClassProbabilitiesRange,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_Key_KeyProp_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FClusterParameters_Statics::NewProp_StellarClassProbabilitiesRange,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FClusterParameters_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		&NewStructOps,
		"ClusterParameters",
		sizeof(FClusterParameters),
		alignof(FClusterParameters),
		Z_Construct_UScriptStruct_FClusterParameters_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FClusterParameters_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FClusterParameters_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FClusterParameters_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FClusterParameters()
	{
		if (!Z_Registration_Info_UScriptStruct_ClusterParameters.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_ClusterParameters.InnerSingleton, Z_Construct_UScriptStruct_FClusterParameters_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_ClusterParameters.InnerSingleton;
	}
	DEFINE_FUNCTION(AStarCluster::execGenerateCluster)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->GenerateCluster();
		P_NATIVE_END;
	}
	void AStarCluster::StaticRegisterNativesAStarCluster()
	{
		UClass* Class = AStarCluster::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "GenerateCluster", &AStarCluster::execGenerateCluster },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_AStarCluster_GenerateCluster_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AStarCluster_GenerateCluster_Statics::Function_MetaDataParams[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AStarCluster_GenerateCluster_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AStarCluster, nullptr, "GenerateCluster", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AStarCluster_GenerateCluster_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AStarCluster_GenerateCluster_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AStarCluster_GenerateCluster()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AStarCluster_GenerateCluster_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AStarCluster);
	UClass* Z_Construct_UClass_AStarCluster_NoRegister()
	{
		return AStarCluster::StaticClass();
	}
	struct Z_Construct_UClass_AStarCluster_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Stars_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Stars_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_Stars;
		static const UECodeGen_Private::FStructPropertyParams NewProp_StarsModel_ValueProp;
		static const UECodeGen_Private::FStructPropertyParams NewProp_StarsModel_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarsModel_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_StarsModel;
		static const UECodeGen_Private::FBytePropertyParams NewProp_ClusterType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ClusterType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_ClusterType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarCount_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_StarCount;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarDensity_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_StarDensity;
		static const UECodeGen_Private::FFloatPropertyParams NewProp_SpectralClassProbabilities_ValueProp;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SpectralClassProbabilities_Key_KeyProp_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SpectralClassProbabilities_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralClassProbabilities_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_SpectralClassProbabilities;
		static const UECodeGen_Private::FFloatPropertyParams NewProp_StellarClassProbabilities_ValueProp;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StellarClassProbabilities_Key_KeyProp_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StellarClassProbabilities_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StellarClassProbabilities_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_StellarClassProbabilities;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarMeshInstances_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarMeshInstances;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ClusterBounds_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_ClusterBounds;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AStarCluster_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AAstroActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
	const FClassFunctionLinkInfo Z_Construct_UClass_AStarCluster_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_AStarCluster_GenerateCluster, "GenerateCluster" }, // 2757874109
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "StarCluster.h" },
		{ "ModuleRelativePath", "StarCluster.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_Stars_Inner = { "Stars", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UClass_AStar_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_Stars_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "ModuleRelativePath", "StarCluster.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_Stars = { "Stars", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, Stars), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_Stars_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_Stars_MetaData)) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel_ValueProp = { "StarsModel", nullptr, (EPropertyFlags)0x0000000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 1, Z_Construct_UScriptStruct_FStarModel, METADATA_PARAMS(nullptr, 0) }; // 491934795
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel_Key_KeyProp = { "StarsModel_Key", nullptr, (EPropertyFlags)0x0000000000020001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "ModuleRelativePath", "StarCluster.h" },
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel = { "StarsModel", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, StarsModel), EMapPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel_MetaData)) }; // 491934795
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterType_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterType = { "ClusterType", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, ClusterType), Z_Construct_UEnum_APS_ALPHA_EStarClusterType, METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterType_MetaData)) }; // 651664193
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_StarCount_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StarCount = { "StarCount", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, StarCount), METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarCount_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarCount_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_StarDensity_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StarDensity = { "StarDensity", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, StarDensity), METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarDensity_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarDensity_MetaData)) };
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_ValueProp = { "SpectralClassProbabilities", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 1, METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_Key_KeyProp_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_Key_KeyProp = { "SpectralClassProbabilities_Key", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UEnum_APS_ALPHA_ESpectralClass, METADATA_PARAMS(nullptr, 0) }; // 1101356590
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities = { "SpectralClassProbabilities", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, SpectralClassProbabilities), EMapPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_MetaData)) }; // 1101356590
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_ValueProp = { "StellarClassProbabilities", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 1, METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_Key_KeyProp_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_Key_KeyProp = { "StellarClassProbabilities_Key", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UEnum_APS_ALPHA_EStellarClass, METADATA_PARAMS(nullptr, 0) }; // 3378852079
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities = { "StellarClassProbabilities", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, StellarClassProbabilities), EMapPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_MetaData)) }; // 3378852079
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_StarMeshInstances_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "//// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = \"Star Cluster\")\n//    UStaticMesh* StarModel;\n// HISM \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "/ \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\nUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = \"Star Cluster\")\n    UStaticMesh* StarModel;\n HISM \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_StarMeshInstances = { "StarMeshInstances", nullptr, (EPropertyFlags)0x001000000008000d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, StarMeshInstances), Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarMeshInstances_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_StarMeshInstances_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterBounds_MetaData[] = {
		{ "Category", "Star Cluster" },
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "ModuleRelativePath", "StarCluster.h" },
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterBounds = { "ClusterBounds", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarCluster, ClusterBounds), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterBounds_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterBounds_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AStarCluster_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_Stars_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_Stars,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StarsModel,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StarCount,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StarDensity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_Key_KeyProp_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_SpectralClassProbabilities,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_Key_KeyProp_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StellarClassProbabilities,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_StarMeshInstances,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarCluster_Statics::NewProp_ClusterBounds,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AStarCluster_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AStarCluster>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AStarCluster_Statics::ClassParams = {
		&AStarCluster::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_AStarCluster_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AStarCluster_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AStarCluster_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AStarCluster()
	{
		if (!Z_Registration_Info_UClass_AStarCluster.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AStarCluster.OuterSingleton, Z_Construct_UClass_AStarCluster_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AStarCluster.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AStarCluster>()
	{
		return AStarCluster::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AStarCluster);
	AStarCluster::~AStarCluster() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_Statics::ScriptStructInfo[] = {
		{ FRange::StaticStruct, Z_Construct_UScriptStruct_FRange_Statics::NewStructOps, TEXT("Range"), &Z_Registration_Info_UScriptStruct_Range, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FRange), 3882291279U) },
		{ FClusterParameters::StaticStruct, Z_Construct_UScriptStruct_FClusterParameters_Statics::NewStructOps, TEXT("ClusterParameters"), &Z_Registration_Info_UScriptStruct_ClusterParameters, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FClusterParameters), 2845724219U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AStarCluster, AStarCluster::StaticClass, TEXT("AStarCluster"), &Z_Registration_Info_UClass_AStarCluster, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AStarCluster), 258501588U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_3899290272(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarCluster_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
