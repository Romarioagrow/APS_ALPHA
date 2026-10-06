#pragma once

#include "CoreMinimal.h"
#include "APSFlightReadout.h"
#include "Widgets/SCompoundWidget.h"

class ASpaceship;
class SVerticalBox;

/**
 * Rio 06.10 ("Flight HUD v4", Obsidian): the ship's instrument panels. NAVIGATION top right (the active target, range,
 * arrival and the nearest bodies), the flight bar at the bottom centre (autopilot or star drive, speed against the
 * band's limit, what is next, the band) and the key hints under it. Replaces the two text panels; the painted markers
 * and the altimeter stay ASpaceship's.
 */
class SAPSShipHud final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSShipHud) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ASpaceship>, Ship)
		/** M toggles the navigation card. */
		SLATE_ATTRIBUTE(bool, ShowNavigation)
		/** The status line, shown in the flight bar outside band flight (vehicles, the legacy model). */
		SLATE_ATTRIBUTE(FText, StatusText)
		SLATE_ATTRIBUTE(FText, HintText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** The two cards, for the painted markers to keep clear of (APSNavigationHud). */
	TSharedPtr<SWidget> GetNavigationCard() const { return NavigationCard; }
	TSharedPtr<SWidget> GetFlightBar() const { return FlightBar; }

private:
	EActiveTimerReturnType Refresh(double CurrentTime, float DeltaTime);
	TSharedRef<SWidget> BuildNavigationCard();
	TSharedRef<SWidget> BuildFlightBar();
	void RebuildNearest();

	TWeakObjectPtr<ASpaceship> Ship;
	TAttribute<bool> ShowNavigation;
	TAttribute<FText> StatusText;
	FAPSFlightReadout Readout;
	TSharedPtr<SWidget> NavigationCard;
	TSharedPtr<SWidget> FlightBar;
	TSharedPtr<SVerticalBox> NearestList;
	/** The nearest list's names, picked index and distances last drawn: rebuilt only when it changes. */
	FString NearestSignature;
};
