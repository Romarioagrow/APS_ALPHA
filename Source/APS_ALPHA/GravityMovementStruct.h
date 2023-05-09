#pragma once

#include "CoreMinimal.h"
#include "GravityMovementStruct.generated.h"

USTRUCT(BlueprintType)
struct FGravityMovementStruct
{
	GENERATED_BODY()


    FGravityMovementStruct()
        : MovementsForceSpeedZeroG(25.f)
        , MovementsForceSpeedStation(1500.f)
        , MovementsForceSpeedPlanet(100.f)
        , MovementsForceSpeedShip(100.f)
        , MovementsForceSpeedLowG(250)
        , JumpForceSpeedZeroG(25.f)
        , JumpForceSpeedStation(40.f)
        , JumpForceSpeedPlanet(100.f)
        , JumpForceSpeedShip(100.f)
        , JumpForceSpeedLowG(10.f)
    {}

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - ZeroG")
        float MovementsForceSpeedZeroG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Station")
        float MovementsForceSpeedStation;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Planet")
        float MovementsForceSpeedPlanet;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Ship")
        float MovementsForceSpeedShip;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - LowG")
        float MovementsForceSpeedLowG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - ZeroG")
        float JumpForceSpeedZeroG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Station")
        float JumpForceSpeedStation;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Planet")
        float JumpForceSpeedPlanet;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Ship")
        float JumpForceSpeedShip;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - LowG")
        float JumpForceSpeedLowG;

   
};

