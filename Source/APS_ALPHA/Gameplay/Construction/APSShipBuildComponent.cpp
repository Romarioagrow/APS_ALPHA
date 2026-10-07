#include "APSShipBuildComponent.h"

#include "APSConstructionMode.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "APSConstruction"

namespace APSShipBuildLocal
{
	/** In orbit: within this many of its radii above the surface of a planet or moon. */
	constexpr double OrbitReachRadii = 4.0;
	/** The zone around the ship: orbital structures are tens to hundreds of metres. */
	constexpr double ZoneRadiusCm = 200000.0;
	constexpr float RotateStepDegrees = 15.f;
	constexpr float FineRotateStepDegrees = 5.f;

	/** 1-9, then 0 for the tenth card; INDEX_NONE for other keys. */
	int32 SlotOfKey(const FKey& Key)
	{
		const FKey SlotKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven,
			EKeys::Eight, EKeys::Nine, EKeys::Zero};
		for (int32 Slot = 0; Slot < static_cast<int32>(UE_ARRAY_COUNT(SlotKeys)); ++Slot)
		{
			if (SlotKeys[Slot] == Key)
			{
				return Slot;
			}
		}
		return INDEX_NONE;
	}

	/** The colony terminal or the strategic map holds the input with its own widget. */
	bool IsOtherScreenOpen(const UWorld* World, const APlayerController* PlayerController)
	{
		const UAPSColonyTerminalSubsystem* Terminal = World ? World->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
		const AGravityPlayerController* GravityController = Cast<AGravityPlayerController>(PlayerController);
		return (Terminal && Terminal->IsTerminalOpen()) || (GravityController && GravityController->IsStrategicMapOpen());
	}

	void ShowBuildCursor(APlayerController& PlayerController)
	{
		// Game and UI: the palette takes its clicks, the viewport keeps the keyboard (W/S still fly) and the world clicks.
		PlayerController.SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController.SetInputMode(InputMode);
	}
}

UAPSShipBuildComponent::UAPSShipBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

ASpaceship* UAPSShipBuildComponent::GetShip() const
{
	return Cast<ASpaceship>(GetOwner());
}

APlanetaryBody* UAPSShipBuildComponent::FindOrbitSite(FText* OutWhyNot) const
{
	using namespace APSShipBuildLocal;
	const ASpaceship* Ship = GetShip();
	UWorld* World = GetWorld();
	if (!Ship || !World)
	{
		return nullptr;
	}
	if (Ship->IsGroundVehicle())
	{
		if (OutWhyNot) *OutWhyNot = LOCTEXT("OrbitBuildVehicle", "NOT FROM A GROUND VEHICLE");
		return nullptr;
	}
	if (Ship->CurrentFlightEnvironment == EShipFlightEnvironment::Atmosphere
		|| Ship->CurrentFlightEnvironment == EShipFlightEnvironment::Surface)
	{
		if (OutWhyNot) *OutWhyNot = LOCTEXT("OrbitBuildAir", "LEAVE THE ATMOSPHERE FIRST");
		return nullptr;
	}
	const FVector Location = Ship->GetActorLocation();
	APlanetaryBody* Best = nullptr;
	double BestAltitude = TNumericLimits<double>::Max();
	for (TActorIterator<APlanetaryBody> It(World); It; ++It)
	{
		APlanetaryBody* Body = *It;
		const double Radius = IsValid(Body) ? Body->GetWorldScapeBodyRadiusCm() : 0.0;
		if (Radius <= 0.0)
		{
			continue;
		}
		const double Altitude = FVector::Dist(Location, Body->GetActorLocation()) - Radius;
		if (Altitude <= Radius * OrbitReachRadii && Altitude < BestAltitude)
		{
			Best = Body;
			BestAltitude = Altitude;
		}
	}
	if (!Best && OutWhyNot)
	{
		*OutWhyNot = LOCTEXT("OrbitBuildNoWorld", "IN ORBIT OF A PLANET OR MOON");
	}
	return Best;
}

FString UAPSShipBuildComponent::GetHintText() const
{
	const UWorld* World = GetWorld();
	if (Mode.IsValid() || !World)
	{
		return FString();
	}
	if (World->GetTimeSeconds() < RefusalUntilSeconds)
	{
		return RefusalText.ToString();
	}
	return FindOrbitSite() ? FString(TEXT("B ORBITAL BUILD")) : FString();
}

