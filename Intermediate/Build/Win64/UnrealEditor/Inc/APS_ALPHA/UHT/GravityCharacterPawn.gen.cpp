// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityCharacterPawn.h"
#include "Engine/Classes/Engine/HitResult.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityCharacterPawn() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AControlledPawn();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityCharacterPawn();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityCharacterPawn_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityPawn_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EAnimationState();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGravityState();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGravityType();
	ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UCameraComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UCapsuleComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UPrimitiveComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USceneComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USkeletalMeshComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USpringArmComponent_NoRegister();
	ENGINE_API UScriptStruct* Z_Construct_UScriptStruct_FHitResult();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	DEFINE_FUNCTION(AGravityCharacterPawn::execOnEndOverlap)
	{
		P_GET_OBJECT(UPrimitiveComponent,Z_Param_OverlappedComponent);
		P_GET_OBJECT(AActor,Z_Param_OtherActor);
		P_GET_OBJECT(UPrimitiveComponent,Z_Param_OtherComp);
		P_GET_PROPERTY(FIntProperty,Z_Param_OtherBodyIndex);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->OnEndOverlap(Z_Param_OverlappedComponent,Z_Param_OtherActor,Z_Param_OtherComp,Z_Param_OtherBodyIndex);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execOnBeginOverlap)
	{
		P_GET_OBJECT(UPrimitiveComponent,Z_Param_OverlappedComponent);
		P_GET_OBJECT(AActor,Z_Param_OtherActor);
		P_GET_OBJECT(UPrimitiveComponent,Z_Param_OtherComp);
		P_GET_PROPERTY(FIntProperty,Z_Param_OtherBodyIndex);
		P_GET_UBOOL(Z_Param_bFromSweep);
		P_GET_STRUCT_REF(FHitResult,Z_Param_Out_SweepResult);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->OnBeginOverlap(Z_Param_OverlappedComponent,Z_Param_OtherActor,Z_Param_OtherComp,Z_Param_OtherBodyIndex,Z_Param_bFromSweep,Z_Param_Out_SweepResult);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execAlignCharacterToCamera)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->AlignCharacterToCamera();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execRotateYaw)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RotateYaw(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execRotateRoll)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RotateRoll(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execRotatePitch)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RotatePitch(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execMoveUp)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->MoveUp(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execMoveRight)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->MoveRight(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execMoveForward)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->MoveForward(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execLookUp)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->LookUp(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execTurn)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->Turn(Z_Param_Value);
		P_NATIVE_END;
	}
	void AGravityCharacterPawn::StaticRegisterNativesAGravityCharacterPawn()
	{
		UClass* Class = AGravityCharacterPawn::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "AlignCharacterToCamera", &AGravityCharacterPawn::execAlignCharacterToCamera },
			{ "LookUp", &AGravityCharacterPawn::execLookUp },
			{ "MoveForward", &AGravityCharacterPawn::execMoveForward },
			{ "MoveRight", &AGravityCharacterPawn::execMoveRight },
			{ "MoveUp", &AGravityCharacterPawn::execMoveUp },
			{ "OnBeginOverlap", &AGravityCharacterPawn::execOnBeginOverlap },
			{ "OnEndOverlap", &AGravityCharacterPawn::execOnEndOverlap },
			{ "RotatePitch", &AGravityCharacterPawn::execRotatePitch },
			{ "RotateRoll", &AGravityCharacterPawn::execRotateRoll },
			{ "RotateYaw", &AGravityCharacterPawn::execRotateYaw },
			{ "Turn", &AGravityCharacterPawn::execTurn },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "AlignCharacterToCamera", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics
	{
		struct GravityCharacterPawn_eventLookUp_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventLookUp_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "LookUp", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::GravityCharacterPawn_eventLookUp_Parms), Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_LookUp()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics
	{
		struct GravityCharacterPawn_eventMoveForward_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventMoveForward_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "MoveForward", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::GravityCharacterPawn_eventMoveForward_Parms), Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_MoveForward()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics
	{
		struct GravityCharacterPawn_eventMoveRight_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventMoveRight_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "MoveRight", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::GravityCharacterPawn_eventMoveRight_Parms), Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_MoveRight()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics
	{
		struct GravityCharacterPawn_eventMoveUp_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventMoveUp_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "MoveUp", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::GravityCharacterPawn_eventMoveUp_Parms), Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_MoveUp()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics
	{
		struct GravityCharacterPawn_eventOnBeginOverlap_Parms
		{
			UPrimitiveComponent* OverlappedComponent;
			AActor* OtherActor;
			UPrimitiveComponent* OtherComp;
			int32 OtherBodyIndex;
			bool bFromSweep;
			FHitResult SweepResult;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OverlappedComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_OverlappedComponent;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_OtherActor;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OtherComp_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_OtherComp;
		static const UECodeGen_Private::FIntPropertyParams NewProp_OtherBodyIndex;
		static void NewProp_bFromSweep_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bFromSweep;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SweepResult_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SweepResult;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OverlappedComponent_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OverlappedComponent = { "OverlappedComponent", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnBeginOverlap_Parms, OverlappedComponent), Z_Construct_UClass_UPrimitiveComponent_NoRegister, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OverlappedComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OverlappedComponent_MetaData)) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherActor = { "OtherActor", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnBeginOverlap_Parms, OtherActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherComp_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherComp = { "OtherComp", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnBeginOverlap_Parms, OtherComp), Z_Construct_UClass_UPrimitiveComponent_NoRegister, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherComp_MetaData, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherComp_MetaData)) };
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherBodyIndex = { "OtherBodyIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnBeginOverlap_Parms, OtherBodyIndex), METADATA_PARAMS(nullptr, 0) };
	void Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_bFromSweep_SetBit(void* Obj)
	{
		((GravityCharacterPawn_eventOnBeginOverlap_Parms*)Obj)->bFromSweep = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_bFromSweep = { "bFromSweep", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, sizeof(bool), sizeof(GravityCharacterPawn_eventOnBeginOverlap_Parms), &Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_bFromSweep_SetBit, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_SweepResult_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_SweepResult = { "SweepResult", nullptr, (EPropertyFlags)0x0010008008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnBeginOverlap_Parms, SweepResult), Z_Construct_UScriptStruct_FHitResult, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_SweepResult_MetaData, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_SweepResult_MetaData)) }; // 1287526515
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OverlappedComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherActor,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherComp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_OtherBodyIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_bFromSweep,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::NewProp_SweepResult,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "OnBeginOverlap", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::GravityCharacterPawn_eventOnBeginOverlap_Parms), Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00480401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics
	{
		struct GravityCharacterPawn_eventOnEndOverlap_Parms
		{
			UPrimitiveComponent* OverlappedComponent;
			AActor* OtherActor;
			UPrimitiveComponent* OtherComp;
			int32 OtherBodyIndex;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OverlappedComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_OverlappedComponent;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_OtherActor;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OtherComp_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_OtherComp;
		static const UECodeGen_Private::FIntPropertyParams NewProp_OtherBodyIndex;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OverlappedComponent_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OverlappedComponent = { "OverlappedComponent", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnEndOverlap_Parms, OverlappedComponent), Z_Construct_UClass_UPrimitiveComponent_NoRegister, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OverlappedComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OverlappedComponent_MetaData)) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherActor = { "OtherActor", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnEndOverlap_Parms, OtherActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherComp_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherComp = { "OtherComp", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnEndOverlap_Parms, OtherComp), Z_Construct_UClass_UPrimitiveComponent_NoRegister, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherComp_MetaData, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherComp_MetaData)) };
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherBodyIndex = { "OtherBodyIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventOnEndOverlap_Parms, OtherBodyIndex), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OverlappedComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherActor,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherComp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::NewProp_OtherBodyIndex,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "OnEndOverlap", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::GravityCharacterPawn_eventOnEndOverlap_Parms), Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00080401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics
	{
		struct GravityCharacterPawn_eventRotatePitch_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventRotatePitch_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "RotatePitch", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::GravityCharacterPawn_eventRotatePitch_Parms), Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics
	{
		struct GravityCharacterPawn_eventRotateRoll_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventRotateRoll_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "RotateRoll", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::GravityCharacterPawn_eventRotateRoll_Parms), Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics
	{
		struct GravityCharacterPawn_eventRotateYaw_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventRotateYaw_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "RotateYaw", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::GravityCharacterPawn_eventRotateYaw_Parms), Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics
	{
		struct GravityCharacterPawn_eventTurn_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventTurn_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "Turn", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::GravityCharacterPawn_eventTurn_Parms), Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_Turn()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGravityCharacterPawn);
	UClass* Z_Construct_UClass_AGravityCharacterPawn_NoRegister()
	{
		return AGravityCharacterPawn::StaticClass();
	}
	struct Z_Construct_UClass_AGravityCharacterPawn_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RootSceneComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_RootSceneComponent;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CapsuleComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_CapsuleComponent;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MeshComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_MeshComponent;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CameraSpringArm_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_CameraSpringArm;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlayerCamera_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlayerCamera;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RotationInterpolationSpeed_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_RotationInterpolationSpeed;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CameraInterpolationSpeed_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_CameraInterpolationSpeed;
		static const UECodeGen_Private::FBytePropertyParams NewProp_CurrentGravityType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CurrentGravityType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_CurrentGravityType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_CurrentGravityState_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CurrentGravityState_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_CurrentGravityState;
		static const UECodeGen_Private::FBytePropertyParams NewProp_CurrentAnimationState_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CurrentAnimationState_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_CurrentAnimationState;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GravityTargetActor_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GravityTargetActor;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ForwardSpeed_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_ForwardSpeed;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RightSpeed_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_RightSpeed;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_UpSpeed_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_UpSpeed;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGravityCharacterPawn_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AControlledPawn,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
	const FClassFunctionLinkInfo Z_Construct_UClass_AGravityCharacterPawn_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_AGravityCharacterPawn_AlignCharacterToCamera, "AlignCharacterToCamera" }, // 621088259
		{ &Z_Construct_UFunction_AGravityCharacterPawn_LookUp, "LookUp" }, // 3537675169
		{ &Z_Construct_UFunction_AGravityCharacterPawn_MoveForward, "MoveForward" }, // 3752209854
		{ &Z_Construct_UFunction_AGravityCharacterPawn_MoveRight, "MoveRight" }, // 2636117551
		{ &Z_Construct_UFunction_AGravityCharacterPawn_MoveUp, "MoveUp" }, // 2822253874
		{ &Z_Construct_UFunction_AGravityCharacterPawn_OnBeginOverlap, "OnBeginOverlap" }, // 1920086188
		{ &Z_Construct_UFunction_AGravityCharacterPawn_OnEndOverlap, "OnEndOverlap" }, // 3981340622
		{ &Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch, "RotatePitch" }, // 2322532327
		{ &Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll, "RotateRoll" }, // 1331998239
		{ &Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw, "RotateYaw" }, // 2294616694
		{ &Z_Construct_UFunction_AGravityCharacterPawn_Turn, "Turn" }, // 693459909
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "GravityCharacterPawn.h" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent = { "RootSceneComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, RootSceneComponent), Z_Construct_UClass_USceneComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent = { "CapsuleComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CapsuleComponent), Z_Construct_UClass_UCapsuleComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent = { "MeshComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, MeshComponent), Z_Construct_UClass_USkeletalMeshComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm = { "CameraSpringArm", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CameraSpringArm), Z_Construct_UClass_USpringArmComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera = { "PlayerCamera", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, PlayerCamera), Z_Construct_UClass_UCameraComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RotationInterpolationSpeed_MetaData[] = {
		{ "Category", "Rotation" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RotationInterpolationSpeed = { "RotationInterpolationSpeed", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, RotationInterpolationSpeed), METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RotationInterpolationSpeed_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RotationInterpolationSpeed_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraInterpolationSpeed_MetaData[] = {
		{ "Category", "Camera" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraInterpolationSpeed = { "CameraInterpolationSpeed", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CameraInterpolationSpeed), METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraInterpolationSpeed_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraInterpolationSpeed_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityType_MetaData[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityType = { "CurrentGravityType", nullptr, (EPropertyFlags)0x0020080000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CurrentGravityType), Z_Construct_UEnum_APS_ALPHA_EGravityType, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityType_MetaData)) }; // 459840205
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityState_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityState_MetaData[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityState = { "CurrentGravityState", nullptr, (EPropertyFlags)0x0020080000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CurrentGravityState), Z_Construct_UEnum_APS_ALPHA_EGravityState, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityState_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityState_MetaData)) }; // 2783392099
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentAnimationState_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentAnimationState_MetaData[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentAnimationState = { "CurrentAnimationState", nullptr, (EPropertyFlags)0x0020080000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CurrentAnimationState), Z_Construct_UEnum_APS_ALPHA_EAnimationState, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentAnimationState_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentAnimationState_MetaData)) }; // 2413416102
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_GravityTargetActor_MetaData[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_GravityTargetActor = { "GravityTargetActor", nullptr, (EPropertyFlags)0x0020080000000015, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, GravityTargetActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_GravityTargetActor_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_GravityTargetActor_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ForwardSpeed_MetaData[] = {
		{ "Category", "Movement" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ForwardSpeed = { "ForwardSpeed", nullptr, (EPropertyFlags)0x0020080000000004, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, ForwardSpeed), METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ForwardSpeed_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ForwardSpeed_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RightSpeed_MetaData[] = {
		{ "Category", "Movement" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RightSpeed = { "RightSpeed", nullptr, (EPropertyFlags)0x0020080000000004, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, RightSpeed), METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RightSpeed_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RightSpeed_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_UpSpeed_MetaData[] = {
		{ "Category", "Movement" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_UpSpeed = { "UpSpeed", nullptr, (EPropertyFlags)0x0020080000000004, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, UpSpeed), METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_UpSpeed_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_UpSpeed_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AGravityCharacterPawn_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RotationInterpolationSpeed,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraInterpolationSpeed,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityState_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentGravityState,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentAnimationState_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CurrentAnimationState,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_GravityTargetActor,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ForwardSpeed,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RightSpeed,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_UpSpeed,
	};
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_AGravityCharacterPawn_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UGravityPawn_NoRegister, (int32)VTABLE_OFFSET(AGravityCharacterPawn, IGravityPawn), false },  // 3910067522
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGravityCharacterPawn_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGravityCharacterPawn>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGravityCharacterPawn_Statics::ClassParams = {
		&AGravityCharacterPawn::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_AGravityCharacterPawn_Statics::PropPointers,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::PropPointers),
		UE_ARRAY_COUNT(InterfaceParams),
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGravityCharacterPawn()
	{
		if (!Z_Registration_Info_UClass_AGravityCharacterPawn.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGravityCharacterPawn.OuterSingleton, Z_Construct_UClass_AGravityCharacterPawn_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGravityCharacterPawn.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGravityCharacterPawn>()
	{
		return AGravityCharacterPawn::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGravityCharacterPawn);
	AGravityCharacterPawn::~AGravityCharacterPawn() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGravityCharacterPawn, AGravityCharacterPawn::StaticClass, TEXT("AGravityCharacterPawn"), &Z_Registration_Info_UClass_AGravityCharacterPawn, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGravityCharacterPawn), 3447981366U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_1413958405(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
