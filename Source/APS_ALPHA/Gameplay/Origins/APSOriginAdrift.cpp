#include "APSOriginAdrift.h"

#include "APSOriginArk.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Actors/Tech/TechInfrastructure.h"
#include "APS_ALPHA/Core/Audio/APSAudioSubsystem.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/APSWorldRules.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Origins/APSOriginQuests.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Colony/APSMissionTracker.h"
#include "Components/LocalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "APSOriginAdrift"

namespace APSOriginAdriftLocal
{
	TAutoConsoleVariable<float> CVarDrain(TEXT("aps.Origin.AdriftDrain"), 1.0f,
		TEXT("ADRIFT: scales how fast flight burns the reactor (cruise ~0.2 %/s, orbital ~0.03 %/s at 1). 0: no drain."));
	TAutoConsoleVariable<float> CVarDryCruise(TEXT("aps.Origin.AdriftDryCruise"), 0.35f,
		TEXT("ADRIFT: the cruise band's speed with the reactor empty, as a fraction of its own (the ship limps, never stranded)."));
	TAutoConsoleVariable<float> CVarHoldSeconds(TEXT("aps.Origin.AdriftHoldSeconds"), 6.0f,
		TEXT("ADRIFT: seconds held slow beside a wreck (its cells) or the dead station (its wake)."));
	TAutoConsoleVariable<FString> CVarWreckShip(TEXT("aps.Origin.AdriftWreckShip"),
		TEXT("/Game/APS/APS_ALPHA/Core/Spaceships/S/BP_Spaceship_S_P3_01.BP_Spaceship_S_P3_01_C"),
		TEXT("ADRIFT: the hull the expedition before ours left drifting (two of them)."));

	const FName WreckTag(TEXT("APS.Origin.Derelict"));
	const FName StationTag(TEXT("APS.Origin.DeadStation"));
	const FName InitToken(TEXT("ORIGIN_ADRIFT_INIT"));
	const FName StationToken(TEXT("ORIGIN_ADRIFT_STATION"));
	const FName SalvageToken(TEXT("ORIGIN_ADRIFT_SALVAGE"));
	const FName HomedToken(TEXT("ORIGIN_ADRIFT_HOMED"));
	constexpr int32 WreckCount = 2;
	constexpr float StartReactor = 11.0f;
	constexpr float WreckCells = 25.0f;
	constexpr double WreckReachCm = 400000.0;
	constexpr double StationReachCm = 600000.0;
	constexpr double SlowCmPerSecond = 30000.0;

	FName WreckToken(const int32 Index)
	{
		return FName(*FString::Printf(TEXT("ORIGIN_ADRIFT_WRECK_%d"), Index));
	}

