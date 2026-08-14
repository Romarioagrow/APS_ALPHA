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

	/** Ignore controller drift before applying high-speed spectator traversal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Traversal", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float InputDeadzone{0.05f};

	/** Normal spectator traversal speed in Unreal units (centimetres) per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Traversal", meta = (ClampMin = "1.0", ClampMax = "1000000000.0"))
	double TraversalSpeedUnitsPerSecond{100000000.0};

	/** Hold Left Shift to multiply traversal speed, capped by MaximumTraversalSpeedUnitsPerSecond. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Traversal", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	double BoostMultiplier{10.0};

	/** Hard speed ceiling prevents a Blueprint default from restoring per-sample mega-teleports. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Traversal", meta = (ClampMin = "1.0", ClampMax = "1000000000.0"))
	double MaximumTraversalSpeedUnitsPerSecond{1000000000.0};

	/** Clamp a hitch to a bounded traversal step. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spectator|Traversal", meta = (ClampMin = "0.001", ClampMax = "0.25"))
	float MaximumTraversalDeltaSeconds{0.1f};

	void SuperSpeedMoveForward(float Val);
	void SuperSpeedMoveRight(float Val);

private:
	float NormalizeTraversalInput(float Value) const;
	double GetTraversalSpeedUnitsPerSecond() const;
	void MoveCameraRelative(const FVector& Direction, float RawInput);
};
