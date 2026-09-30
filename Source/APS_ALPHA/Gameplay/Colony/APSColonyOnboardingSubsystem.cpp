#include "APSColonyOnboardingSubsystem.h"

#include "APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APS_ALPHA/Gameplay/Quests/APSEarlyAccessOnboardingDefinition.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestSubsystem.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "APSColonyOnboarding"

DEFINE_LOG_CATEGORY_STATIC(LogAPSColonyOnboarding, Log, All);

namespace APSColonyOnboarding
{
	constexpr float PollIntervalSeconds = 0.5f;
	/** The colony terminal counts as the base interaction within this reach of the base. */
	constexpr double BaseReachCm = 30000.0;
	/** The pad is reached this far past its deck's edge. */
	constexpr double PadReachMarginCm = 1500.0;
	/** Takeoff: the ship has left the spot where the pilot boarded it by this much. */
	constexpr double TakeoffDistanceCm = 30000.0;
	/** Arrival: within this many radii of the destination's centre. */
	constexpr double ArrivalRadii = 3.0;
	/** Landing: this close above the destination's surface, this slow. */
	constexpr double LandingAltitudeCm = 300000.0;
	constexpr double LandingSpeedCmPerSecond = 5000.0;
	/** The surface around a landing is taken as ready this long after the landing. */
	constexpr double AnchorSettleSeconds = 3.0;
	/** On foot this close above the destination's surface counts as being on it. */
	constexpr double OnSurfaceAltitudeCm = 1000000.0;
	/** Back in orbital space: this far above the destination's surface again. */
	constexpr double ExitAltitudeCm = 10000000.0;
	/** A home-planet destination is arrived at only after the ship has been this high above it. */
	constexpr double LeftHomeAltitudeCm = 5000000.0;

	const FName CharacterStep(TEXT("CharacterPossessed"));
	const FName BaseStep(TEXT("BaseOpened"));
	const FName PadStep(TEXT("LandingPadReached"));
	const FName ShipStep(TEXT("ShipPossessed"));
	const FName EnginesStep(TEXT("EnginesStarted"));
	const FName TakeoffStep(TEXT("TakeoffCommitted"));
	const FName CourseStep(TEXT("DestinationSelected"));
	const FName ArrivalStep(TEXT("ArrivalCommitted"));
	const FName LandingStep(TEXT("LandingCommitted"));
	const FName AnchorStep(TEXT("SurfaceAnchorReady"));
	const FName EnteredStep(TEXT("SurfaceEntered"));
	const FName ExitedStep(TEXT("SurfaceExited"));
	const FName HomeStep(TEXT("SystemSelectionRestored"));

	FAPSQuestEntityRef GuidEntity(const EAPSQuestEntityKind Kind, const FGuid& Guid)
	{
		FAPSQuestEntityRef Ref;
		Ref.Kind = Kind;
		Ref.Guid = Guid;
		return Ref;
	}

	FAPSQuestEntityRef KeyEntity(const EAPSQuestEntityKind Kind, const FString& Key)
	{
		FAPSQuestEntityRef Ref;
		Ref.Kind = Kind;
		Ref.StableKey = Key;
		return Ref;
	}

	/** Height above a body's surface sphere (its WorldScape radius). */
	double AltitudeCm(const APlanetaryBody* Body, const FVector& Location)
	{
		return FVector::Distance(Body->GetActorLocation(), Location) - Body->GetWorldScapeBodyRadiusCm();
	}

	/** The body a ship's navigation course points at, when it is a body. */
	const AActor* CourseActor(const ASpaceship* Ship)
	{
		const FShipNavigationContact* Course = Ship && Ship->ShipNavigation
			? Ship->ShipNavigation->GetSelectedContact() : nullptr;
		return Course ? Course->Actor.Get() : nullptr;
	}
}

bool UAPSColonyOnboardingSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAPSColonyOnboardingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSColonyOnboardingSubsystem, STATGROUP_Tickables);
}

UAPSQuestSubsystem* UAPSColonyOnboardingSubsystem::GetQuest() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UAPSQuestSubsystem>() : nullptr;
}

void UAPSColonyOnboardingSubsystem::Tick(const float DeltaTime)
{
	PollAccumulator += DeltaTime;
	if (PollAccumulator < APSColonyOnboarding::PollIntervalSeconds)
	{
		return;
	}
	PollAccumulator = 0.0f;
	if (!bBound && !TryBind())
	{
		return;
	}
	OfferSteps();
	JournalObjective();
}