	struct FSight
	{
		TWeakObjectPtr<AActor> Body;
		TWeakObjectPtr<AActor> Marker;
		TArray<TWeakObjectPtr<ULocalLightComponent>> Lights;
		float HeldSeconds{0.0f};
	};

	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		bool bSpawned{false};
		FSight Wrecks[WreckCount];
		FSight Station;
		TWeakObjectPtr<ASpaceship> DarkCabin;
		TArray<TWeakObjectPtr<ULocalLightComponent>> CabinLights;
		FString PinnedStep;
	};

	TArray<FState>& States()
	{
		static TArray<FState> Entries;
		return Entries;
	}

	FState& StateFor(UWorld* World)
	{
		for (FState& Entry : States())
		{
			if (Entry.World.Get() == World)
			{
				return Entry;
			}
		}
		States().RemoveAll([](const FState& Entry) { return !Entry.World.IsValid(); });
		FState& Added = States().AddDefaulted_GetRef();
		Added.World = World;
		return Added;
	}

	void SetInGameName(AActor* Actor, const FText& Name)
	{
		// The world actor keeps its display name protected; a reflected property (as the Ancients' sites name theirs).
		if (FTextProperty* NameProperty = Actor ? FindFProperty<FTextProperty>(Actor->GetClass(), TEXT("InGameName")) : nullptr)
		{
			NameProperty->SetPropertyValue_InContainer(Actor, Name);
		}
	}

	/** Every visible local light of an actor goes dark (kept to give back); a few nearest Near stay at Fraction. */
	void Darken(AActor& Actor, TArray<TWeakObjectPtr<ULocalLightComponent>>& OutLights)
	{
		OutLights.Reset();
		TArray<ULocalLightComponent*> Lights;
		Actor.GetComponents<ULocalLightComponent>(Lights, true);
		for (ULocalLightComponent* Light : Lights)
		{
			if (IsValid(Light) && Light->IsVisible())
			{
				Light->SetVisibility(false);
				OutLights.Add(Light);
			}
		}
	}

	void Restore(TArray<TWeakObjectPtr<ULocalLightComponent>>& Lights)
	{
		for (const TWeakObjectPtr<ULocalLightComponent>& Light : Lights)
		{
			if (Light.IsValid())
			{
				Light->SetVisibility(true);
			}
		}
		Lights.Reset();
	}

	AAstroGenerator* Generator(UWorld* World)
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsValid(*It) && IsValid(It->HomePlanet) && IsValid(It->HomeStar))
			{
				return *It;
			}
		}
		return nullptr;
	}

	ASpaceship* PilotedShip(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		ASpaceship* Ship = Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr;
		return Ship && !Ship->IsGroundVehicle() ? Ship : nullptr;
	}

	void Notice(UWorld* World, const FText& Title, const FText& Body, const FText& Next)
	{
		UAPSCivilizationJournalSubsystem::Post(World, TEXT("Origin"), FText::Format(LOCTEXT("JournalLine", "{0}: {1}"), Title, Body));
		APSMissionTracker::PushNotice(World, LOCTEXT("Kicker", "ADRIFT  ·  THE LONG NIGHT"), Title, Body, Next, FLinearColor(0.96f, 0.64f, 0.18f));
	}

	uint32 SeedOf(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
		return Gameplay && Gameplay->NewGeneratedWorld ? static_cast<uint32>(Gameplay->NewGeneratedWorld->GenerationSeed) : 42002u;
	}

	/** The wrecks in the outer system (beside the two worlds nearest the start) and the dead station over the living world. */
	void SpawnSights(UWorld* World, FState& State, AAstroGenerator& Gen, FAPSMissionBoard& Board)
	{
		APlanet* Home = Gen.HomePlanet;
		AStar* Star = Gen.HomeStar;
		const FRandomStream Stream(static_cast<int32>(SeedOf(World) * 2654435761u + 7u));
		TArray<APlanet*> Others;
		for (APlanet* Planet : Star->Planets)
		{
			if (IsValid(Planet) && Planet != Home)
			{
				Others.Add(Planet);
			}
		}
		// Outermost first: the start lies beyond them.
		Others.Sort([Star](const APlanet& A, const APlanet& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), Star->GetActorLocation()) > FVector::DistSquared(B.GetActorLocation(), Star->GetActorLocation());
		});
		UClass* WreckClass = LoadClass<ASpaceship>(nullptr, *CVarWreckShip.GetValueOnGameThread());
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		for (int32 Index = 0; Index < WreckCount && WreckClass && Others.IsValidIndex(Index); ++Index)
		{
			APlanet* Near = Others[Index];
			const double Radius = FMath::Max(Near->GetWorldScapeBodyRadiusCm(), 100000.0);
			const FVector Direction = Stream.GetUnitVector();
			const FVector Place = Near->GetActorLocation() + Direction * (Radius * 1.6 + 2000000.0);
			const FRotator Tumble(Stream.FRandRange(-60.0, 60.0), Stream.FRandRange(0.0, 360.0), Stream.FRandRange(-120.0, 120.0));
			ASpaceship* Wreck = World->SpawnActor<ASpaceship>(WreckClass, Place, Tumble, Params);
			if (!IsValid(Wreck))
			{
				continue;
			}
			Wreck->Tags.AddUnique(WreckTag);
			Wreck->AttachToActor(Near, FAttachmentTransformRules::KeepWorldTransform);
			ATechInfrastructure* Marker = World->SpawnActor<ATechInfrastructure>(ATechInfrastructure::StaticClass(), Place, FRotator::ZeroRotator, Params);
			if (IsValid(Marker) && !Marker->GetRootComponent())
			{
				// adrift-t1: a bare marker had no root, stood at the world's origin (0 m away) and could not ride the wreck.
				USceneComponent* Root = NewObject<USceneComponent>(Marker, TEXT("WreckMarkerRoot"));
				Marker->SetRootComponent(Root);
				Root->RegisterComponent();
				Marker->SetActorLocation(Place, false, nullptr, ETeleportType::TeleportPhysics);
			}
			if (IsValid(Marker))
			{
				Marker->Tags.AddUnique(WreckTag);
				Marker->AttachToActor(Wreck, FAttachmentTransformRules::KeepWorldTransform);
			}
			const bool bStripped = Board.IsUnlocked(WreckToken(Index));
			SetInGameName(Marker, bStripped ? LOCTEXT("WreckStripped", "STRIPPED WRECK") : FText::Format(LOCTEXT("WreckName", "A WRECK · {0}"),
				FText::FromString(Near->AstroName.ToString().ToUpper())));
			FSight& Sight = State.Wrecks[Index];
			Sight.Body = Wreck;
			Sight.Marker = Marker;
			Darken(*Wreck, Sight.Lights);
		}
		// The station of the expedition before ours, over the living world's day side.
		UClass* StationClass = nullptr;
		const UGameInstance* GameInstance = World->GetGameInstance();
		const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
		if (Gameplay && Gameplay->SpawnParameters && Gameplay->SpawnParameters->BP_HomeSpaceStation)
		{
			StationClass = Gameplay->SpawnParameters->BP_HomeSpaceStation;
		}
		if (StationClass)
		{
			const double Radius = Home->GetWorldScapeBodyRadiusCm();
			FVector Day = (Star->GetActorLocation() - Home->GetActorLocation()).GetSafeNormal();
			Day = Day.RotateAngleAxis(40.0, Home->GetActorUpVector());
			const FVector Place = Home->GetActorLocation() + Day * FMath::Max(Radius * 1.08, Radius + 40000000.0);
			ASpaceStation* Station = World->SpawnActor<ASpaceStation>(StationClass, Place, FRotationMatrix::MakeFromZ(Day).Rotator(), Params);
			if (IsValid(Station))
			{
				Station->AttachToActor(Home, FAttachmentTransformRules::KeepWorldTransform);
				Station->CalculateAffectionRadius();
				Station->Tags.AddUnique(StationTag);
				State.Station.Body = Station;
				State.Station.Marker = Station;
				const bool bAwake = Board.IsUnlocked(StationToken);
				SetInGameName(Station, bAwake ? FText::Format(LOCTEXT("StationAwake", "THE STATION · {0}"), FText::FromString(Home->AstroName.ToString().ToUpper()))
					: FText::Format(LOCTEXT("StationDead", "A DEAD STATION · {0}"), FText::FromString(Home->AstroName.ToString().ToUpper())));
				if (!bAwake)
				{
					Darken(*Station, State.Station.Lights);
				}
			}
		}
		State.bSpawned = true;
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] the wrecks (%d) and the dead station stand"), Others.Num() >= WreckCount ? WreckCount : Others.Num());
	}

	bool HeldBeside(FSight& Sight, const ASpaceship* Ship, const double ReachCm, const float DeltaSeconds)
	{
		const AActor* Body = Sight.Body.Get();
		if (!Ship || !Body || FVector::Dist(Ship->GetActorLocation(), Body->GetActorLocation()) > ReachCm
			|| Ship->GetKinematicVelocity().Size() > SlowCmPerSecond)
		{
			Sight.HeldSeconds = 0.0f;
			return false;
		}
		Sight.HeldSeconds += DeltaSeconds;
		return Sight.HeldSeconds >= FMath::Max(CVarHoldSeconds.GetValueOnGameThread(), 0.5f);
	}

	void SetReactor(FAPSInfrastructure& Infrastructure, const float Percent)
	{
		Infrastructure.AddStock(APSInfrastructure::EResource::Energy, Percent - Infrastructure.GetStock(APSInfrastructure::EResource::Energy));
	}

	void PinCourse(UWorld* World, FState& State, const FString& StepId, AActor* Target)
	{
		ASpaceship* Ship = PilotedShip(World);
		if (!Ship || !Ship->ShipNavigation || !Target || State.PinnedStep == StepId)
		{
			return;
		}
		if (Ship->ShipNavigation->SetCourse(Target->GetPathName()))
		{
			State.PinnedStep = StepId;
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] course set for %s: %s"), *StepId, *GetNameSafe(Target));
		}
	}
}

