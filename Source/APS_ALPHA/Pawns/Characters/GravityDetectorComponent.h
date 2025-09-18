// Components/GravityDetectorComponent.h

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GravityTypeEnum.h"
#include "Components/ActorComponent.h"
#include "GravityDetectorComponent.generated.h"

class ASpaceship;
class AWorldActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClosestGravityBodyChanged, AActor*, NewBody);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGravityPhysicsParamChanged);

UCLASS(ClassGroup=(Gravity), meta=(BlueprintSpawnableComponent))
class APS_ALPHA_API UGravityDetectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGravityDetectorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	AActor* GravityTargetActor;

	UPROPERTY(BlueprintAssignable, Category="Gravity|Events")
	FOnGravityPhysicsParamChanged OnGravityPhysicsParamChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Gravity|Events")
	FOnClosestGravityBodyChanged OnClosestGravityBodyChanged;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	EGravityType CurrentGravityType{
		EGravityType::ZeroG
	};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	ASpaceship* CurrentSpaceship;

	UFUNCTION(BlueprintCallable, Category="Gravity")
	void RunGravityCheck(ACharacter* Character);

	void SwitchGravityType(AActor* GravitySourceActor);
};