bool UAPSColonyOnboardingSubsystem::TryBind()
{
	using namespace APSColonyOnboarding;
	using Contract = FAPSEarlyAccessOnboardingContract;
	UWorld* World = GetWorld();
	const UAPSCivilizationMaterializationSubsystem* Colony = World
		? World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>() : nullptr;
	UAPSQuestSubsystem* Quest = GetQuest();
	if (!Colony || !Quest || !Colony->IsMaterializationComplete())
	{
		return false;
	}
	const FAPSCivilizationRuntimeManifest& Manifest = Colony->GetRuntimeManifest();
	const FAPSCivilizationManifestEntity* BaseEntity = Manifest.FindEntity(EAPSCivilizationEntityRole::BaseModule);
	const FAPSCivilizationManifestEntity* PadEntity = Manifest.FindEntity(EAPSCivilizationEntityRole::LandingPad);
	const FAPSCivilizationManifestEntity* ShipEntity = Manifest.FindEntity(EAPSCivilizationEntityRole::SelectedShip);
	if (!BaseEntity || !PadEntity || !ShipEntity || !Manifest.CivilizationId.IsValid())
	{
		return false;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		const UAPSCivilizationIdentityComponent* Identity = IsValid(*It)
			&& It->ActorHasTag(TEXT("APS.Civilization.Materialized"))
			? It->FindComponentByClass<UAPSCivilizationIdentityComponent>() : nullptr;
		if (!Identity)
		{
			continue;
		}
		if (Identity->StableEntityId == BaseEntity->StableId)
		{
			BaseActor = *It;
		}
		else if (Identity->StableEntityId == PadEntity->StableId)
		{
			PadActor = *It;
		}
		else if (Identity->StableEntityId == ShipEntity->StableId)
		{
			ShipActor = Cast<ASpaceship>(*It);
		}
	}
	HomeBody = BaseActor.IsValid() ? Cast<APlanetaryBody>(BaseActor->GetAttachParentActor()) : nullptr;
	if (!HomeBody.IsValid())
	{
		return false;
	}
	Pilot = GuidEntity(EAPSQuestEntityKind::GameplayEntity,
		UAPSColonyConstructionSubsystem::MakePilotStableId(Manifest.CivilizationId));
	Base = GuidEntity(EAPSQuestEntityKind::CivilizationEntity, BaseEntity->StableId);
	Pad = GuidEntity(EAPSQuestEntityKind::CivilizationEntity, PadEntity->StableId);
	Ship = GuidEntity(EAPSQuestEntityKind::CivilizationEntity, ShipEntity->StableId);
	HomeSystem = GuidEntity(EAPSQuestEntityKind::StarSystem, Manifest.HomeSystemId);

	// The destination: the first home moon, else the nearest other planet of the system.
	FString DestinationKey;
	if (const APlanet* Planet = Cast<APlanet>(HomeBody.Get()))
	{
		for (int32 Index = 0; Index < Planet->Moons.Num() && !DestinationBody.IsValid(); ++Index)
		{
			if (IsValid(Planet->Moons[Index]))
			{
				DestinationBody = Planet->Moons[Index];
				DestinationKey = FString::Printf(TEXT("%s/M%d"), *Manifest.HomeBodyKey, Index);
			}
		}
	}
	if (!DestinationBody.IsValid())
	{
		double Nearest = TNumericLimits<double>::Max();
		for (TActorIterator<APlanet> It(World); It; ++It)
		{
			const double Distance = IsValid(*It) && *It != HomeBody.Get()
				? FVector::Distance(It->GetActorLocation(), HomeBody->GetActorLocation()) : Nearest;
			if (Distance < Nearest)
			{
				Nearest = Distance;
				DestinationBody = *It;
				DestinationKey = FString::Printf(TEXT("SYS0/S0/%s"), *It->GetName());
			}
		}
	}
	if (!DestinationBody.IsValid())
	{
		// A lone planet without moons: the flight lesson goes to orbit and lands on the home planet again.
		DestinationBody = HomeBody;
		DestinationKey = Manifest.HomeBodyKey;
		bDestinationIsHome = true;
	}

	FString Reason;
	if (!Quest->BindQuestEntity(Contract::QuestId, Contract::PlayerCharacterBinding, Pilot, Reason))
	{
		// The civilization adapter starts the quest when the colony is ready: try again shortly.
		UE_LOG(LogAPSColonyOnboarding, Verbose, TEXT("[APS.Onboarding] pilot not bound yet: %s"), *Reason);
		return false;
	}
	if (DestinationBody.IsValid())
	{
		Destination = KeyEntity(EAPSQuestEntityKind::CelestialBody, DestinationKey);
		LandingContext = KeyEntity(EAPSQuestEntityKind::LandingContext, DestinationKey + TEXT("/Landing"));
		if (!Quest->BindQuestEntity(Contract::QuestId, Contract::DestinationBodyBinding, Destination, Reason)
			|| !Quest->BindQuestEntity(Contract::QuestId, Contract::LandingContextBinding, LandingContext, Reason))
		{
			UE_LOG(LogAPSColonyOnboarding, Warning, TEXT("[APS.Onboarding] destination not bound: %s"), *Reason);
		}
	}
	StreamId = FAPSCivilizationRuntimeManifestFactory::MakeStableId(TEXT("APS.Onboarding.PilotStream"),
		Manifest.ManifestId.ToString(EGuidFormats::DigitsWithHyphens));
	bBound = true;
	UE_LOG(LogAPSColonyOnboarding, Log, TEXT("[APS.Onboarding] pilot bound; destination %s (%s)"),
		*GetNameSafe(DestinationBody.Get()), *DestinationKey);
	return true;
}