bool APSOriginAdrift::IsAdriftWorld(const UWorld* World)
{
	return APSOriginArk::IsHonestStart(World) && APSWorldRules::OfGame(World).Origin == APSWorldRules::EOrigin::Adrift;
}

bool APSOriginAdrift::StartPlace(const AStar& Star, const APlanetaryBody& Home, FVector& OutLocation, FRotator& OutRotation)
{
	double Outermost = 0.0;
	for (const APlanet* Planet : Star.Planets)
	{
		if (IsValid(Planet))
		{
			Outermost = FMath::Max(Outermost, FVector::Dist(Planet->GetActorLocation(), Star.GetActorLocation()));
		}
	}
	const FVector StarPlace = Star.GetActorLocation();
	const double HomeOrbit = FVector::Dist(Home.GetActorLocation(), StarPlace);
	if (HomeOrbit <= 1.0)
	{
		return false;
	}
	// Beyond every world, but not so far that a dry reactor makes the home world a week's limp.
	const double Edge = FMath::Clamp(Outermost * 1.05, HomeOrbit * 1.6, HomeOrbit * 4.0);
	const FVector Out = (Home.GetActorLocation() - StarPlace).GetSafeNormal().RotateAngleAxis(50.0, Star.GetActorUpVector());
	OutLocation = StarPlace + Out * Edge;
	// The nose toward the sun: the whole unknown system ahead, its sun among the worlds.
	OutRotation = FRotationMatrix::MakeFromXZ((StarPlace - OutLocation).GetSafeNormal(), Star.GetActorUpVector()).Rotator();
	return true;
}

