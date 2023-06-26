// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipOnboardComputer.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"
#include "CelestialBody.h"
#include "Planet.h"
#include "Star.h"
#include "Spaceship.h"
#include "StarCluster.h"

USpaceshipOnboardComputer::USpaceshipOnboardComputer(AActor* InOwner)
{
    Owner = InOwner;
}

USpaceshipOnboardComputer::USpaceshipOnboardComputer()
{
    //ASpaceship* Spaceship = Cast<ASpaceship>(GetOwner());
    //if (Spaceship)
    //{
    //    UStaticMeshComponent* SpaceshipHull = Spaceship->GetSpaceshipHull();
    //    // ��������� ����� �������� � SpaceshipHull
    //}
    //Owner = InOwner;
}

void USpaceshipOnboardComputer::ComputeFlightStatus(AWorldActor* AffectedActor)
{
    if (!AffectedActor) return;

    // ��������� ��� ������ � ���������� ��������������� ����� ������ � ��� ������
    if (AffectedActor->IsA(ACelestialBody::StaticClass()))
    {
        ACelestialBody* CelestialBody = Cast<ACelestialBody>(AffectedActor);

        // ��������� ���������� ������ � ����������� �� ���� ����
        if (CelestialBody->IsA(APlanet::StaticClass()))
        {
            FlightSystem.CurrentFlightMode = EFlightMode::Planetary;
            FlightSystem.CurrentFlightType = EFlightType::ZeroG;

            // check proximity!
        }
        else if (CelestialBody->IsA(AStar::StaticClass()))
        {
            FlightSystem.CurrentFlightMode = EFlightMode::Interplanetray;
            FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
            //EngineSystem.CurrentEngineMode = EEngineMode::Offset;
            SwitchEngineMode(EEngineMode::SpaceWrap);//InitiateOffsetMode();
            //SpaceshipHull->SetSimulatePhysics(false);

        }
        // �������� ������ �������, ���� ����������
    }
    else if (AffectedActor->IsA(ATechActor::StaticClass()))
    {
        // ����� �� ������ ��������� ��������� ������ ��� ����������� ��������
        FlightSystem.CurrentFlightMode = EFlightMode::Station;
        FlightSystem.CurrentFlightType = EFlightType::ArtificialGravity;

        SwitchEngineMode(EEngineMode::Impulse);
        //SpaceshipHull->SetSimulatePhysics(true);
        //EngineSystem.InitiateOffsetMode();

        //EngineSystem.CurrentEngineMode = EEngineMode::Impulse;
    }
    else if (AffectedActor->IsA(AStarCluster::StaticClass()))
    {
        // ����� �� ������ ��������� ��������� ������ ��� ����������� ��������
        FlightSystem.CurrentFlightMode = EFlightMode::Interstellar;
        FlightSystem.CurrentFlightType = EFlightType::FTL;

        SwitchEngineMode(EEngineMode::Offset);

        OffsetGalaxy = Cast<AAstroActor>(AffectedActor);

        //bIsRescaling = true;

        // � Tick
        

        //SpaceshipHull->SetSimulatePhysics(true);
        //EngineSystem.InitiateOffsetMode();

        //EngineSystem.CurrentEngineMode = EEngineMode::Impulse;
    }

    FFlightParams* Params = FlightModeParams.Find(FlightSystem.CurrentFlightMode);

    // ��������, ���� �� ������� ���������
    if (Params && !IsBoosting)
    {
        // ��������� ����������
        FlightSystem.FlightParams.ThrustForce = Params->ThrustForce;
        FlightSystem.FlightParams.LinearResistance = Params->LinearResistance;
        FlightSystem.FlightParams.AngularResistance = Params->AngularResistance;

    }

    //// ��������� ���� ��������� ������, ��������, � ����������� �� ���������� �� �������
    //float Distance = FVector::Dist(Actor->GetActorLocation(), ShipLocation); // ��������, ShipLocation - ��� ��������� �������
    //if (Distance < 1000) // ��������� ����������, �������� �� �������� ��������
    //{
    //    FlightRangeType = EFlightRangeType::Nearby;
    //}
    //else if (Distance < 10000)
    //{
    //    FlightRangeType = EFlightRangeType::Medium;
    //}
    //else
    //{
    //    FlightRangeType = EFlightRangeType::Distant;
    //}

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
    return UEnum::GetValueAsString(EngineSystem.CurrentEngineMode);
}

