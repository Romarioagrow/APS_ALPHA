#include "APSStartSequenceSubsystem.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace APSStartSequence
{
	/** The generator frames the start in the frames after possession; seat the pilot once that has settled. */
	constexpr double SettleSeconds = 1.0;
	/** Generation can take a while; give up quietly if the pilot or the home ship never appear. */
	constexpr double TimeoutSeconds = 120.0;
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
		if (Now > APSStartSequence::TimeoutSeconds)
		{
			bDone = true;
			UE_LOG(LogTemp, Warning, TEXT("[APS.Start] ship start: no pilot or home ship after %.0f s; nothing seated"),
				APSStartSequence::TimeoutSeconds);
		}
		return;
	}
	FirstReadySeconds = FirstReadySeconds < 0.0 ? Now : FirstReadySeconds;
	if (Now - FirstReadySeconds < APSStartSequence::SettleSeconds)
	{
		return;
	}
	bDone = true;
	if (HomeShip->HasPilot())
	{
		return;
	}
	const bool bCanSeat = HomeShip->CanRequestVehicleControl(Pilot);
	const bool bSeated = bCanSeat && HomeShip->RequestVehicleControl(Pilot);
	UE_LOG(LogTemp, Log, TEXT("[APS.Start] ship start: %s -> %s: %s"), *GetNameSafe(Pilot), *GetNameSafe(HomeShip),
		bSeated ? TEXT("in the pilot seat") : (bCanSeat ? TEXT("control request refused") : TEXT("seat out of reach")));
	if (bSeated)
	{
		UAPSCivilizationJournalSubsystem::Post(this, TEXT("Start"), NSLOCTEXT("APSCivilizationJournal", "Seated",
			"The pilot takes the home ship's seat."));
	}
}

TStatId UAPSStartSequenceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSStartSequenceSubsystem, STATGROUP_Tickables);
}