bool UAPSColonyOnboardingSubsystem::Offer(const FName Step, const FName Verb, const FAPSQuestEntityRef& Subject,
	const FAPSQuestEntityRef& Target)
{
	if (Accepted.Contains(Step))
	{
		return true;
	}
	UAPSQuestSubsystem* Quest = GetQuest();
	if (!Quest)
	{
		return false;
	}
	FAPSQuestEvent Event;
	Event.StreamId = StreamId;
	Event.EventId = FGuid::NewGuid();
	Event.CorrelationId = FGuid::NewGuid();
	Event.Sequence = ++Sequence;
	Event.Verb = Verb;
	Event.Result = EAPSQuestEventResult::Succeeded;
	Event.Subject = Subject;
	Event.Target = Target;
	Event.Quantity = 1;
	FString Reason;
	if (!Quest->SubmitQuestEvent(Event, Reason))
	{
		// Not the active objective yet (or its bindings are missing): offered again next time.
		return false;
	}
	Accepted.Add(Step);
	UE_LOG(LogAPSColonyOnboarding, Log, TEXT("[APS.Onboarding] %s accepted (%s)"), *Step.ToString(),
		*Verb.ToString());
	return true;
}

void UAPSColonyOnboardingSubsystem::OfferSteps()
{
	using namespace APSColonyOnboarding;
	UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	const FAPSQuestEntityRef None;
	const bool bOnFoot = Pawn->IsA<ACustomGravityCharacter>();
	ASpaceship* HomeShip = ShipActor.Get();
	const bool bPilotingHomeShip = HomeShip && Pawn == HomeShip;
	APlanetaryBody* Target = DestinationBody.Get();

	if (bOnFoot)
	{
		Offer(CharacterStep, TEXT("APS.Player.Character.Possessed"), Pilot, None);
	}
	if (bTerminalOpenedAtBase)
	{
		Offer(BaseStep, TEXT("APS.Interaction.Open"), Pilot, Base);
	}
	if (bOnFoot && PadActor.IsValid())
	{
		FVector PadCenter;
		FVector PadExtent;
		PadActor->GetActorBounds(true, PadCenter, PadExtent);
		const FVector Up = PadActor->GetActorUpVector();
		const double Across = FVector::VectorPlaneProject(Pawn->GetActorLocation() - PadActor->GetActorLocation(), Up).Size();
		if (Across <= FMath::Max(PadExtent.X, PadExtent.Y) + PadReachMarginCm)
		{
			Offer(PadStep, TEXT("APS.Navigation.Reach"), Pilot, Pad);
		}
	}
	if (bPilotingHomeShip)
	{
		if (!bBoarded)
		{
			bBoarded = true;
			BoardLocation = HomeShip->GetActorLocation();
		}
		Offer(ShipStep, TEXT("APS.Flight.Ship.Possessed"), Pilot, Ship);
		if (HomeShip->GetEngineRunning())
		{
			Offer(EnginesStep, TEXT("APS.Flight.Engine.Started"), None, Ship);
		}
		if (FVector::Distance(HomeShip->GetActorLocation(), BoardLocation) > TakeoffDistanceCm)
		{
			Offer(TakeoffStep, TEXT("APS.Flight.Takeoff.Committed"), None, Ship);
		}
	}
	// A course from the ship's own target keys or from the terminal's SYSTEM tab.
	const ASpaceship* CourseShip = Cast<ASpaceship>(Pawn) ? Cast<ASpaceship>(Pawn) : HomeShip;
	const AActor* Course = CourseActor(CourseShip);
	if (Target && Course == Target)
	{
		Offer(CourseStep, TEXT("APS.Navigation.Body.Selected"), Pilot, Destination);
	}
	if (Target && bPilotingHomeShip)
	{
		const double Altitude = AltitudeCm(Target, HomeShip->GetActorLocation());
		bLeftHome |= bDestinationIsHome && Altitude >= LeftHomeAltitudeCm;
		if (FVector::Distance(HomeShip->GetActorLocation(), Target->GetActorLocation())
			<= Target->GetWorldScapeBodyRadiusCm() * ArrivalRadii && (!bDestinationIsHome || bLeftHome))
		{
			Offer(ArrivalStep, TEXT("APS.Navigation.Arrival.Committed"), None, Destination);
		}
		if (Altitude <= LandingAltitudeCm && HomeShip->GetVelocity().Size() <= LandingSpeedCmPerSecond
			&& Offer(LandingStep, TEXT("APS.Surface.Landing.Committed"), Destination, LandingContext)
			&& LandedSeconds < 0.0)
		{
			LandedSeconds = World->GetTimeSeconds();
		}
		if (Accepted.Contains(EnteredStep) && Altitude >= ExitAltitudeCm)
		{
			Offer(ExitedStep, TEXT("APS.Surface.Exited"), Destination, LandingContext);
		}
	}
	if (LandedSeconds >= 0.0 && World->GetTimeSeconds() - LandedSeconds >= AnchorSettleSeconds)
	{
		Offer(AnchorStep, TEXT("APS.Surface.Anchor.Ready"), Destination, LandingContext);
	}
	if (Target && bOnFoot && Accepted.Contains(AnchorStep)
		&& AltitudeCm(Target, Pawn->GetActorLocation()) <= OnSurfaceAltitudeCm)
	{
		Offer(EnteredStep, TEXT("APS.Surface.Entered"), Destination, LandingContext);
	}
	// Finally the course home: the home planet (or its star) selected again.
	if (Accepted.Contains(ExitedStep) && Course && (Course == HomeBody.Get()
		|| (Cast<APlanet>(HomeBody.Get()) && Course == Cast<APlanet>(HomeBody.Get())->ParentStar)))
	{
		Offer(HomeStep, TEXT("APS.Navigation.System.Selected"), Pilot, HomeSystem);
	}
}

