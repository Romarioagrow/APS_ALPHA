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
    return UEnum::GetValueAsString(FlightSystem.CurrentFlightType);
}

FString USpaceshipOnboardComputer::GetFlightModeAsString()
{
    return UEnum::GetValueAsString(FlightSystem.CurrentFlightMode);
}

FString USpaceshipOnboardComputer::GetFlightStatusAsString()
{
    return UEnum::GetValueAsString(FlightSystem.CurrentFlightStatus);
}

FString USpaceshipOnboardComputer::GetEngineStateAsString()
{
    return UEnum::GetValueAsString(EngineSystem.CurrentEngineState);
}

FString USpaceshipOnboardComputer::GetEngineTypeAsString()
{
    return UEnum::GetValueAsString(EngineSystem.CurrentEngineType);
}