double USpaceshipOnboardComputer::GetEngineThrustForce()
{
    return FlightSystem.FlightParams.ThrustForce;
}

void USpaceshipOnboardComputer::IncreaseFlightMode()
{
    // ���������, ��� ������� FlightMode �� �������� ��������� � �����.
    if (static_cast<int32>(FlightSystem.CurrentFlightMode) < static_cast<int32>(EFlightMode::Intergalaxy))
    {
        FlightSystem.CurrentFlightSafeMode = EFlightSafeMode::Unsafe;

        // ����������� FlightMode �� 1.
        FlightSystem.CurrentFlightMode = static_cast<EFlightMode>(static_cast<int32>(FlightSystem.CurrentFlightMode) + 1);

        // ��������� ��������� ������ ��� ������ FlightMode.
        if (FlightModeParams.Contains(FlightSystem.CurrentFlightMode))
        {
            FlightSystem.FlightParams = FlightModeParams[FlightSystem.CurrentFlightMode];

            /// TODO: Switch to unsafe flight mode
        }
    }
}

void USpaceshipOnboardComputer::DecreaseFlightMode()
{
    // ���������, ��� ������� FlightMode �� �������� ������ � �����.
    if (static_cast<int32>(FlightSystem.CurrentFlightMode) > static_cast<int32>(EFlightMode::Station))
    {
        // ��������� FlightMode �� 1.
        FlightSystem.CurrentFlightMode = static_cast<EFlightMode>(static_cast<int32>(FlightSystem.CurrentFlightMode) - 1);

        // ��������� ��������� ������ ��� ������ FlightMode.
        if (FlightModeParams.Contains(FlightSystem.CurrentFlightMode))
        {
            FlightSystem.FlightParams = FlightModeParams[FlightSystem.CurrentFlightMode];
        }
    }
}

void USpaceshipOnboardComputer::AccelerateBoost(float DeltaTime)
{
    // ����������� ���� ���� �� ������������ �������. ����� � ��������� 10% � �������� �������.
    FlightSystem.FlightParams.ThrustForce += FlightSystem.FlightParams.ThrustForce * 1.1 * DeltaTime;
}

void USpaceshipOnboardComputer::DecelerateBoost(float DeltaTime)
{
    // ��������� ���� ���� �� ������������ �������. ����� � ��������� 10% � �������� �������.
    FlightSystem.FlightParams.ThrustForce -= FlightSystem.FlightParams.ThrustForce * 0.1 * DeltaTime;
}

void FEngineSystem::InitiateOffsetMode()
{
    CurrentEngineMode = EEngineMode::Offset;

}

void FEngineSystem::InitiateImpulseMode()
{
    CurrentEngineMode = EEngineMode::Impulse;

}

void USpaceshipOnboardComputer::SwitchEngineMode(EEngineMode EngineMode)
{
    EngineSystem.CurrentEngineMode = EngineMode;
    switch (EngineMode)
    {
    case EEngineMode::Impulse:
        CurrentMovementStrategy = MakeUnique<ImpulseMovementStrategy>(SpaceshipHull);
        SpaceshipHull->SetSimulatePhysics(true);
        break;
    case EEngineMode::Offset:
        CurrentMovementStrategy = MakeUnique<OffsetMovementStrategy>(SpaceshipHull);
        SpaceshipHull->SetSimulatePhysics(false);
        break;
        // �������� ������ ������� ��� ������ ����� ��������
    case EEngineMode::SpaceWrap:
        //CurrentMovementStrategy = MakeUnique<OffsetWrapMovementStrategy>(SpaceshipHull);
        CurrentMovementStrategy = MakeUnique<OffsetWrapMovementStrategy>(SpaceshipHull, OffsetSystem, 100000.0);
        SpaceshipHull->SetSimulatePhysics(false);
        break;
        // �������� ������ ������� ��� ������ ����� ��������
    }
}