double APSOriginAdrift::CruiseScale(const AActor* Ship)
{
	const UWorld* World = Ship ? Ship->GetWorld() : nullptr;
	if (!World || !IsAdriftWorld(World))
	{
		return 1.0;
	}
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
	return Infrastructure && Infrastructure->GetStock(APSInfrastructure::EResource::Energy) < 1.0f
		? FMath::Clamp(static_cast<double>(APSOriginAdriftLocal::CVarDryCruise.GetValueOnGameThread()), 0.05, 1.0) : 1.0;
}

FText APSOriginAdrift::ReactorLine(const UWorld* World)
{
	const FAPSInfrastructure* Infrastructure = World && IsAdriftWorld(World) ? APSInfrastructureFind(World) : nullptr;
	if (!Infrastructure)
	{
		return FText::GetEmpty();
	}
	const float Percent = Infrastructure->GetStock(APSInfrastructure::EResource::Energy);
	return Percent < 1.0f ? LOCTEXT("ReactorDry", "REACTOR DRY: the cruise limps at a third")
		: FText::Format(LOCTEXT("Reactor", "REACTOR {0} %"), FText::AsNumber(FMath::FloorToInt(FMath::Min(Percent, 100.0f))));
}

bool APSOriginAdrift::IsDeadHull(const AActor* Actor)
{
	return IsValid(Actor) && (Actor->ActorHasTag(APSOriginAdriftLocal::WreckTag) || Actor->ActorHasTag(APSOriginAdriftLocal::StationTag));
}

