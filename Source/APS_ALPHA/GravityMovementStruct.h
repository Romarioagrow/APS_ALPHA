#pragma once

#include "CoreMinimal.h"
#include "GravityMovementStruct.generated.h"

USTRUCT(BlueprintType)
struct FGravityMovementStruct
{
	GENERATED_BODY()


    FGravityMovementStruct()
        : MovementsForceSpeedZeroG(0.05f)
        , MovementsForceSpeedStation(100.0f)
        , MovementsForceSpeedPlanet(100.0f)
        , MovementsForceSpeedShip(100.0f)
        , JumpForceSpeedZeroG(100.0f)
        , JumpForceSpeedStation(100.0f)
        , JumpForceSpeedPlanet(100.0f)
        , JumpForceSpeedShip(100.0f)
    {}

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - ZeroG")
        float MovementsForceSpeedZeroG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Station")
        float MovementsForceSpeedStation;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Planet")
        float MovementsForceSpeedPlanet;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Ship")
        float MovementsForceSpeedShip;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - ZeroG")
        float JumpForceSpeedZeroG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Station")
        float JumpForceSpeedStation;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Planet")
        float JumpForceSpeedPlanet;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Ship")
        float JumpForceSpeedShip;

   
};

