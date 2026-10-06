#pragma once

#include "CoreMinimal.h"

/**
 * Rio 06.10 HUD: what the ship's flight bar shows, already formatted the way the flight model's status line formats
 * it (UAPSShipFlightModel::GetHudReadout fills it; SAPSShipHud draws it). bValid is false outside band flight (ground
 * vehicles, the legacy model): the bar then shows the status text instead.
 */
struct FAPSFlightReadout
{
	bool bValid{false};

	bool bAutopilot{false};
	FString AutopilotTarget;
	FString AutopilotRemaining;

	bool bStarDrive{false};
	/** 0..1 while the drive spools up, 1 once it cruises. */
	float DriveSpool{1.0f};
	FString DriveSet;
	FString DriveCruise;
	/** Why the drive holds back (a body too near), empty when it does not. */
	FString DriveHeldBy;

	bool bAutoBand{false};
	FString BandName;
	FString Speed;
	FString SpeedLimit;
	/** Speed over the band's limit, for the bar (0 at rest, 1 at the limit). */
	float SpeedFraction{0.0f};
	/** The boost multiplier while boosting, 0 otherwise. */
	float Boost{0.0f};
	bool bEngineRunning{true};

	/** The nearest surface: its body and distance; both empty in open space. */
	FString NearestBody;
	FString NearestDistance;
	/** How the band steers (the status line's last field). */
	FString Control;
	/** The star drive's short notice while it lasts. */
	FString Notice;
};
