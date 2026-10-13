#include "APSStartSequenceSubsystem.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace APSStartSequence
{
	/** The generator frames the start in the frames after possession; seat the pilot once that has settled. */
	constexpr double SettleSeconds = 1.0;
	/** Generation can take a while; give up quietly if the pilot or the home ship never appear. */
	constexpr double TimeoutSeconds = 120.0;
	/**
	 * Rio 13.10 (ADRIFT: the pilot fell from orbit): until the seat takes him, the pilot is held at the seat with no
	 * velocity, so gravity cannot carry him off the hull while the start settles; a refused seat is asked again a few
	 * times before the hold lets go.
	 */
	constexpr int32 SeatAttempts = 5;
	constexpr double RetrySeconds = 1.5;

	void HoldAtSeat(ACustomGravityCharacter& Pilot, const ASpaceship& HomeShip)
	{
		const USceneComponent* Hatch = HomeShip.PilotExitPoint;
		const FVector Place = IsValid(Hatch) ? Hatch->GetComponentLocation() : HomeShip.GetActorLocation();
		Pilot.SetActorLocation(Place, false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Movement = Pilot.GetCharacterMovement())
		{
			Movement->Velocity = FVector::ZeroVector;
		}
	}
}

void UAPSStartSequenceSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMainGameplayInstance* GameplayState = GameInstance
		? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	// Only a new generated game that starts in the ship; menus, saves and other starts have nothing to do here.
	if (!GameplayState || !GameplayState->bSpawnGeneratedCivilization || GameplayState->bIsLoadingMode
		|| !GameplayState->SpawnParameters
		|| GameplayState->SpawnParameters->CharacterSpawnPlace != ECharSpawnPlace::SpaceShip)
	{
		bDone = true;
		return;
	}

	ACustomGravityCharacter* Pilot = Cast<ACustomGravityCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	ASpaceship* HomeShip = nullptr;
	for (TActorIterator<ASpaceship> It(World); It; ++It)
	{
		if (IsValid(*It) && It->ActorHasTag(TEXT("APS.Fleet.HomeShip")))
		{
			HomeShip = *It;
			break;
		}
	}
	const double Now = World->GetTimeSeconds();
	if (!IsValid(Pilot) || !IsValid(HomeShip))
	{
		// Rio 13.10: the generator's start may take longer than the cutoff; wait while its starter hierarchy is still owed.
		bool bGeneratorBusy = false;
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			bGeneratorBusy |= IsValid(*It) && !It->IsStarterHierarchySpawned();
		}
		if (Now > APSStartSequence::TimeoutSeconds && !bGeneratorBusy)
		{
			bDone = true;
			UE_LOG(LogTemp, Warning, TEXT("[APS.Start] ship start: no pilot or home ship after %.0f s; nothing seated"),
				APSStartSequence::TimeoutSeconds);
		}
		return;
	}
	if (HomeShip->HasPilot())
	{
		bDone = true;
		return;
	}
	FirstReadySeconds = FirstReadySeconds < 0.0 ? Now : FirstReadySeconds;
	// Held at the seat while the start settles and between attempts: nothing carries him off the hull.
	APSStartSequence::HoldAtSeat(*Pilot, *HomeShip);
	if (Now - FirstReadySeconds < APSStartSequence::SettleSeconds || Now < NextAttemptSeconds)
	{
		return;
	}
	++Attempts;
	NextAttemptSeconds = Now + APSStartSequence::RetrySeconds;
	const bool bCanSeat = HomeShip->CanRequestVehicleControl(Pilot);
	const bool bSeated = bCanSeat && HomeShip->RequestVehicleControl(Pilot);
	UE_LOG(LogTemp, Log, TEXT("[APS.Start] ship start (attempt %d): %s -> %s: %s"), Attempts, *GetNameSafe(Pilot), *GetNameSafe(HomeShip),
		bSeated ? TEXT("in the pilot seat") : (bCanSeat ? TEXT("control request refused") : TEXT("seat out of reach")));
	if (bSeated)
	{
		bDone = true;
		UAPSCivilizationJournalSubsystem::Post(this, TEXT("Start"), NSLOCTEXT("APSCivilizationJournal", "Seated",
			"The pilot takes the home ship's seat."));
	}
	else if (Attempts >= APSStartSequence::SeatAttempts)
	{
		bDone = true;
		UE_LOG(LogTemp, Warning, TEXT("[APS.Start] ship start: the seat refused %d times; the pilot stays on his feet at the ship"), Attempts);
	}
}

TStatId UAPSStartSequenceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSStartSequenceSubsystem, STATGROUP_Tickables);
}
