#include "IMovementStrategy.h"

#include "APS_ALPHA/Actors/Astro/StarSystem.h"
//#include "StarSystem.h" 
// ImpulseMovementStrategy.cpp
void ImpulseMovementStrategy::ThrustForward(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("ImpulseMovementStrategy::ThrustForward")));
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("SpaceshipHull, %s"), *SpaceshipHull->GetName()));

    /*const FVector Direction = ForwardVector->GetForwardVector();
    const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);*/

    const FVector Direction = SpaceshipHull->GetForwardVector();
    const FVector Impulse = Direction * Value * 100000000; // ���������� ���� ���� ����
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ImpulseMovementStrategy::ThrustSide(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetRightVector();
    const FVector Impulse = Direction * Value * 10000; // ���������� ���� ���� ����
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ImpulseMovementStrategy::ThrustVertical(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetUpVector();
    const FVector Impulse = Direction * Value * 10000; // ���������� ���� ���� ����
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}


// OffsetMovementStrategy.cpp
void OffsetMovementStrategy::ThrustForward(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetForwardVector();
    const FVector NewLocation = SpaceshipHull->GetComponentLocation() + Direction * Value * 10000; // ���������� ���� �������� ��������
    SpaceshipHull->SetWorldLocation(NewLocation, false);
}

void OffsetMovementStrategy::ThrustSide(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetRightVector();
    const FVector NewLocation = SpaceshipHull->GetComponentLocation() + Direction * Value * 10000; // ���������� ���� �������� ��������
    SpaceshipHull->SetWorldLocation(NewLocation, false);
}

void OffsetMovementStrategy::ThrustVertical(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetUpVector();
    const FVector NewLocation = SpaceshipHull->GetComponentLocation() + Direction * Value * 10000; // ���������� ���� �������� ��������
    SpaceshipHull->SetWorldLocation(NewLocation, false);
}
void OffsetWrapMovementStrategy::ThrustForward(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    // �������� ������ ������ �������
    const FVector Direction = SpaceshipHull->GetForwardVector();

    // �������� ������ �� StarSystem
    //AStarSystem* StarSystem = /* �������� ������ �� StarSystem � ����� ���� */

        // �������� StarSystem
    if (StarSystem)
    {
        StarSystem->AddActorWorldOffset(-Direction * Value * 1000000);

    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("NO StarSystem")));

    }
}

void OffsetWrapMovementStrategy::ThrustSide(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    // �������� ������ � ������� �������
    const FVector Direction = SpaceshipHull->GetRightVector();

    // �������� ������ �� StarSystem
   // AStarSystem* StarSystem = /* �������� ������ �� StarSystem � ����� ���� */
    if (StarSystem)
    {
        // �������� StarSystem
        StarSystem->AddActorLocalOffset(-Direction * Value);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("NO StarSystem")));

    }
}

void OffsetWrapMovementStrategy::ThrustVertical(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    // �������� ������ ����� �������
    const FVector Direction = SpaceshipHull->GetUpVector();

    // �������� ������ �� StarSystem
   // AStarSystem* StarSystem = /* �������� ������ �� StarSystem � ����� ���� */
    if (StarSystem)
    {
        // �������� StarSystem
        StarSystem->AddActorLocalOffset(-Direction * Value);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("NO StarSystem")));

    } 
}