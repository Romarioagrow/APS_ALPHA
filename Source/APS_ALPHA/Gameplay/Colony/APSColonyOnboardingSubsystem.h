#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSColonyOnboardingSubsystem.generated.h"

class AActor;
class APlanetaryBody;
class ASpaceship;
class UAPSQuestSubsystem;

/**
 * Plays the early-access onboarding quest (APS-80) from what the pilot actually does (U4, Rio 29.09: systems in the
 * code must be reachable in the game). The civilization adapter starts the quest and binds the base, pad, ship and home
 * system; this bridge binds the pilot, a destination (the first home moon, else the nearest other planet) and its
 * landing context, and feeds one owned event stream: character control, the colony terminal opened at the base, the
 * landing pad reached, the home ship boarded, engines, takeoff, course, arrival, landing, surface in and out, course
 * home. Construction orders already carry the pilot as subject, so production's own build events count too.
 * Each step is re-offered while its condition holds until the quest accepts it; the quest ignores steps that are not
 * its active objective yet.
 */
UCLASS()
class APS_ALPHA_API UAPSColonyOnboardingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate(); }

	/** The colony terminal opened: within reach of the base it is the base interaction the quest asks for. */
	void NotifyTerminalOpened();

	/** The current objective, when the onboarding shows one. */
	bool GetObjective(FText& OutTitle, FText& OutBody) const;

private:
	UAPSQuestSubsystem* GetQuest() const;
	bool TryBind();
	void OfferSteps();
	/** One authoritative fact on this bridge's stream; true once the quest consumed it. */
	bool Offer(FName Step, FName Verb, const FAPSQuestEntityRef& Subject, const FAPSQuestEntityRef& Target);
	void JournalObjective();

	FGuid StreamId;
	int64 Sequence{0};
	FAPSQuestEntityRef Pilot;
	FAPSQuestEntityRef Base;
	FAPSQuestEntityRef Pad;
	FAPSQuestEntityRef Ship;
	FAPSQuestEntityRef HomeSystem;
	FAPSQuestEntityRef Destination;
	FAPSQuestEntityRef LandingContext;
	TWeakObjectPtr<AActor> BaseActor;
	TWeakObjectPtr<AActor> PadActor;
	TWeakObjectPtr<ASpaceship> ShipActor;
	TWeakObjectPtr<APlanetaryBody> DestinationBody;
	TWeakObjectPtr<APlanetaryBody> HomeBody;
	/** Steps the quest has consumed. */
	TSet<FName> Accepted;
	FVector BoardLocation{FVector::ZeroVector};
	bool bBoarded{false};
	/** With no moon or other planet the home planet is the destination: arrival then needs a trip to orbit first. */
	bool bDestinationIsHome{false};
	bool bLeftHome{false};
	bool bTerminalOpenedAtBase{false};
	bool bBound{false};
	double LandedSeconds{-1.0};
	float PollAccumulator{0.0f};
	FGuid LastPromptId;
};