void UAPSColonyOnboardingSubsystem::NotifyTerminalOpened()
{
	// The base must be known to judge the distance: a terminal opened right at founding would otherwise be lost.
	if (!bBound)
	{
		TryBind();
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (Pawn && BaseActor.IsValid()
		&& FVector::Distance(Pawn->GetActorLocation(), BaseActor->GetActorLocation()) <= APSColonyOnboarding::BaseReachCm)
	{
		bTerminalOpenedAtBase = true;
	}
}

bool UAPSColonyOnboardingSubsystem::GetObjective(FText& OutTitle, FText& OutBody) const
{
	const UAPSQuestSubsystem* Quest = GetQuest();
	FAPSQuestPromptSnapshot Prompt;
	if (!Quest || !Quest->TryGetCurrentPromptSnapshot(Prompt) || Prompt.State != EAPSQuestPromptState::Active)
	{
		return false;
	}
	OutTitle = Prompt.Title;
	OutBody = Prompt.Body;
	return true;
}

void UAPSColonyOnboardingSubsystem::JournalObjective()
{
	const UAPSQuestSubsystem* Quest = GetQuest();
	FAPSQuestPromptSnapshot Prompt;
	if (!Quest || !Quest->TryGetCurrentPromptSnapshot(Prompt) || Prompt.State != EAPSQuestPromptState::Active
		|| Prompt.PromptStableId == LastPromptId)
	{
		return;
	}
	LastPromptId = Prompt.PromptStableId;
	UAPSCivilizationJournalSubsystem::Post(this, TEXT("Objective"), FText::Format(
		LOCTEXT("NewObjective", "{0}. {1}"), Prompt.Title, Prompt.Body));
}

#undef LOCTEXT_NAMESPACE