void APSOriginAdrift::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSOriginAdriftLocal;
	if (!World || !World->IsGameWorld() || !IsAdriftWorld(World))
	{
		return;
	}
	const UGameInstance* GameInstance = World->GetGameInstance();
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (Gameplay && Gameplay->bPendingSavedWorldReplay)
	{
		return;
	}
	FAPSMissionBoard* Board = APSMissionsFind(World);
	FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
	AAstroGenerator* Gen = Generator(World);
	if (!Board || !Infrastructure || !Gen || !Gen->IsStarterHierarchySpawned())
	{
		return;
	}
	FState& State = StateFor(World);
	// The reactor at eleven percent, once (a load keeps its saved stock).
	if (!Board->IsUnlocked(InitToken))
	{
		Board->Unlock(InitToken);
		SetReactor(*Infrastructure, StartReactor);
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] reactor at %.0f %%"), StartReactor);
	}
	if (!State.bSpawned)
	{
		SpawnSights(World, State, *Gen, *Board);
	}
	ASpaceship* Ship = PilotedShip(World);
	// The cabin stays dark until the station wakes (a few lamps at the emergency level).
	if (Ship && !Board->IsUnlocked(StationToken) && State.DarkCabin.Get() != Ship && !IsDeadHull(Ship))
	{
		Restore(State.CabinLights);
		Darken(*Ship, State.CabinLights);
		State.DarkCabin = Ship;
	}
	// Every flight burns the reactor, the cruise most.
	if (Ship && Ship->GetEngineRunning() && Ship->FlightModel && CVarDrain.GetValueOnGameThread() > 0.0f)
	{
		float PerSecond = 0.01f;
		switch (Ship->FlightModel->GetFlightBand())
		{
		case EAPSFlightBand::Stellar: PerSecond = 0.4f; break;
		case EAPSFlightBand::Cruise: PerSecond = 0.2f; break;
		case EAPSFlightBand::Orbital: PerSecond = 0.03f; break;
		default: break;
		}
		const float Before = Infrastructure->GetStock(APSInfrastructure::EResource::Energy);
		if (Before > 0.0f)
		{
			Infrastructure->AddStock(APSInfrastructure::EResource::Energy, -PerSecond * CVarDrain.GetValueOnGameThread() * DeltaSeconds);
			const float After = Infrastructure->GetStock(APSInfrastructure::EResource::Energy);
			if (Before >= 1.0f && After < 1.0f)
			{
				Notice(World, LOCTEXT("DryTitle", "REACTOR DRY"), LOCTEXT("DryBody", "The last of it is gone. The cruise drive limps at a third; the wrecks and the station still have cells."),
					LOCTEXT("DryNext", "A wreck's cells, or the trickle charge"));
			}
		}
	}
	// The wrecks: hold beside one and the drones strip its cells.
	for (int32 Index = 0; Index < WreckCount; ++Index)
	{
		FSight& Sight = State.Wrecks[Index];
		if (Board->IsUnlocked(WreckToken(Index)) || !HeldBeside(Sight, Ship, WreckReachCm, DeltaSeconds))
		{
			continue;
		}
		Board->Unlock(WreckToken(Index));
		Board->Unlock(SalvageToken);
		Infrastructure->AddStock(APSInfrastructure::EResource::Energy, WreckCells);
		SetInGameName(Sight.Marker.Get(), LOCTEXT("WreckStripped", "STRIPPED WRECK"));
		Notice(World, LOCTEXT("SalvageTitle", "THE CELLS"), Index == 0
			? LOCTEXT("SalvageBody0", "Its log stops mid-sentence: \"...the station took the last of the crew. We are going down to the blue one.\" Its cells are ours now (+25 %).")
			: LOCTEXT("SalvageBody1", "No log this time, only a child's drawing taped to a console: a world with a lot of water. Its cells are ours (+25 %)."),
			LOCTEXT("SalvageNext", "The reactor holds a while longer"));
		if (UAPSAudioSubsystem* Audio = World->GetSubsystem<UAPSAudioSubsystem>())
		{
			Audio->PlayConfirm();
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] wreck %d stripped: +%.0f %%"), Index, WreckCells);
	}
	// The dead station: hold beside it and it wakes.
	if (!Board->IsUnlocked(StationToken) && HeldBeside(State.Station, Ship, StationReachCm, DeltaSeconds))
	{
		Board->Unlock(StationToken);
		SetReactor(*Infrastructure, 100.0f);
		Restore(State.Station.Lights);
		Restore(State.CabinLights);
		if (const APlanet* Home = Gen->HomePlanet)
		{
			SetInGameName(State.Station.Marker.Get(), FText::Format(LOCTEXT("StationAwake", "THE STATION · {0}"),
				FText::FromString(Home->AstroName.ToString().ToUpper())));
		}
		Notice(World, LOCTEXT("WakeTitle", "THE STATION WAKES"), LOCTEXT("WakeBody", "Its lights come on bay by bay. Its tanks were full all along: the reactor reads a hundred percent, and the cabin is lit again."),
			LOCTEXT("WakeNext", "Down to the living world"));
		if (UAPSAudioSubsystem* Audio = World->GetSubsystem<UAPSAudioSubsystem>())
		{
			Audio->PlayPowerUp();
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] the dead station wakes: reactor full"));
	}
	// The story's course: the wreck nearest the ship while its step is open, then the station.
	const APSOriginQuests::FView V = APSOriginQuests::View(World);
	if (const APSOriginQuests::FStep* Step = V.IsValid() ? V.Current() : nullptr)
	{
		const FString StepId(Step->Id);
		if (StepId == TEXT("Wreck"))
		{
			AActor* Nearest = nullptr;
			double Best = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < WreckCount; ++Index)
			{
				AActor* Marker = State.Wrecks[Index].Marker.Get();
				if (Marker && !Board->IsUnlocked(WreckToken(Index)) && Ship)
				{
					const double D = FVector::DistSquared(Marker->GetActorLocation(), Ship->GetActorLocation());
					Nearest = D < Best ? Marker : Nearest;
					Best = FMath::Min(Best, D);
				}
			}
			PinCourse(World, State, StepId, Nearest);
		}
		else if (StepId == TEXT("Station"))
		{
			PinCourse(World, State, StepId, State.Station.Marker.Get());
		}
	}
	// Down on the living world: the ship is the home, the colony's site moves to where the pilot first stood.
	const APSOriginQuests::FChain* Chain = APSOriginQuests::ChainOf(APSWorldRules::EOrigin::Adrift);
	const int32 DownIndex = Chain ? Chain->Steps.IndexOfByPredicate([](const APSOriginQuests::FStep& Candidate)
	{
		return FCString::Strcmp(Candidate.Id, TEXT("Down")) == 0;
	}) : INDEX_NONE;
	if (DownIndex != INDEX_NONE && !Board->IsUnlocked(HomedToken) && Board->IsUnlocked(APSOriginQuests::StepToken(*Chain, DownIndex)))
	{
		const APlayerController* Controller = World->GetFirstPlayerController();
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		UAPSCivilizationMaterializationSubsystem* Materialization = World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>();
		if (Pawn && Pawn->IsA<ACharacter>() && Materialization && Materialization->RelocateBaseNear(Pawn->GetActorLocation()))
		{
			Board->Unlock(HomedToken);
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] the colony's site moves to the landing"));
		}
	}
}

