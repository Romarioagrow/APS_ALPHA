// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipOnboardComputer.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"



USpaceshipOnboardComputer::USpaceshipOnboardComputer()
{
}

FString USpaceshipOnboardComputer::GetEnumValueAsString(const TCHAR* EnumName, int32 EnumValue)
{
    const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, EnumName, true);
    if (!EnumPtr)
    {
        return FString("Invalid");
    }

    FString EnumValueName = EnumPtr->GetNameByValue((int64)EnumValue).ToString();
    int32 DoubleColonIndex = EnumValueName.Find(TEXT("::"));

    if (DoubleColonIndex != INDEX_NONE)
    {
        EnumValueName = EnumValueName.Mid(DoubleColonIndex + 2);
    }

    return EnumValueName;
}

FString USpaceshipOnboardComputer::GetFlightTypeAsString()
{
    EFlightType FlightType = FlightSystem.CurrentFlightType;
    FString String = UEnum::GetValueAsString(FlightType);
    return String;
}

FString USpaceshipOnboardComputer::GetFlightModeAsString()
{
    EFlightMode FlightMode = FlightSystem.CurrentFlightMode;
    FString String = UEnum::GetValueAsString(FlightMode);
    return String;
}

FString USpaceshipOnboardComputer::GetFlightStatusAsString()
{
    EFlightStatus FlightStatus = FlightSystem.CurrentFlightStatus;
    FString String = UEnum::GetValueAsString(FlightStatus);
    return String;
}

// Допустим, у вас есть объект EngineSystem
FString USpaceshipOnboardComputer::GetEngineStateAsString()
{
    EEngineState EngineState = EngineSystem.CurrentEngineState;
    FString String = UEnum::GetValueAsString(EngineState);
    return String;
}

FString USpaceshipOnboardComputer::GetEngineTypeAsString()
{
    EEngineType EngineType = EngineSystem.CurrentEngineType;
    FString String = UEnum::GetValueAsString(EngineType);
    return String;
}