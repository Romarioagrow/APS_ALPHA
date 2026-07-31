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
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

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

	/** Full-scale source checks are throttled instead of scanning the world every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity", meta = (ClampMin = "0.02"))
	float DetectionInterval{0.1f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	bool bAutomaticDetection{true};

	UFUNCTION(BlueprintCallable, Category="Gravity")
	void RunGravityCheck(ACharacter* Character);

	void SwitchGravityType(AActor* GravitySourceActor);

	UFUNCTION(BlueprintPure, Category = "Gravity")
	FVector GetGravityDirectionAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void ClearGravitySource();

private:
	AActor* FindBestOverlappingSource(ACharacter* Character) const;
	AWorldActor* FindClosestFullScaleSource(ACharacter* Character) const;
};