void UAPSShipBuildComponent::Toggle()
{
	if (Mode.IsValid())
	{
		Exit(true);
		return;
	}
	Enter();
}

void UAPSShipBuildComponent::Enter()
{
	using namespace APSShipBuildLocal;
	ASpaceship* Ship = GetShip();
	APlayerController* PlayerController = Ship ? Cast<APlayerController>(Ship->GetController()) : nullptr;
	UWorld* World = GetWorld();
	if (Mode.IsValid() || !Ship || !PlayerController || !World || !Ship->IsLocallyControlled()
		|| IsOtherScreenOpen(World, PlayerController))
	{
		return;
	}
	FText WhyNot;
	APlanetaryBody* Body = FindOrbitSite(&WhyNot);
	if (!Body)
	{
		RefusalText = FText::Format(LOCTEXT("OrbitBuildRefused", "ORBITAL BUILD: {0}"), WhyNot);
		RefusalUntilSeconds = World->GetTimeSeconds() + 3.0;
		UE_LOG(LogTemp, Log, TEXT("[APS.Construction] %s: orbital build refused: %s"), *Ship->GetName(), *WhyNot.ToString());
		return;
	}
	RefusalUntilSeconds = 0.0;
	Site = Body;

	APSConstruction::FFrame Frame;
	Frame.Site = Body;
	Frame.BuilderLocation = Ship->GetActorLocation();
	const FVector Radial = (Ship->GetActorLocation() - Body->GetActorLocation()).GetSafeNormal();
	Frame.BuilderUp = Radial.IsNearlyZero() ? FVector::UpVector : Radial;
	Frame.BuilderFeetCm = 0.0;
	Frame.RadiusCm = ZoneRadiusCm;
	Frame.Placement = APSConstruction::EPlacement::Orbit;
	Mode = MakeShared<FAPSConstructionMode>(Ship);
	Mode->Begin(Frame);

	// Its own keys, on a component of the ship: the controller stacks the pawn's extra input components above its own, so
	// these take the band keys 1-5 and 0, Q/E and the mouse buttons while building. The turn axes are held at zero: the
	// mouse is the cursor's now, and the ship keeps its turn (W/S still move it).
	if (UInputComponent* Input = NewObject<UInputComponent>(Ship))
	{
		const FKey PressedKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six,
			EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero, EKeys::Q, EKeys::E, EKeys::R, EKeys::C, EKeys::X,
			EKeys::Delete, EKeys::B, EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::MouseScrollUp,
			EKeys::MouseScrollDown};
		for (const FKey& PressedKey : PressedKeys)
		{
			Input->BindKey(PressedKey, IE_Pressed, this, &UAPSShipBuildComponent::HandleKey);
		}
		Input->BindKey(EKeys::Q, IE_Repeat, this, &UAPSShipBuildComponent::HandleKey);
		Input->BindKey(EKeys::E, IE_Repeat, this, &UAPSShipBuildComponent::HandleKey);
		Input->BindKey(EKeys::R, IE_Repeat, this, &UAPSShipBuildComponent::HandleKey);
		for (const TCHAR* Axis : {TEXT("ThrustYaw"), TEXT("ThrustPitch"), TEXT("ThrustRoll")})
		{
			Input->BindAxis(Axis, this, &UAPSShipBuildComponent::IgnoreAxis);
		}
		Input->RegisterComponent();
		BuildInput = Input;
	}
	ShowBuildCursor(*PlayerController);
	UE_LOG(LogTemp, Log, TEXT("[APS.Construction] %s enters orbital build mode at %s, %.0f km above it"), *Ship->GetName(),
		*GetNameSafe(Body), (FVector::Dist(Ship->GetActorLocation(), Body->GetActorLocation()) - Body->GetWorldScapeBodyRadiusCm())
			/ 100000.0);
}

