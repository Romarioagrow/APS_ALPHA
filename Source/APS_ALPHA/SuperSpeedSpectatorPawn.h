#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "SuperSpeedSpectatorPawn.generated.h"

UCLASS()
class APS_ALPHA_API ASuperSpeedSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	ASuperSpeedSpectatorPawn();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void SuperSpeedMoveForward(float Val);
	void SuperSpeedMoveRight(float Val);
};
