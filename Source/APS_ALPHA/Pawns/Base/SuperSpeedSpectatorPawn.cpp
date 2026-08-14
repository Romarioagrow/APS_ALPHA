#include "SuperSpeedSpectatorPawn.h"

#include "InputCoreTypes.h"

namespace
{
	constexpr float DefaultInputDeadzone = 0.05f;
	constexpr double DefaultTraversalSpeedUnitsPerSecond = 100000000.0;
	constexpr double DefaultMaximumTraversalSpeedUnitsPerSecond = 1000000000.0;
	constexpr float DefaultMaximumTraversalDeltaSeconds = 0.1f;

	static_assert(DefaultInputDeadzone > 0.0f && DefaultInputDeadzone < 1.0f);
	static_assert(DefaultTraversalSpeedUnitsPerSecond > 0.0);
	static_assert(DefaultMaximumTraversalSpeedUnitsPerSecond >= DefaultTraversalSpeedUnitsPerSecond);
	static_assert(DefaultMaximumTraversalDeltaSeconds > 0.0f);
}
ASuperSpeedSpectatorPawn::ASuperSpeedSpectatorPawn()
{
	// Определите здесь любые свойства, если это необходимо
}

void ASuperSpeedSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Привязка осевых входных событий
	PlayerInputComponent->BindAxis("MoveForward", this, &ASuperSpeedSpectatorPawn::SuperSpeedMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASuperSpeedSpectatorPawn::SuperSpeedMoveRight);
}




void ASuperSpeedSpectatorPawn::SuperSpeedMoveForward(float Val)
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	MoveCameraRelative(PlayerController->PlayerCameraManager->GetCameraRotation().Vector(), Val);
}

void ASuperSpeedSpectatorPawn::SuperSpeedMoveRight(float Val)
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	const FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
	MoveCameraRelative(FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Y), Val);
}

float ASuperSpeedSpectatorPawn::NormalizeTraversalInput(const float Value) const
{
	const float ClampedInput = FMath::Clamp(Value, -1.0f, 1.0f);
	const float SafeDeadzone = FMath::Clamp(InputDeadzone, 0.0f, 0.95f);
	const float Magnitude = FMath::Abs(ClampedInput);
	if (Magnitude <= SafeDeadzone)
	{
		return 0.0f;
	}

	return FMath::Sign(ClampedInput) * ((Magnitude - SafeDeadzone) / (1.0f - SafeDeadzone));
}

double ASuperSpeedSpectatorPawn::GetTraversalSpeedUnitsPerSecond() const
{
	const double MaximumSpeed = FMath::Clamp(MaximumTraversalSpeedUnitsPerSecond, 1.0, 1000000000.0);
	const double BaseSpeed = FMath::Clamp(TraversalSpeedUnitsPerSecond, 1.0, MaximumSpeed);
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	double RequestedSpeed = BaseSpeed;
	if (PlayerController && PlayerController->IsInputKeyDown(EKeys::LeftShift))
	{
		RequestedSpeed *= FMath::Clamp(BoostMultiplier, 1.0, 10.0);
	}
	return FMath::Min(RequestedSpeed, MaximumSpeed);
}

void ASuperSpeedSpectatorPawn::MoveCameraRelative(const FVector& Direction, const float RawInput)
{
	const float NormalizedInput = NormalizeTraversalInput(RawInput);
	if (FMath::IsNearlyZero(NormalizedInput) || Direction.IsNearlyZero() || !GetWorld())
	{
		return;
	}

	const float MaximumDeltaSeconds = FMath::Clamp(MaximumTraversalDeltaSeconds, 0.001f, 0.25f);
	const float DeltaSeconds = FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0.0f, MaximumDeltaSeconds);
	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	const double TravelDistance = GetTraversalSpeedUnitsPerSecond() * NormalizedInput * DeltaSeconds;
	SetActorLocation(GetActorLocation() + Direction.GetSafeNormal() * TravelDistance, false);
}
