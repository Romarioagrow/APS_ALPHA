#include "APSObjectActions.h"

#include "APSInfrastructure.h"
#include "APSStarSystems.h"
#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Megastructures/APSMegastructures.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "APSObjectActions"

namespace APSObjectActionsLocal
{
	using namespace APSFleet;

	TArray<TPair<FName, APSObjectActions::FProvider>>& Providers()
	{
		static TArray<TPair<FName, APSObjectActions::FProvider>> List;
		return List;
	}

	const FLinearColor NavigationColour(0.30f, 0.80f, 1.00f, 1.0f);
	const FLinearColor FleetColour(0.95f, 0.85f, 0.35f, 1.0f);

	ASpaceship* PilotedShip(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr;
	}

	FText DistanceText(UWorld* World, const AActor* Object)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn || !Object) return FText::GetEmpty();
		const double Cm = FVector::Dist(Pawn->GetActorLocation(), Object->GetActorLocation());
		FNumberFormattingOptions Two;
		Two.SetMaximumFractionalDigits(2);
		return Cm >= 0.01 * APSStars::AstronomicalUnitCm
			? FText::Format(LOCTEXT("DistanceAu", "{0} AU"), APSUINumber::Number(Cm / APSStars::AstronomicalUnitCm, &Two))
			: FText::Format(LOCTEXT("DistanceKm", "{0} km"), APSUINumber::Number(FMath::RoundToInt64(Cm / 100000.0)));
	}

	/** The idle ship of a division nearest the object that can take the order. */
	ASpaceship* PickOfDivision(const FAPSFleetCommand& Fleet, const EOrder Order, const AActor* Target, const EDivision Division,
		FText& OutRefusal)
	{
		ASpaceship* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (const FAPSFleetUnit& Unit : Fleet.GetUnits())
		{
			ASpaceship* Ship = Unit.Ship.Get();
			if (!Ship || Unit.Division != Division || (Unit.Order != EOrder::None && Unit.Phase != EPhase::Holding)) continue;
			const FText Refusal = Fleet.CheckOrder(Ship, Order, Target);
			if (!Refusal.IsEmpty())
			{
				if (OutRefusal.IsEmpty()) OutRefusal = Refusal;
				continue;
			}
			const double Distance = FVector::DistSquared(Ship->GetActorLocation(), Target->GetActorLocation());
			if (Distance < BestDistance) { BestDistance = Distance; Best = Ship; }
		}
		if (!Best && OutRefusal.IsEmpty())
		{
			OutRefusal = FText::Format(LOCTEXT("NoDivisionShip", "No free {0} ship."), DivisionName(Division));
		}
		return Best;
	}

	/** What a ship does at the target, for a button whose order is under way. */
	FText WorkingStatus(const EOrder Order)
	{
		switch (Order)
		{
		case EOrder::BuildOutpost:
		case EOrder::BuildStation:
		case EOrder::BuildShipyard:
		case EOrder::BuildHeadquarters:
		case EOrder::BuildStructure: return LOCTEXT("UnderwayBuilding", "BUILDING");
		case EOrder::Survey:
		case EOrder::SurveySystem: return LOCTEXT("UnderwaySurveying", "SURVEYING");
		case EOrder::Probe: return LOCTEXT("UnderwayScanning", "SCANNING");
		case EOrder::Expedition: return LOCTEXT("UnderwayInvestigating", "INVESTIGATING");
		default: return LOCTEXT("UnderwayWorking", "AT WORK");
		}
	}

	FAPSObjectAction FleetAction(UWorld* World, const FName Id, const FText& Label, const EOrder Order, AActor* Object,
		ASpaceship* Ship, const FText& Refusal, const FName StructureType = NAME_None,
		const TOptional<EDivision> Division = TOptional<EDivision>())
	{
		FAPSObjectAction Action;
		Action.Id = Id;
		Action.Label = Label;
		Action.Group = LOCTEXT("GroupFleet", "FLEET");
		Action.Colour = FleetColour;
		Action.bEnabled = Ship != nullptr;
		const FAPSFleetCommand* Fleet = APSFleetFind(World);
		const FAPSFleetUnit* Unit = Fleet && Ship ? Fleet->FindUnit(Ship) : nullptr;
		Action.Detail = Unit ? FText::Format(LOCTEXT("SentShip", "{0} ({1}) goes."), FText::FromString(Unit->CallSign),
			DivisionName(Unit->Division)) : Refusal;
		TWeakObjectPtr<UWorld> WeakWorld = World;
		TWeakObjectPtr<AActor> WeakObject = Object;
		TWeakObjectPtr<ASpaceship> WeakShip = Ship;
		Action.Execute = [WeakWorld, WeakObject, WeakShip, Order, StructureType]()
		{
			FAPSFleetCommand* Command = APSFleetFind(WeakWorld.Get());
			ASpaceship* Picked = WeakShip.Get();
			if (!Command || !Picked || !WeakObject.IsValid()) return LOCTEXT("OrderGone", "The ship or the target is gone.");
			FText Refused;
			return Command->IssueOrder({Picked}, Order, WeakObject.Get(), Refused, StructureType) > 0
				? FText::Format(LOCTEXT("OrderGiven", "{0}: {1}."), FText::FromString(Command->FindUnit(Picked)->CallSign),
					OrderName(Order))
				: Refused;
		};
		// Rio 04.10: a ship already on this order here (on its way, or at work) makes the button a progress bar: the flight
		// is the first part of the fill, the work the rest.
		Action.Underway = [WeakWorld, WeakObject, Order, StructureType, Division](float& OutProgress, FText& OutStatus, FText& OutWho)
		{
			constexpr float FlightShare = 0.3f;
			const FAPSFleetCommand* Command = APSFleetFind(WeakWorld.Get());
			const AActor* Target = WeakObject.Get();
			if (!Command || !Target) return false;
			for (const FAPSFleetUnit& Unit : Command->GetUnits())
			{
				if (Unit.Order != Order || Unit.Target.Get() != Target
					|| (Unit.Phase != EPhase::Departing && Unit.Phase != EPhase::Transit && Unit.Phase != EPhase::Working)
					|| (Order == EOrder::BuildStructure && Unit.StructureType != StructureType)
					|| (Division.IsSet() && Unit.Division != Division.GetValue()))
				{
					continue;
				}
				OutWho = FText::Format(LOCTEXT("UnderwayWho", "{0} ({1})"), FText::FromString(Unit.CallSign), DivisionName(Unit.Division));
				if (Unit.Phase == EPhase::Working)
				{
					OutProgress = FlightShare + (1.0f - FlightShare) * FMath::Clamp(Unit.Progress, 0.0f, 1.0f);
					OutStatus = WorkingStatus(Order);
				}
				else
				{
					const double Flown = Unit.TransitStartCm > 0.0 ? 1.0 - Unit.RemainingCm / Unit.TransitStartCm : 0.0;
					OutProgress = FlightShare * static_cast<float>(FMath::Clamp(Flown, 0.0, 1.0));
					OutStatus = LOCTEXT("UnderwayEnRoute", "EN ROUTE");
				}
				return true;
			}
			return false;
		};
		return Action;
	}

	/**
	 * Rio 04.10 ("what the units do, we must be able to do ourselves, from the bridge of a ship that can"): an order the
	 * piloted ship carries out itself, by the fleet's rules (FAPSFleetCommand::IssuePilotOrder): its division decides what
	 * it can do, it must be near enough, and the work goes on while it stays there.
	 */
	FAPSObjectAction PilotAction(UWorld* World, const FName Id, const FText& Label, const EOrder Order, AActor* Object,
		ASpaceship* Ship, const FName StructureType = NAME_None)
	{
		FAPSObjectAction Action;
		Action.Id = Id;
		Action.Label = Label;
		Action.Group = LOCTEXT("GroupPilot", "YOUR SHIP");
		Action.Colour = NavigationColour;
		const FAPSFleetCommand* Command = APSFleetFind(World);
		const FText Refusal = Command ? Command->CheckPilotOrder(Ship, Order, Object, StructureType)
			: LOCTEXT("PilotNoFleet", "No fleet command in this world.");
		Action.bEnabled = Refusal.IsEmpty();
		Action.Detail = Action.bEnabled ? LOCTEXT("PilotDetail", "Your ship does it itself; stay near until it is done.") : Refusal;
		TWeakObjectPtr<UWorld> WeakWorld = World;
		TWeakObjectPtr<AActor> WeakObject = Object;
		TWeakObjectPtr<ASpaceship> WeakShip = Ship;
		Action.Execute = [WeakWorld, WeakObject, WeakShip, Order, StructureType]()
		{
			FAPSFleetCommand* Fleet = APSFleetFind(WeakWorld.Get());
			ASpaceship* Piloted = WeakShip.Get();
			if (!Fleet || !Piloted || !WeakObject.IsValid()) return LOCTEXT("OrderGone", "The ship or the target is gone.");
			const FText Refused = Fleet->IssuePilotOrder(Piloted, Order, WeakObject.Get(), StructureType);
			return Refused.IsEmpty() ? FText::Format(LOCTEXT("PilotOrderGiven", "YOUR SHIP: {0}."), OrderName(Order)) : Refused;
		};
		Action.Underway = [WeakWorld, WeakObject, WeakShip, Order, StructureType](float& OutProgress, FText& OutStatus, FText& OutWho)
		{
			const FAPSFleetCommand* Fleet = APSFleetFind(WeakWorld.Get());
			const ASpaceship* Piloted = WeakShip.Get();
			const FAPSFleetUnit* Unit = Fleet && Piloted ? Fleet->FindUnit(Piloted) : nullptr;
			if (!Unit || !Unit->bPilotWork || Unit->Order != Order || Unit->Phase != EPhase::Working
				|| !WeakObject.IsValid() || Unit->Target.Get() != WeakObject.Get()
				|| (Order == EOrder::BuildStructure && Unit->StructureType != StructureType))
			{
				return false;
			}
			OutProgress = FMath::Clamp(Unit->Progress, 0.0f, 1.0f);
			OutWho = LOCTEXT("PilotWho", "Your ship");
			double Distance = 0.0;
			double Range = 0.0;
			OutStatus = Fleet->PilotWorkRange(Piloted, Order, WeakObject.Get(), Distance, Range) && Distance > Range
				? LOCTEXT("PilotOutOfRange", "OUT OF RANGE, PAUSED") : WorkingStatus(Order);
			return true;
		};
		return Action;
	}

	void Navigation(UWorld* World, AActor* Object, TArray<FAPSObjectAction>& OutActions)
	{
		ASpaceship* Ship = PilotedShip(World);
		{
			FAPSObjectAction Action;
			Action.Id = TEXT("Nav.SetCourse");
			Action.Label = LOCTEXT("SetCourse", "SET COURSE");
			Action.Group = LOCTEXT("GroupNavigation", "NAVIGATION");
			Action.Colour = NavigationColour;
			Action.bEnabled = Ship && Ship->ShipNavigation;
			Action.Detail = Action.bEnabled ? LOCTEXT("SetCourseDetail", "The HUD marks it; fly there yourself.")
				: LOCTEXT("NotPiloting", "Pilot a ship first.");
			TWeakObjectPtr<UWorld> WeakWorld = World;
			TWeakObjectPtr<AActor> WeakObject = Object;
			Action.Execute = [WeakWorld, WeakObject]()
			{
				ASpaceship* Piloted = PilotedShip(WeakWorld.Get());
				if (!Piloted || !Piloted->ShipNavigation || !WeakObject.IsValid()) return LOCTEXT("NoCourse", "No ship or target.");
				Piloted->ShipNavigation->RefreshContacts(Piloted->GetActorLocation(), true);
				return Piloted->ShipNavigation->SetCourse(WeakObject->GetPathName())
					? FText::Format(LOCTEXT("CourseSet", "COURSE SET: {0}"), APSObjectActions::NameOf(WeakObject.Get()))
					: LOCTEXT("NotCharted", "Not charted for navigation yet.");
			};
			OutActions.Add(MoveTemp(Action));
		}
		{
			FAPSObjectAction Action;
			Action.Id = TEXT("Nav.Autopilot");
			Action.Label = LOCTEXT("Autopilot", "AUTOPILOT THERE");
			Action.Group = LOCTEXT("GroupNavigation", "NAVIGATION");
			Action.Colour = NavigationColour;
			Action.bEnabled = Ship && Ship->FlightModel;
			Action.Detail = Action.bEnabled ? LOCTEXT("AutopilotDetail", "Your ship flies there itself; any helm input takes over.")
				: LOCTEXT("NotPiloting", "Pilot a ship first.");
			TWeakObjectPtr<UWorld> WeakWorld = World;
			TWeakObjectPtr<AActor> WeakObject = Object;
			Action.Execute = [WeakWorld, WeakObject]()
			{
				ASpaceship* Piloted = PilotedShip(WeakWorld.Get());
				if (!Piloted || !Piloted->FlightModel || !WeakObject.IsValid()) return LOCTEXT("NoAutopilot", "No ship or target.");
				Piloted->FlightModel->EngageAutopilot(WeakObject.Get());
				return FText::Format(LOCTEXT("AutopilotOn", "AUTOPILOT: {0}"), APSObjectActions::NameOf(WeakObject.Get()));
			};
			OutActions.Add(MoveTemp(Action));
		}
	}

	void Fleet(UWorld* World, AActor* Object, TArray<FAPSObjectAction>& OutActions)
	{
		const FAPSFleetCommand* Command = APSFleetFind(World);
		if (!Command || !Object) return;
		FGuid SystemId;
		const bool bSystem = !Object->IsA<APlanetaryBody>() && FAPSInfrastructure::SiteSystem(World, Object, SystemId);
		const bool bBody = Object->IsA<APlanetaryBody>();
		if (bSystem)
		{
			FText Refusal;
			ASpaceship* Ship = PickOfDivision(*Command, EOrder::Probe, Object, EDivision::Exploration, Refusal);
			if (!Ship) Ship = PickOfDivision(*Command, EOrder::Probe, Object, EDivision::Science, Refusal);
			OutActions.Add(FleetAction(World, TEXT("Fleet.Probe"), LOCTEXT("Probe", "SEND A PROBE"), EOrder::Probe, Object, Ship, Refusal));
			Refusal = FText::GetEmpty();
			Ship = PickOfDivision(*Command, EOrder::SurveySystem, Object, EDivision::Science, Refusal);
			if (!Ship) Ship = PickOfDivision(*Command, EOrder::SurveySystem, Object, EDivision::Exploration, Refusal);
			OutActions.Add(FleetAction(World, TEXT("Fleet.SurveySystem"), LOCTEXT("SurveySystem", "SURVEY THE SYSTEM"),
				EOrder::SurveySystem, Object, Ship, Refusal));
			const FAPSStarSystems* Stars = APSStarSystemsFind(World);
			if (Stars && Stars->AnomalyKindOf(SystemId) != INDEX_NONE && Stars->GetState(SystemId).Anomaly == 2)
			{
				Refusal = FText::GetEmpty();
				Ship = PickOfDivision(*Command, EOrder::Expedition, Object, EDivision::Science, Refusal);
				if (!Ship) Ship = PickOfDivision(*Command, EOrder::Expedition, Object, EDivision::Exploration, Refusal);
				OutActions.Add(FleetAction(World, TEXT("Fleet.SystemExpedition"),
					FText::Format(LOCTEXT("SystemExpedition", "EXPEDITION TO THE {0}"), FAPSStarSystems::AnomalyName(Stars->AnomalyKindOf(SystemId))),
					EOrder::Expedition, Object, Ship, Refusal));
			}
		}
		if (bBody)
		{
			FText Refusal;
			ASpaceship* Ship = PickOfDivision(*Command, EOrder::Survey, Object, EDivision::Exploration, Refusal);
			OutActions.Add(FleetAction(World, TEXT("Fleet.Survey"), LOCTEXT("Survey", "SURVEY"), EOrder::Survey, Object, Ship, Refusal,
				NAME_None, EDivision::Exploration));
			Refusal = FText::GetEmpty();
			Ship = PickOfDivision(*Command, EOrder::Survey, Object, EDivision::Science, Refusal);
			OutActions.Add(FleetAction(World, TEXT("Fleet.Study"), LOCTEXT("Study", "STUDY (SCIENCE)"), EOrder::Survey, Object, Ship, Refusal,
				NAME_None, EDivision::Science));
			if (const FAPSFleetBodyRecord* Record = Command->FindBody(Object);
				Record && Record->bHasAnomaly && Record->Anomaly >= EAnomalyState::Located && Record->Anomaly < EAnomalyState::Investigated)
			{
				Refusal = FText::GetEmpty();
				Ship = PickOfDivision(*Command, EOrder::Expedition, Object, EDivision::Exploration, Refusal);
				if (!Ship) Ship = PickOfDivision(*Command, EOrder::Expedition, Object, EDivision::Science, Refusal);
				OutActions.Add(FleetAction(World, TEXT("Fleet.Expedition"), LOCTEXT("Expedition", "EXPEDITION TO THE ANOMALY"),
					EOrder::Expedition, Object, Ship, Refusal));
			}
			for (const EOrder Order : {EOrder::BuildOutpost, EOrder::BuildStation, EOrder::BuildShipyard, EOrder::BuildHeadquarters})
			{
				Refusal = FText::GetEmpty();
				Ship = PickOfDivision(*Command, Order, Object, EDivision::Construction, Refusal);
				FAPSObjectAction Action = FleetAction(World, FName(*(TEXT("Fleet.") + OrderName(Order).ToString().Replace(TEXT(" "), TEXT("")))),
					OrderName(Order), Order, Object, Ship, Refusal);
				Action.Group = LOCTEXT("GroupConstruction", "CONSTRUCTION");
				OutActions.Add(MoveTemp(Action));
			}
		}
		if (bBody || bSystem || Object->IsA<ATechActor>())
		{
			// Any free ship of the main fleet flies there and holds.
			FText Refusal;
			ASpaceship* Ship = PickOfDivision(*Command, EOrder::Move, Object, EDivision::MainFleet, Refusal);
			OutActions.Add(FleetAction(World, TEXT("Fleet.Move"), LOCTEXT("MoveThere", "SEND THE MAIN FLEET"), EOrder::Move, Object, Ship, Refusal));
		}
	}

	/** The piloted fleet ship's own orders at the object: what its division can do, as the crews would. */
	void Pilot(UWorld* World, AActor* Object, TArray<FAPSObjectAction>& OutActions)
	{
		const FAPSFleetCommand* Command = APSFleetFind(World);
		ASpaceship* Ship = PilotedShip(World);
		const FAPSFleetUnit* Unit = Command && Ship && Object ? Command->FindUnit(Ship) : nullptr;
		if (!Unit) return;
		const EDivision Division = Unit->Division;
		const auto Add = [&](const FName Id, const FText& Label, const EOrder Order)
		{
			if (DivisionCan(Division, Order)) OutActions.Add(PilotAction(World, Id, Label, Order, Object, Ship));
		};
		FGuid SystemId;
		const bool bBody = Object->IsA<APlanetaryBody>();
		if (!bBody && FAPSInfrastructure::SiteSystem(World, Object, SystemId))
		{
			Add(TEXT("Pilot.Probe"), LOCTEXT("PilotProbe", "LAUNCH A PROBE"), EOrder::Probe);
			Add(TEXT("Pilot.SurveySystem"), LOCTEXT("PilotSurveySystem", "CHART THE SYSTEM"), EOrder::SurveySystem);
			const FAPSStarSystems* Stars = APSStarSystemsFind(World);
			if (Stars && Stars->AnomalyKindOf(SystemId) != INDEX_NONE && Stars->GetState(SystemId).Anomaly == 2)
			{
				Add(TEXT("Pilot.SystemExpedition"), FText::Format(LOCTEXT("PilotSystemExpedition", "INVESTIGATE THE {0}"),
					FAPSStarSystems::AnomalyName(Stars->AnomalyKindOf(SystemId))), EOrder::Expedition);
			}
		}
		if (bBody)
		{
			Add(TEXT("Pilot.Survey"), SurveyBy(Division) == ESurvey::Studied ? LOCTEXT("PilotStudy", "STUDY IT")
				: LOCTEXT("PilotSurvey", "SCAN IT"), EOrder::Survey);
			if (const FAPSFleetBodyRecord* Record = Command->FindBody(Object);
				Record && Record->bHasAnomaly && Record->Anomaly >= EAnomalyState::Located && Record->Anomaly < EAnomalyState::Investigated)
			{
				Add(TEXT("Pilot.Expedition"), LOCTEXT("PilotExpedition", "LAND AT THE ANOMALY"), EOrder::Expedition);
			}
			for (const EOrder Order : {EOrder::BuildOutpost, EOrder::BuildStation, EOrder::BuildShipyard, EOrder::BuildHeadquarters})
			{
				Add(FName(*(TEXT("Pilot.") + OrderName(Order).ToString().Replace(TEXT(" "), TEXT("")))), OrderName(Order), Order);
			}
		}
		// The construction catalogue, raised by the ship itself.
		const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
		if (!Infrastructure || !DivisionCan(Division, EOrder::BuildStructure)) return;
		TArray<TPair<FName, FText>> Options;
		Infrastructure->GetOptions(Object, Options);
		for (const TPair<FName, FText>& Option : Options)
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Option.Key);
			if (!Type) continue;
			FAPSObjectAction Action = PilotAction(World, FName(*(TEXT("Pilot.Build.") + Type->Id.ToString())),
				FText::Format(LOCTEXT("PilotBuildType", "BUILD {0} YOURSELF"), Type->Name), EOrder::BuildStructure, Object, Ship,
				Type->Id);
			if (!Option.Value.IsEmpty())
			{
				Action.bEnabled = false;
				Action.Detail = Option.Value;
			}
			else if (Action.bEnabled)
			{
				Action.Detail = FText::Format(LOCTEXT("PilotBuildDetail", "{0}. Costs {1}; yields {2} a minute. Your ship builds it; stay near."),
					Type->Role, APSInfrastructure::DescribeAmounts(Type->Cost), APSInfrastructure::DescribeAmounts(Type->Yield));
			}
			OutActions.Add(MoveTemp(Action));
		}
	}

	void Construction(UWorld* World, AActor* Object, TArray<FAPSObjectAction>& OutActions)
	{
		const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
		const FAPSFleetCommand* Command = APSFleetFind(World);
		if (!Infrastructure || !Command || !Object) return;
		TArray<TPair<FName, FText>> Options;
		Infrastructure->GetOptions(Object, Options);
		for (const TPair<FName, FText>& Option : Options)
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Option.Key);
			if (!Type) continue;
			FText Refusal = Option.Value;
			ASpaceship* Ship = nullptr;
			if (Refusal.IsEmpty()) Ship = Command->PickShipFor(EOrder::BuildStructure, Object, Type->Id, Refusal);
			FAPSObjectAction Action = FleetAction(World, FName(*(TEXT("Build.") + Type->Id.ToString())),
				FText::Format(LOCTEXT("BuildType", "BUILD {0}"), Type->Name), EOrder::BuildStructure, Object, Ship, Refusal, Type->Id);
			Action.Group = APSInfrastructure::DepartmentName(Type->Department);
			Action.Colour = APSInfrastructure::DepartmentColour(Type->Department);
			if (Ship)
			{
				Action.Detail = FText::Format(LOCTEXT("BuildDetail", "{0}. Costs {1}; yields {2} a minute."), Type->Role,
					APSInfrastructure::DescribeAmounts(Type->Cost), APSInfrastructure::DescribeAmounts(Type->Yield));
			}
			OutActions.Add(MoveTemp(Action));
		}
	}

	FText Kilometres(const double Cm)
	{
		return FText::Format(LOCTEXT("MegaKm", "{0} km"), APSUINumber::Number(FMath::RoundToInt64(Cm / 100000.0)));
	}

	FText Radii(const double Ratio)
	{
		FNumberFormattingOptions Two;
		Two.SetMaximumFractionalDigits(2);
		return APSUINumber::Number(Ratio, &Two);
	}

	/**
	 * Rio 03.10: a hub's or megastructure's numbers by its world (its orbit, the ring's radius, the counterweight's height,
	 * the swarm's distance from the star) and where its chain stands: its step and the next one's state here.
	 */
	void DescribeMegastructure(UWorld* World, const AActor* Object, const APSInfrastructure::FType& Type,
		const FAPSInfrastructure& Infrastructure, const TFunctionRef<void(const FText&, const FText&)> Add)
	{
		using APSMegastructures::EKind;
		const AActor* Site = Object->GetAttachParentActor();
		if (!Site)
		{
			return;
		}
		APSMegastructures::FWorldLayout Layout;
		const bool bWorld = APSMegastructures::LayoutAt(Site, Layout);
		const double Radius = Layout.BodyRadiusCm;
		switch (APSMegastructures::KindOf(Type.Visual))
		{
		case EKind::SpaceHub:
		case EKind::GrandHub:
			if (bWorld)
			{
				Add(LOCTEXT("FieldMegaOrbit", "ORBIT"), FText::Format(LOCTEXT("MegaOrbitUp", "{0} up"),
					Kilometres(FVector::Dist(Object->GetActorLocation(), Site->GetActorLocation()) - Radius)));
			}
			Add(LOCTEXT("FieldMegaBerths", "BERTHS"), FText::Format(LOCTEXT("MegaBerths", "+{0} station berths at its world"),
				APSUINumber::Number(Type.HubBerths)));
			break;
		case EKind::SpaceElevator:
			if (bWorld)
			{
				Add(LOCTEXT("FieldMegaCounterweight", "COUNTERWEIGHT"), FText::Format(LOCTEXT("MegaCounterweight",
					"{0} up, at the stationary orbit ({1} radii)"), Kilometres(Layout.CounterweightRadiusCm - Radius),
					Radii(Layout.CounterweightRadiusCm / FMath::Max(Radius, 1.0))));
			}
			break;
		case EKind::OrbitalRing:
			if (bWorld)
			{
				Add(LOCTEXT("FieldMegaRing", "RING RADIUS"), FText::Format(LOCTEXT("MegaRing", "{0} ({1} radii, {2} up)"),
					Kilometres(Layout.RingRadiusCm), Radii(Layout.RingRadiusCm / FMath::Max(Radius, 1.0)),
					Kilometres(Layout.RingRadiusCm - Radius)));
			}
			break;
		case EKind::DysonSwarm:
		case EKind::DysonSphere:
		{
			FNumberFormattingOptions Three;
			Three.SetMaximumFractionalDigits(3);
			Add(LOCTEXT("FieldMegaStar", "FROM THE STAR"), FText::Format(LOCTEXT("MegaAu", "{0} AU"), APSUINumber::Number(
				FVector::Dist(Object->GetActorLocation(), Site->GetActorLocation()) / APSStars::AstronomicalUnitCm, &Three)));
			break;
		}
		default:
			break;
		}
		// The chain: the station is its first step; the next one's state at its place (the swarm's at the star).
		TArray<FName> Steps;
		if (!APSInfrastructure::GetChain(Type.Id, Steps))
		{
			return;
		}
		const int32 Index = Steps.IndexOfByKey(Type.Id);
		const int32 Total = Steps.Num() + 1;
		if (Index == INDEX_NONE || Index + 1 >= Steps.Num())
		{
			Add(LOCTEXT("FieldMegaChain", "CHAIN"), FText::Format(LOCTEXT("MegaChainDone", "STEP {0} OF {1}: THE CHAIN STANDS COMPLETE"),
				APSUINumber::Number(Total), APSUINumber::Number(Total)));
			return;
		}
		const APSInfrastructure::FType* Next = APSInfrastructure::Find(Steps[Index + 1]);
		if (!Next)
		{
			return;
		}
		const AActor* NextSite = Site;
		if (Next->Placement == APSInfrastructure::EPlacement::StarSystem && !Site->IsA<AStar>())
		{
			NextSite = nullptr;
			for (TActorIterator<AStar> It(World); It && !NextSite; ++It)
			{
				NextSite = IsValid(*It) ? *It : nullptr;
			}
		}
		FText State;
		if (NextSite && Infrastructure.CountAt(NextSite, Next->Id) > 0)
		{
			State = LOCTEXT("MegaNextStands", "STANDS");
		}
		else if (NextSite)
		{
			const FText Refusal = Infrastructure.CheckBuild(Next->Id, NextSite);
			State = Refusal.IsEmpty() ? LOCTEXT("MegaNextReady", "READY TO BUILD") : Refusal;
		}
		Add(LOCTEXT("FieldMegaChain", "CHAIN"), FText::Format(LOCTEXT("MegaChainStep", "STEP {0} OF {1}  /  NEXT: {2}{3}"),
			APSUINumber::Number(Index + 2), APSUINumber::Number(Total), Next->Name,
			State.IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("MegaNextState", " ({0})"), State)));
	}

	void EnsureBuiltIns()
	{
		static bool bRegistered = false;
		if (bRegistered) return;
		bRegistered = true;
		Providers().Emplace(TEXT("Navigation"), &Navigation);
		Providers().Emplace(TEXT("Pilot"), &Pilot);
		Providers().Emplace(TEXT("Fleet"), &Fleet);
		Providers().Emplace(TEXT("Construction"), &Construction);
	}
}

