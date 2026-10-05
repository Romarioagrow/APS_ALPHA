#pragma once

#include "CoreMinimal.h"
#include "SAPSStarScheme.h"
#include "Widgets/SCompoundWidget.h"

class AActor;
class ASpaceship;
class STextBlock;
class SVerticalBox;
class UWorld;

/**
 * Rio 05.10 (star map): pieces the colony terminal shares between MAP > STAR MAP and FLEET ORDERS' STARS target picker.
 */
namespace APSStarMapUI
{
	/** The LEGEND switch: shows or hides the legend and hint lines of every star map (remembered); an on/off dot by it. */
	APS_ALPHA_API TSharedRef<SWidget> LegendToggle();
	/**
	 * A star system's anchor as an order target: its kind ("STAR SYSTEM  /  M5V  /  SCANNED"), a line on it (worlds, how
	 * far from home, the relay network) and its anomaly's line (empty when none is known). False for anything else.
	 */
	APS_ALPHA_API bool DescribeSystemTarget(UWorld* World, const AActor* Target, FText& OutKind, FText& OutBody, FText& OutAnomaly);
	/** The planned routes of the picked ships to a star system target ("E-07  ETA ~0:38"), for the STARS picker. */
	APS_ALPHA_API TArray<SAPSStarScheme::FPlannedRoute> PlannedRoutes(UWorld* World, const TArray<ASpaceship*>& Ships,
		const AActor* Target);
	/** The star system an actor stands for (its anchor), else an invalid id. */
	APS_ALPHA_API FGuid SystemOf(const AActor* Actor);
}

/**
 * MAP > STAR MAP (Rio 05.10, approved mock star_level_mock.png): the star scheme with its title and filters over it
 * (ALL / KNOWN / CLAIMED / UNCHARTED, LEGEND), and on the right the picked system's card (distance and rank, what is
 * known, its worlds, the relay network, its anomaly, the ship the next order would send with its ETA), its actions
 * (the next order: a probe, a survey of the system, an expedition; SET COURSE; FLEET ORDERS; OPEN THE SYSTEM SCHEME)
 * and the shown systems nearest first. A double click opens a system's scheme when it stands (home, or the one
 * materialized now); otherwise the card says why.
 */
class APS_ALPHA_API SAPSStarMapPanel final : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnStar, AActor*);
	DECLARE_DELEGATE_OneParam(FOnSystem, const FGuid&);

	SLATE_BEGIN_ARGS(SAPSStarMapPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		/** The ship a course goes to: the piloted one, else the home ship (SAPSColonyTerminal::GetCourseShip). */
		SLATE_ARGUMENT(TFunction<ASpaceship*()>, CourseShip)
		/** The system scheme of a system that stands: its star. */
		SLATE_EVENT(FOnStar, OnOpenScheme)
		/** FLEET ORDERS with this system as the target. */
		SLATE_EVENT(FOnSystem, OnFleetOrders)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	/** Picks a system: the map marks it and the card shows it. */
	void Select(const FGuid& Id);
	TSharedPtr<SAPSStarScheme> GetScheme() const { return Scheme; }

private:
	/** The card's lines for the pick, read twice a second and on every pick. */
	struct FCard
	{
		bool bValid{false};
		FText Name;
		FText Designation;
		FLinearColor Colour{FLinearColor::White};
		FText Kind;
		FText Distance;
		FText Known;
		FLinearColor KnownColour{FLinearColor::White};
		FText Worlds;
		FText Network;
		FLinearColor NetworkColour{FLinearColor::White};
		FText Anomaly;
		FLinearColor AnomalyColour{FLinearColor::White};
		FText BestShip;
		FLinearColor BestShipColour{FLinearColor::White};
		/** The next order for it and whether it can go; its label and the line under it. */
		bool bHasOrder{false};
		bool bOrderReady{false};
		FText OrderLabel;
		FText OrderDetail;
		bool bSchemeReady{false};
		FText SchemeDetail;
	};

	void ReadCard();
	void RebuildList();
	/** The pick's anchor (spawned on first use) or, for home, its star; null without the catalogue. */
	AActor* PickedSite(bool bAnchor) const;
	FReply RunOrder();
	FReply SetCourse();
	FReply OpenScheme();
	FReply FleetOrders();
	void HandleOpened(const FGuid& Id);

	TWeakObjectPtr<UWorld> World;
	TFunction<ASpaceship*()> CourseShip;
	FOnStar OnOpenScheme;
	FOnSystem OnFleetOrders;
	TSharedPtr<SAPSStarScheme> Scheme;
	TSharedPtr<SVerticalBox> List;
	FString ListSignature;
	FCard Card;
	APSFleet::EOrder NextOrder{APSFleet::EOrder::None};
	TWeakObjectPtr<ASpaceship> NextShip;
	FText Message;
	bool bMessageIsError{false};
	float CardClock{0.0f};
	/** The system the pilot is in when it is not home (the map can be centred on it), and its name. */
	FGuid PilotSystemId;
	FText PilotSystemName;
};

/**
 * FLEET ORDERS' ROUTES (Rio 05.10, fleet_target_mock.png): for a star system target, each picked ship of the fleet in its
 * division's colour, where it is now, how far it has to go and about when it arrives. Rows are rebuilt when the pick or
 * the target changes; their values are read live.
 */
class APS_ALPHA_API SAPSStarRoutes final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSStarRoutes) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		SLATE_ARGUMENT(TFunction<TArray<ASpaceship*>()>, Ships)
		SLATE_ARGUMENT(TFunction<AActor*()>, Target)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void Rebuild();

	TWeakObjectPtr<UWorld> World;
	TFunction<TArray<ASpaceship*>()> Ships;
	TFunction<AActor*()> Target;
	TSharedPtr<SVerticalBox> Rows;
	FString Signature;
};