void UAPSShipBuildComponent::Exit(const bool bRestoreInput)
{
	const TSharedPtr<FAPSConstructionMode> EndingMode = MoveTemp(Mode);
	Mode.Reset();
	if (EndingMode.IsValid())
	{
		EndingMode->End();
	}
	if (UInputComponent* Input = BuildInput.Get())
	{
		// Unbound at once (this may run inside one of its own key events, whose dispatch holds copies); then gone.
		Input->KeyBindings.Reset();
		Input->AxisBindings.Reset();
		Input->DestroyComponent();
	}
	BuildInput.Reset();
	Site.Reset();
	const ASpaceship* Ship = GetShip();
	APlayerController* PlayerController = Ship ? Cast<APlayerController>(Ship->GetController()) : nullptr;
	if (!PlayerController && GetWorld())
	{
		// The pilot left the ship while building (F): the walker's controller gets its input back.
		PlayerController = GetWorld()->GetFirstPlayerController();
	}
	if (bRestoreInput && PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void UAPSShipBuildComponent::HandleKey(const FKey Key)
{
	using namespace APSShipBuildLocal;
	// Held for the call: a key may end build mode.
	const TSharedPtr<FAPSConstructionMode> Building = Mode;
	if (!Building.IsValid())
	{
		return;
	}
	const ASpaceship* Ship = GetShip();
	const APlayerController* PlayerController = Ship ? Cast<APlayerController>(Ship->GetController()) : nullptr;
	const bool bShift = PlayerController
		&& (PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift));
	const float Step = bShift ? FineRotateStepDegrees : RotateStepDegrees;
	if (const int32 Slot = SlotOfKey(Key); Slot != INDEX_NONE)
	{
		Building->SelectSlot(Slot);
	}
	else if (Key == EKeys::Q)
	{
		Building->Rotate(-Step);
	}
	else if (Key == EKeys::E)
	{
		Building->Rotate(Step);
	}
	else if (Key == EKeys::R)
	{
		Building->RotatePitch(bShift ? -RotateStepDegrees : RotateStepDegrees);
	}
	else if (Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown)
	{
		Building->AdjustDistance(Key == EKeys::MouseScrollUp ? 1.0f : -1.0f);
	}
	else if (Key == EKeys::LeftMouseButton)
	{
		Building->Place();
	}
	else if (Key == EKeys::RightMouseButton)
	{
		Building->CancelSelection();
	}
	else if (Key == EKeys::C)
	{
		Building->ToggleSection();
	}
	else if (Key == EKeys::X || Key == EKeys::Delete)
	{
		Building->RemoveHovered();
	}
	else if (Key == EKeys::B)
	{
		Building->RequestExit();
	}
}

void UAPSShipBuildComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	using namespace APSShipBuildLocal;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Mode.IsValid())
	{
		return;
	}
	ASpaceship* Ship = GetShip();
	APlayerController* PlayerController = Ship ? Cast<APlayerController>(Ship->GetController()) : nullptr;
	FText WhyNot;
	APlanetaryBody* Body = FindOrbitSite(&WhyNot);
	// Another screen took the input (the strategic map, the terminal): leave quietly, it gives the input back on closing.
	const bool bOtherScreen = PlayerController && IsOtherScreenOpen(GetWorld(), PlayerController);
	if (!PlayerController || !Ship->IsLocallyControlled() || bOtherScreen || !Body || Mode->IsExitRequested())
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Construction] orbital build mode ends: %s"), !PlayerController ? TEXT("no pilot")
			: bOtherScreen ? TEXT("another screen took the input") : !Body ? *WhyNot.ToString() : TEXT("asked"));
		Exit(!bOtherScreen);
		return;
	}
	Site = Body;
	if (!PlayerController->ShouldShowMouseCursor())
	{
		// Something else hid the cursor (a screen closing in the same frame): build mode needs it.
		ShowBuildCursor(*PlayerController);
	}
	APSConstruction::FFrame Frame;
	Frame.Site = Body;
	Frame.BuilderLocation = Ship->GetActorLocation();
	const FVector Radial = (Ship->GetActorLocation() - Body->GetActorLocation()).GetSafeNormal();
	Frame.BuilderUp = Radial.IsNearlyZero() ? FVector::UpVector : Radial;
	Frame.BuilderFeetCm = 0.0;
	Frame.RadiusCm = ZoneRadiusCm;
	Frame.Placement = APSConstruction::EPlacement::Orbit;
	Mode->Tick(DeltaTime, Frame);
}

void UAPSShipBuildComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Mode.IsValid())
	{
		Exit(false);
	}
	Super::EndPlay(EndPlayReason);
}

#undef LOCTEXT_NAMESPACE