void APSObjectActions::RegisterProvider(const FName Name, FProvider Provider)
{
	APSObjectActionsLocal::EnsureBuiltIns();
	for (TPair<FName, FProvider>& Pair : APSObjectActionsLocal::Providers())
	{
		if (Pair.Key == Name) { Pair.Value = MoveTemp(Provider); return; }
	}
	APSObjectActionsLocal::Providers().Emplace(Name, MoveTemp(Provider));
}

void APSObjectActions::Gather(UWorld* World, AActor* Object, TArray<FAPSObjectAction>& OutActions)
{
	APSObjectActionsLocal::EnsureBuiltIns();
	OutActions.Reset();
	if (!World || !Object) return;
	for (const TPair<FName, FProvider>& Pair : APSObjectActionsLocal::Providers())
	{
		if (Pair.Value) Pair.Value(World, Object, OutActions);
	}
}

FText APSObjectActions::KindOf(const AActor* Object)
{
	if (!Object) return FText::GetEmpty();
	if (Object->ActorHasTag(TEXT("APS.Ancient.Site"))) return LOCTEXT("KindAncient", "ANCIENT SITE");
	if (FGuid SystemId; FAPSStarSystems::AnchorSystem(Object, SystemId))
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(Object->GetWorld());
		return Stars && Stars->IsClaimed(SystemId) ? LOCTEXT("KindClaimedSystem", "STAR SYSTEM  /  CLAIMED")
			: LOCTEXT("KindSystem", "STAR SYSTEM");
	}
	if (const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(Object->GetWorld()))
	{
		if (const FAPSBuiltStructure* Built = Infrastructure->FindByActor(Object))
		{
			if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built->Type))
			{
				return FText::Format(LOCTEXT("KindBuilt", "{0}  /  {1}"), APSInfrastructure::CategoryName(Type->Category),
					APSInfrastructure::DepartmentName(Type->Department));
			}
		}
	}
	if (Object->IsA<AStar>()) return LOCTEXT("KindStar", "STAR");
	if (Object->IsA<AMoon>()) return LOCTEXT("KindMoon", "MOON");
	if (Object->IsA<APlanet>()) return LOCTEXT("KindPlanet", "PLANET");
	if (Object->IsA<ASpaceHeadquarters>()) return LOCTEXT("KindHeadquarters", "HEADQUARTERS");
	if (Object->IsA<ASpaceShipyard>()) return LOCTEXT("KindShipyard", "SHIPYARD");
	if (Object->IsA<ASpaceStation>()) return LOCTEXT("KindStation", "STATION");
	if (Object->IsA<AAutonomousOutpost>()) return LOCTEXT("KindOutpost", "OUTPOST");
	if (Object->IsA<AColony>()) return LOCTEXT("KindSettlement", "SETTLEMENT");
	if (const ASpaceship* Ship = Cast<ASpaceship>(Object))
	{
		const FAPSFleetCommand* Fleet = APSFleetFind(Object->GetWorld());
		const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr;
		return Unit ? FText::Format(LOCTEXT("KindShipUnit", "SHIP  /  {0}"), APSFleet::DivisionName(Unit->Division))
			: LOCTEXT("KindShip", "SHIP");
	}
	return FText::FromString(Object->GetClass()->GetName().Replace(TEXT("BP_"), TEXT("")).Replace(TEXT("_C"), TEXT("")).ToUpper());
}