namespace APSOriginAdriftConsole
{
	// Offscreen runs: the reactor by hand, and a jump beside a sight (the hold does the rest).
	FAutoConsoleCommandWithWorldAndArgs ReactorCommand(TEXT("aps.Origin.AdriftReactor"),
		TEXT("Test: sets the ADRIFT reactor to a percentage (aps.Origin.AdriftReactor 0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (FAPSInfrastructure* Infrastructure = World ? APSInfrastructureFind(World) : nullptr; Infrastructure && Args.Num() > 0)
			{
				APSOriginAdriftLocal::SetReactor(*Infrastructure, FCString::Atof(*Args[0]));
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] %s"), *APSOriginAdrift::ReactorLine(World).ToString());
		}));

	FAutoConsoleCommandWithWorldAndArgs BesideCommand(TEXT("aps.Origin.AdriftBeside"),
		TEXT("Test: moves the piloted ship 1.5 km beside a sight: aps.Origin.AdriftBeside wreck0 | wreck1 | station."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			using namespace APSOriginAdriftLocal;
			ASpaceship* Ship = PilotedShip(World);
			if (!Ship || Args.IsEmpty())
			{
				return;
			}
			FState& State = StateFor(World);
			const AActor* Target = Args[0].Equals(TEXT("station"), ESearchCase::IgnoreCase) ? State.Station.Body.Get()
				: Args[0].Equals(TEXT("wreck1"), ESearchCase::IgnoreCase) ? State.Wrecks[1].Body.Get() : State.Wrecks[0].Body.Get();
			if (!Target)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Origin.Adrift] no such sight standing"));
				return;
			}
			const FVector Away = (Ship->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
			const FVector Place = Target->GetActorLocation() + Away * 150000.0;
			Ship->SetActorLocationAndRotation(Place, FRotationMatrix::MakeFromX(-Away).Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Adrift] the ship stands 1.5 km beside %s"), *GetNameSafe(Target));
		}));
}

#undef LOCTEXT_NAMESPACE
