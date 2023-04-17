#pragma once

#include "CoreMinimal.h"
#include "GravityTypeEnum.generated.h"

UENUM(BlueprintType)
enum class EGravityType : uint8
{
    ZeroG,
    OnStation,
    OnPlanet,
    OnShip
};