FText APSObjectActions::NameOf(const AActor* Object)
{
	if (!Object) return FText::GetEmpty();
	if (FGuid SystemId; FAPSStarSystems::AnchorSystem(Object, SystemId))
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(Object->GetWorld());
		const FAPSStarSystemInfo* Info = Stars ? Stars->Find(SystemId) : nullptr;
		return Info ? FText::FromString(Info->Name) : LOCTEXT("UnknownSystem", "STAR SYSTEM");
	}
	if (const ACelestialBody* Body = Cast<ACelestialBody>(Object); Body && !Body->AstroName.IsNone())
	{
		return FText::FromString(Body->AstroName.ToString().ToUpper());
	}
	if (const ASpaceship* Ship = Cast<ASpaceship>(Object))
	{
		const FAPSFleetCommand* Fleet = APSFleetFind(Object->GetWorld());
		if (const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr) return FText::FromString(Unit->CallSign);
	}
	if (Object->GetClass()->ImplementsInterface(UItemInfoInterface::StaticClass()))
	{
		const FText Name = IItemInfoInterface::Execute_GetInGameName(Object);
		if (!Name.IsEmpty()) return FText::FromString(Name.ToString().ToUpper());
	}
	// Without a name of its own, the actor's name made readable: "BP_SpaceShipyard_1_C_0" reads "SPACE SHIPYARD 1".
	FString Raw = Object->GetName();
	Raw.RemoveFromStart(TEXT("BP_"));
	if (const int32 Suffix = Raw.Find(TEXT("_C_"), ESearchCase::CaseSensitive, ESearchDir::FromEnd); Suffix != INDEX_NONE)
	{
		Raw.LeftInline(Suffix);
	}
	Raw.RemoveFromEnd(TEXT("_C"));
	FString Readable;
	for (int32 Index = 0; Index < Raw.Len(); ++Index)
	{
		const TCHAR Char = Raw[Index];
		if (Char == TEXT('_'))
		{
			Readable.AppendChar(TEXT(' '));
			continue;
		}
		if (Index > 0 && FChar::IsUpper(Char) && FChar::IsLower(Raw[Index - 1]))
		{
			Readable.AppendChar(TEXT(' '));
		}
		Readable.AppendChar(Char);
	}
	return FText::FromString(Readable.IsEmpty() ? Object->GetName() : Readable.ToUpper());
}

void APSObjectActions::Describe(UWorld* World, const AActor* Object, TArray<TPair<FText, FText>>& OutFields)
{
	using namespace APSObjectActionsLocal;
	OutFields.Reset();
	if (!World || !Object) return;
	const auto Add = [&OutFields](const FText& Label, const FText& Value) { if (!Value.IsEmpty()) OutFields.Emplace(Label, Value); };
	Add(LOCTEXT("FieldDistance", "DISTANCE"), DistanceText(World, Object));
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
	FGuid SystemId;
	if (!Object->IsA<APlanetaryBody>() && FAPSInfrastructure::SiteSystem(World, Object, SystemId))
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(World);
		if (const FAPSStarSystemInfo* Info = Stars ? Stars->Find(SystemId) : nullptr)
		{
			const FAPSStarSystemState State = Stars->GetState(SystemId);
			FNumberFormattingOptions Two;
			Two.SetMaximumFractionalDigits(2);
			Add(LOCTEXT("FieldSpectral", "STAR"), FText::FromString(Info->Spectral.IsEmpty() ? TEXT("-") : Info->Spectral));
			Add(LOCTEXT("FieldStars", "STARS"), APSUINumber::Number(Info->StarCount));
			Add(LOCTEXT("FieldPlanets", "WORLDS"), State.Knowledge >= APSStars::EKnowledge::Scanned
				? APSUINumber::Number(Info->PotentialPlanets) : LOCTEXT("ScanToKnow", "unknown until scanned"));
			Add(LOCTEXT("FieldKnown", "KNOWN"), APSStars::KnowledgeName(State.Knowledge));
			Add(LOCTEXT("FieldClaimed", "CLAIMED"), State.bClaimed ? LOCTEXT("Yes", "YES") : LOCTEXT("No", "NO"));
			Add(LOCTEXT("FieldRoom", "SYSTEM SPHERE"), FText::Format(LOCTEXT("RoomAu", "{0} AU"),
				APSUINumber::Number(Info->RoomCm / APSStars::AstronomicalUnitCm, &Two)));
			Add(LOCTEXT("FieldReach", "NETWORK"), Stars->IsInReach(SystemId) ? LOCTEXT("InReach", "in reach of a relay")
				: LOCTEXT("OutOfReach", "out of reach"));
			if (const int32 Kind = Stars->AnomalyKindOf(SystemId); Kind != INDEX_NONE && State.Anomaly > 0)
			{
				Add(LOCTEXT("FieldSystemAnomaly", "ANOMALY"), FText::Format(LOCTEXT("SystemAnomalyState", "{0}  /  {1}"),
					FAPSStarSystems::AnomalyName(Kind), State.Anomaly >= 3 ? LOCTEXT("Investigated", "INVESTIGATED")
						: State.Anomaly == 2 ? LOCTEXT("Located", "LOCATED: go there or send an expedition")
						: LOCTEXT("Detected", "DETECTED: survey the system to locate it")));
			}
		}
	}
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Object))
	{
		Add(LOCTEXT("FieldType", "TYPE"), StaticEnum<EPlanetType>()
			? FText::FromString(StaticEnum<EPlanetType>()->GetDisplayNameTextByValue(static_cast<int64>(Body->PlanetType)).ToString().ToUpper())
			: FText::GetEmpty());
		Add(LOCTEXT("FieldRadius", "RADIUS"), FText::Format(LOCTEXT("RadiusKm", "{0} km"),
			APSUINumber::Number(FMath::RoundToInt(Body->GetWorldScapeBodyRadiusCm() / 100000.0))));
		// Rio 03.10: where this world's megastructures would stand (a world with ground: the elevator needs it).
		APSMegastructures::FWorldLayout Layout;
		if (Body->PlanetType != EPlanetType::GasGiant && Body->PlanetType != EPlanetType::HotGiant
			&& Body->PlanetType != EPlanetType::IceGiant && APSMegastructures::LayoutAt(Body, Layout))
		{
			Add(LOCTEXT("FieldMegaLayout", "RING / STATIONARY ORBIT"), FText::Format(LOCTEXT("MegaLayout", "{0}  /  {1}"),
				Layout.bRingFits ? Kilometres(Layout.RingRadiusCm) : LOCTEXT("MegaNoRingRoom", "no room for a ring"),
				Layout.bElevatorFits ? Kilometres(Layout.CounterweightRadiusCm) : LOCTEXT("MegaNoOrbitRoom", "no room for an elevator")));
		}
		if (const FAPSFleetCommand* Fleet = APSFleetFind(World))
		{
			Add(LOCTEXT("FieldSurvey", "KNOWN"), APSFleet::SurveyName(Fleet->GetSurvey(Body)));
			Add(LOCTEXT("FieldOutposts", "OUTPOSTS"), APSUINumber::Number(Fleet->CountOutposts(Body)));
			if (const FAPSFleetBodyRecord* Record = Fleet->FindBody(Body); Record && Record->bHasAnomaly
				&& Record->Anomaly != APSFleet::EAnomalyState::Hidden)
			{
				Add(LOCTEXT("FieldAnomaly", "ANOMALY"), APSFleet::AnomalyName(Record->AnomalyKind));
			}
		}
	}
	if (const ASpaceship* Ship = Cast<ASpaceship>(Object))
	{
		if (const FAPSFleetCommand* Fleet = APSFleetFind(World))
		{
			if (const FAPSFleetUnit* Unit = Fleet->FindUnit(Ship))
			{
				Add(LOCTEXT("FieldDivision", "DIVISION"), APSFleet::DivisionName(Unit->Division));
				Add(LOCTEXT("FieldState", "NOW"), Fleet->DescribeState(*Unit));
			}
		}
		Add(LOCTEXT("FieldClass", "CLASS"), FText::FromString(Ship->GetSizeClassName()));
	}
	if (Infrastructure)
	{
		if (const FAPSBuiltStructure* Built = Infrastructure->FindByActor(Object))
		{
			if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built->Type))
			{
				Add(LOCTEXT("FieldRole", "ROLE"), Type->Role);
				Add(LOCTEXT("FieldYield", "YIELD / MIN"), APSInfrastructure::DescribeAmounts(Type->Yield));
				if (APSInfrastructure::IsMegaVisual(Type->Visual))
				{
					DescribeMegastructure(World, Object, *Type, *Infrastructure, Add);
				}
			}
		}
		TArray<const FAPSBuiltStructure*> Here;
		Infrastructure->GetAt(Object, Here);
		if (!Here.IsEmpty())
		{
			FString Names;
			for (const FAPSBuiltStructure* Structure : Here)
			{
				const APSInfrastructure::FType* Type = APSInfrastructure::Find(Structure->Type);
				Names += (Names.IsEmpty() ? TEXT("") : TEXT(", ")) + (Type ? Type->Name.ToString() : Structure->Type.ToString());
			}
			Add(LOCTEXT("FieldStanding", "STANDS HERE"), FText::FromString(Names));
		}
	}
}

#undef LOCTEXT_NAMESPACE
